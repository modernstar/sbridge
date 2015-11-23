/*
 * SBridge
 * Sangoma WAN - TCP/IP Bridge
 *
 * Author(s):   Moises Silva <moises.silva@gmail.com>
 *
 * Copyright:   (c) 2009 Regulus Labs Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 * 
 */

#include <libsangoma.h>
#include <sqlite3.h>
#include "sangoma_pri.h"
#include "sbridge_log.h"
#include "sbridge_os.h"
#include "sbridge_chan.h"
#include "sbridge_config.h"
#include "sbridge_media.h"
#include "sbridge_v110.h"
#include "sbridge_v32.h"

static sbridge_config_t g_config;
static sbridge_command_options_t g_options;
static int32_t g_errcount = 0;

static int sbridge_ignore_signals[] =
{
	SIGHUP,
	SIGINT,
	SIGPIPE,
	SIGALRM,
	SIGUSR2,
	SIGPOLL,
	SIGPROF,
	SIGVTALRM,
	SIGQUIT,
	SIGIO
};

static int on_hangup(sangoma_pri_t *spri, sangoma_pri_event_t event_type, pri_event *event) 
{
	int32_t chanindex = event->hangup.channel - 1;
	sbridge_span_t *span = spri->private_info;
	sbridge_chan_t *chan = &span->channels[chanindex];
	sbridge_log(SBRIDGE_LOG_NOTICE, "Got hangup signal on channel %d\n", event->hangup.channel);
	
	sbridge_assert(event->hangup.channel == chan->channo, -1);

	sbridge_mutex_lock(&chan->lock);
	if (chan->running) {
		/* we must set the media_thread member to invalid before cancelling the thread, because
		   the media thread will check for the member upon exit to decide if it should send a notification
		   to the signaling thread or not  */
		sbridge_thread_t mediathread = chan->media_thread;
		chan->media_thread = SBRIDGE_INVALID_THREAD_ID;
		sbridge_mutex_unlock(&chan->lock);
		sbridge_log(SBRIDGE_LOG_NOTICE, "Killing channel %d media thread %llu\n", event->hangup.channel, mediathread);
		/* we already have a media thread, let's kill it before continuing 
		 * we don't expect this calls to fail, if they ever fail there is something wrong
		 * at the OS level, we just log the error, continue to hangup the PRI call
		 * and hope for the best
		 * */
		if (!sbridge_thread_cancel(mediathread)) {
			sbridge_log(SBRIDGE_LOG_ERROR, "Failed to cancel thread %llu\n", mediathread);
		}
		if (!sbridge_thread_join(mediathread, NULL)) {
			sbridge_log(SBRIDGE_LOG_ERROR, "Failed to join thread %llu\n", mediathread);
		}
		sbridge_log(SBRIDGE_LOG_NOTICE, "-- Hanging up channel %d on PRI request\n", event->hangup.channel);
		pri_hangup(spri->pri, event->hangup.call, event->hangup.cause);
	} else {
		/* else {} 
		 * if the channel is no longer running means the media thread was about to notify us about the hangup
		 * so we don't do anything here, the signal delivered by the media thread will take care of the cleaning
		 * */
		sbridge_mutex_unlock(&chan->lock);
	}
	return 0;
}

static int on_sqlite3_row(void *arg, int argc, char *value[], char *name[])
{
	int i;
	sbridge_chan_t *chan = arg;
	chan->sqlhit = SBRIDGE_TRUE;
	sbridge_log(SBRIDGE_LOG_DEBUG, "SQL row hit in chan %d\n", chan->channo);
	for (i = 0; i < argc; i++) {
		sbridge_log(SBRIDGE_LOG_DEBUG, "%s == %s\n", name[i], value[i]);
	}
	return 0;
}

static sbridge_bool_t sbridge_is_blacklisted(sbridge_chan_t *chan, const char *callingnumber)
{
	static const char table[] = "blacklist";
	char *sql = NULL;
	char *sqlerr = NULL;
	int res;

	sql = sqlite3_mprintf("SELECT calleridnum FROM %q WHERE calleridnum = \"%q\";", table, callingnumber);
	sbridge_log(SBRIDGE_LOG_DEBUG, "Executing SQL query %s\n", sql);
	chan->sqlhit = SBRIDGE_FALSE;
	res = sqlite3_exec(g_config.db, sql, on_sqlite3_row, chan, &sqlerr);
	if (res != SQLITE_OK) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed executing query %s: %s\n", sql, sqlerr);
		sqlite3_free(sqlerr);
		sqlite3_free(sql);
		return SBRIDGE_TRUE;
	}
	/* TODO: check if sqlerr may ever have something even on success? and therefore needs to be freed? */
	sqlite3_free(sql);
	return chan->sqlhit;
}

static int on_ring(struct sangoma_pri *spri, sangoma_pri_event_t event_type, pri_event *event) 
{
	sbridge_span_t *span;
	sbridge_chan_t *chan;
	int32_t channelindex = event->ring.channel - 1;
	
	sbridge_assert(event->ring.channel >= 1, -1);

	span = spri->private_info;
	chan = &span->channels[channelindex];
	sbridge_log(SBRIDGE_LOG_NOTICE, "-- Ring on channel %d (from %s to %s) with capabilities = 0x%X\n", 
			event->ring.channel, event->ring.callingnum, event->ring.callednum, event->ring.ctype);

	/* check if the number is blacklisted */
	if (sbridge_is_blacklisted(chan, event->ring.callingnum)) {
		sbridge_log(SBRIDGE_LOG_NOTICE, "Rejecting call from blacklisted number %s\n", event->ring.callingnum);
		pri_hangup(spri->pri, event->ring.call, PRI_CAUSE_CALL_REJECTED);
		return 0;
	}

	/*
	 * check the call type
	 * event->ring.ctype is either 
	 * PRI_TRANS_CAP_SPEECH voice call
	 * PRI_TRANS_CAP_DIGITAL, v.110 call
	 * PRI_TRANS_CAP_3_1K_AUDIO, v.32 call
	 * */
	switch (event->ring.ctype) {
	case PRI_TRANS_CAP_DIGITAL:
		sbridge_log(SBRIDGE_LOG_NOTICE, "Receiving V.110 call on channel %d\n", chan->channo);
		chan->is_digital = SBRIDGE_TRUE;
		chan->app_func = sbridge_run_v110;
		break;
	case PRI_TRANS_CAP_3_1K_AUDIO:
		sbridge_log(SBRIDGE_LOG_NOTICE, "Receiving V.32 call on channel %d\n", chan->channo);
		chan->is_digital = SBRIDGE_FALSE;
		chan->app_func = sbridge_run_v32;
		break;
	case PRI_TRANS_CAP_SPEECH:
		sbridge_log(SBRIDGE_LOG_NOTICE, "Receiving voice call on channel %d\n", chan->channo);
		chan->is_digital = SBRIDGE_FALSE;
		chan->app_func = sbridge_media_voice;
		break;
	default:
		sbridge_log(SBRIDGE_LOG_NOTICE, "Rejecting call type %d on chan %d, we only support PRI_TRANS_CAP_DIGITAL, PRI_TRANS_CAP_3_1K_AUDIO and PRI_TRANS_CAP_SPEECH\n", chan->channo);
		pri_hangup(spri->pri, event->ring.call, PRI_CAUSE_BEARERCAPABILITY_NOTAVAIL);
		return 0;
	}

	/* ready to go, launch the media thread */
	chan->call = event->ring.call;
	if (!sbridge_launch_media_thread(chan)) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to launch media thread to handle channel %d, rejecting call\n", event->ring.channel);
		pri_hangup(spri->pri, event->ring.call, PRI_CAUSE_REQUESTED_CHAN_UNAVAIL);
		return -1;
	}

	/* it seems the last argument is 0 when the call is digital, and we only expect digital calls */
	pri_answer(spri->pri, event->ring.call, event->ring.channel, chan->is_digital ? 0 : 1);

	return 0;
}

static int on_restart(struct sangoma_pri *spri, sangoma_pri_event_t event_type, pri_event *event)
{
	sbridge_log(SBRIDGE_LOG_NOTICE, "-- Restarting channel %d\n", event->restart.channel);
	return 0;
}

static int on_anything(struct sangoma_pri *spri, sangoma_pri_event_t event_type, pri_event *event) 
{
	sbridge_log(SBRIDGE_LOG_NOTICE, "%s: Caught Event %d (%s)\n", __FUNCTION__, event_type, sangoma_pri_event_str(event_type));
	return 0;
}

/*! \brief Cleanup the media thread. 
 * when this function is called one of the media threads has ended. This function must always
 * be executed in the span signaling thread as a result of the signal sent by the media thread
 * */
static void shutdown_media_thread(int signum, siginfo_t *siginfo, void *sigptr)
{
	/* since this handler was registered with sigaction (man sigaction), the signal that triggered
	 * this handler is masked during the execution of this signal handler and therefore if other
	 * call ends while we execute this handler it will be deferred until were done here
	 * */
	int32_t spanindex = 0;
	sbridge_thread_t this_thread = sbridge_thread_self();
	/* find the span this thread is handling and set the need_hangup flag */
	for (; spanindex < g_config.numspans; spanindex++) {
		sbridge_span_t *span = &g_config.spans[spanindex];
		if (sbridge_thread_equal(span->signaling_thread, this_thread)) {
			span->need_hangup = SBRIDGE_TRUE;
			return;
		}
	}
	g_errcount++;
	return;
}

/*! \brief called each time before going to sleep waiting for input on the D-channel. Called in sangoma_pri_t lock held */
static int signaling_loop_handler(sangoma_pri_t *spri)
{
	int32_t chanindex;
	sbridge_span_t *span = spri->private_info;
	if (span->need_hangup) {

		sbridge_log(SBRIDGE_LOG_DEBUG, "Hangup of channels requested on span %d\n", span->spanno);

		for (chanindex = 0; chanindex < span->numchans; chanindex++) {
			sbridge_chan_t *chan = &span->channels[chanindex];
			sbridge_mutex_lock(&chan->lock);
			if (!chan->running) {
				/* this call should return immediately given that the media thread has already ended */
				sbridge_thread_join(chan->media_thread, NULL);
				chan->media_thread = SBRIDGE_INVALID_THREAD_ID;
				sbridge_log(SBRIDGE_LOG_NOTICE, "-- Hanging up channel %d on media thread request\n", chan->channo);
				pri_hangup(chan->span->spri.pri, chan->call, -1);
			}
			sbridge_mutex_unlock(&chan->lock);
		}

		sbridge_log(SBRIDGE_LOG_DEBUG, "Done hangup of channels requested on span %d\n", span->spanno);
		span->need_hangup = SBRIDGE_FALSE;
	}
	return 0;
}

static void *run_signaling_thread(void *arg)
{
	sbridge_span_t *span = arg;
	int chanindex;

	span->signaling_thread = sbridge_thread_self();

	sbridge_log(SBRIDGE_LOG_NOTICE, "Handling signaling in thread %llu\n", span->signaling_thread);

	for (chanindex = 0; chanindex < span->numchans; chanindex++) {
		span->channels[chanindex].span = span;
		span->channels[chanindex].media_thread = SBRIDGE_INVALID_THREAD_ID;
		span->channels[chanindex].sangoma_sock = SBRIDGE_INVALID_FD;
		span->channels[chanindex].tcp_sock = SBRIDGE_INVALID_FD;
		span->channels[chanindex].running = SBRIDGE_FALSE;
		span->channels[chanindex].channo = chanindex + 1;
		span->channels[chanindex].is_digital = SBRIDGE_FALSE;
		sbridge_mutex_initialize(&span->channels[chanindex].lock);
	}

	if (sangoma_init_pri(&span->spri, span->spanno, span->dchan, 
			span->switch_type, span->node_type, span->pridebug) < 0) {
		sbridge_log(SBRIDGE_LOG_ERROR, "fatal error, sangoma_init_pri returned error, cannot continue!\n");
		pthread_exit(NULL);
	}
	sbridge_log(SBRIDGE_LOG_NOTICE, "PRI initialized on span %d with debuglevel %d\n", span->spanno, span->pridebug);
	span->spri.private_info = span;
	span->spri.on_loop = signaling_loop_handler;
	SANGOMA_MAP_PRI_EVENT(span->spri, SANGOMA_PRI_EVENT_ANY, on_anything);
	SANGOMA_MAP_PRI_EVENT(span->spri, SANGOMA_PRI_EVENT_RING, on_ring);
	SANGOMA_MAP_PRI_EVENT(span->spri, SANGOMA_PRI_EVENT_HANGUP_REQ, on_hangup);
	SANGOMA_MAP_PRI_EVENT(span->spri, SANGOMA_PRI_EVENT_RESTART, on_restart);

	if (sangoma_run_pri(&span->spri)) {
		// TODO: improve this lame error handling
		sbridge_log(SBRIDGE_LOG_ERROR, "sangoma_run_pri returned error\n");
		pthread_exit(NULL);
	}

	pthread_exit(0);
}

int main(int argc, char *argv[])
{
	int spanindex;
	int s;
	struct sigaction act;
	sbridge_thread_t mainthread;

	/* start initializing the log subsystem */
	sbridge_log_initialize();

	if (!sbridge_parse_arguments(argc, argv, &g_options)) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to read command line arguments\n");
		exit(1);
	}

	/* memsets the configuration (spans, channels etc) to 0 and parse the configuration file */
	if (!sbridge_config_parse(&g_config, g_options.configname)) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to parse configuration file '%s'\n", g_options.configname);
		exit(1);
	}

	sbridge_log_set_level(g_config.loglevel);
	/* provide any argument to not daemonize */
	if (argc == 1 && !sbridge_daemonize()) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to daemonize sbridge\n");
		exit(1);
	}

	/* register signal to get hangup notifications from the media thread */
	memset(&act, 0, sizeof(act));
	act.sa_sigaction = shutdown_media_thread;
	act.sa_flags = SA_SIGINFO;
	if (sigaction(SBRIDGE_MEDIA_THREAD_DONE_SIGNAL, &act, NULL)) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to register signal handler: %s\n", sbridge_os_get_last_error());
		exit(1);
	}

	/* ignore other signals, TODO: does this work in Windows? */
	for (s = 0; s < sizeof(sbridge_ignore_signals)/sizeof(sbridge_ignore_signals[0]); s++) {
		memset(&act, 0, sizeof(act));
		act.sa_handler = SIG_IGN;
		if (sigaction(sbridge_ignore_signals[s], &act, NULL)) {
			sbridge_log(SBRIDGE_LOG_ERROR, "Failed to register %d signal handler: %s\n", 
					sbridge_ignore_signals[s], sbridge_os_get_last_error());
			exit(1);
		}
	}

	for (spanindex = 0; spanindex < g_config.numspans; spanindex++) {
		sbridge_span_t *span = &g_config.spans[spanindex];
		if (!sbridge_thread_create_foreground(&span->signaling_thread, run_signaling_thread, span)) {
			sbridge_log(SBRIDGE_LOG_ERROR, "Failed to launch signaling thread for span %d\n", span->spanno);
			exit(1);
		}
	}

	mainthread = sbridge_thread_self();
	sbridge_log(SBRIDGE_LOG_NOTICE, "Launched %d signaling threads, main thread is %llu\n", spanindex, mainthread);
	for (spanindex = 0; spanindex < g_config.numspans; spanindex++) {
		sbridge_span_t *span = &g_config.spans[spanindex];
		sbridge_log(SBRIDGE_LOG_DEBUG, "SBridge main thread waiting on signaling thread of span %d\n", span->spanno);
		sbridge_thread_join(span->signaling_thread, NULL);
	}
	sbridge_log(SBRIDGE_LOG_NOTICE, "SBridge going out\n");

	return 0;
}


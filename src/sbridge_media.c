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
#include "sangoma_pri.h"
#include "sbridge_log.h"
#include "sbridge_os.h"
#include "sbridge_chan.h"
#include "sbridge_config.h"
#include "sbridge_v110.h"
#include "sbridge_v32.h"
#include "sbridge_media.h"

static void clean_thread_data(void *thread_data)
{
	sbridge_chan_t *chan = thread_data;
	sbridge_log(SBRIDGE_LOG_NOTICE, "Cleaning up media thread data for channel %d\n", chan->channo);

	if (chan->v110) {
		sbridge_v110_free(&chan->v110);
		chan->v110 = NULL;
	}
	if (chan->v32) {
		sbridge_v32_free(&chan->v32);
		chan->v32 = NULL;
	}

	if (chan->trace_raw_in) {
		fclose(chan->trace_raw_in);
		chan->trace_raw_in = NULL;
	}
	if (chan->trace_raw_out) {
		fclose(chan->trace_raw_out);
		chan->trace_raw_out = NULL;
	}

	if (chan->sangoma_sock != SBRIDGE_INVALID_FD) {
		sangoma_socket_close(&chan->sangoma_sock);
		chan->sangoma_sock = SBRIDGE_INVALID_FD;
	}

	if (chan->tcp_sock != SBRIDGE_INVALID_FD) {
		close(chan->tcp_sock);
		chan->tcp_sock = SBRIDGE_INVALID_FD;
	}

	sbridge_mutex_lock(&chan->lock);
	/* chan->media_thread is always set to invalid by the signaling thread on hangup 
	 * if the media thread is still valid it means no hangup has been requested yet
	 * chan->running is our way to make sure the signaling thread does not request
	 * a hangup when the media thread is already requesting one
	 * */
	chan->running = SBRIDGE_FALSE;
	/* media_thread set to invalid means the signaling thread won and they requested a hangup first */
	if (chan->media_thread != SBRIDGE_INVALID_THREAD_ID) {

		sbridge_mutex_unlock(&chan->lock);

		/* even if the signaling thread gets a hangup from the PSTN network at this point, they will not kill us 
		 * because running = false and therefore we are responsible for sending a signal so they can
		 * take care of joining our thread and calling pri_hangup from their signal handler. */

		/* by holding the spri->lock we make sure we don't interrupt our signaling thread in a critical state */
		sbridge_mutex_lock(&chan->span->spri.lock);
		sbridge_thread_notify(chan->span->signaling_thread, SBRIDGE_MEDIA_THREAD_DONE_SIGNAL);
		sbridge_mutex_unlock(&chan->span->spri.lock);

		sbridge_log(SBRIDGE_LOG_NOTICE, "Notifying signaling thread %llu of shutdown\n", chan->span->signaling_thread);
	} else {
		sbridge_log(SBRIDGE_LOG_NOTICE, "Not notifying signaling thread %llu of shutdown, they requested the shutdown\n", chan->span->signaling_thread);
		sbridge_mutex_unlock(&chan->lock);
	}
}

static sbridge_bool_t get_tcp_sock(sbridge_chan_t *chan)
{
	sbridge_hostent_t shp;
	struct hostent *hp;
	struct sockaddr_in addr_in;
	int flags = 0;
	int i = 0;
	short port = 0;
	const char *hostname = NULL;
	sbridge_time_t currtime = sbridge_time(NULL);
	sbridge_config_t *config = chan->span->config;

	for (i = 0; i < config->numhosts; i++) {

		sbridge_spin_lock(&config->tcp_hosts[i].lock);

		if (config->tcp_hosts[i].disabled && (currtime < config->tcp_hosts[i].enable_time)) {

			sbridge_spin_unlock(&config->tcp_hosts[i].lock);

			continue;
		}
		/* if we're here the host is enabled */
		config->tcp_hosts[i].disabled = SBRIDGE_FALSE;

		/* quick access to hostname and port */
		hostname = config->tcp_hosts[i].name;
		port = config->tcp_hosts[i].port;

		sbridge_spin_unlock(&config->tcp_hosts[i].lock);

		/* resolve the host name */ 
		if (!(hp = sbridge_gethostbyname(hostname, &shp))) {
			sbridge_log(SBRIDGE_LOG_ERROR, "Failed to resolve host %s, disabling it!\n", hostname);

			sbridge_spin_lock(&config->tcp_hosts[i].lock);

			config->tcp_hosts[i].disabled = SBRIDGE_TRUE;
			config->tcp_hosts[i].enable_time = currtime + SBRIDGE_HOST_DISABLE_TIME;

			sbridge_spin_unlock(&config->tcp_hosts[i].lock);
			continue;
		}

		/* create the socket */ 
		chan->tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (chan->tcp_sock < 0){
			sbridge_log(SBRIDGE_LOG_ERROR, "Failed to create TCP socket!\n");
			continue;
		}

		memset(&addr_in, 0, sizeof(addr_in));
		addr_in.sin_family = AF_INET;
		addr_in.sin_port = htons(port);
		memcpy(&addr_in.sin_addr, hp->h_addr, sizeof(addr_in.sin_addr));

		/* connect to the tcp server */ 
		if (connect(chan->tcp_sock, (struct sockaddr *)&addr_in, sizeof(addr_in))) {
			sbridge_log(SBRIDGE_LOG_ERROR, "Failed to connect to TCP server %s: %s\n", 
					hostname, sbridge_os_get_last_error());

			sbridge_spin_lock(&config->tcp_hosts[i].lock);

			config->tcp_hosts[i].disabled = SBRIDGE_TRUE;
			config->tcp_hosts[i].enable_time = currtime + SBRIDGE_HOST_DISABLE_TIME;

			sbridge_spin_unlock(&config->tcp_hosts[i].lock);
			continue;
		}
		flags = fcntl(chan->tcp_sock, F_GETFL);
		if (flags < 0) {
			sbridge_log(SBRIDGE_LOG_ERROR, "Failed to get TCP socket flags!\n");
			continue;
		}

		if (fcntl(chan->tcp_sock, F_SETFL, flags | O_NONBLOCK)) {
			sbridge_log(SBRIDGE_LOG_ERROR, "Failed to set TCP socket as O_NONBLOCK!\n");
			continue;
		}
		sbridge_log(SBRIDGE_LOG_NOTICE, "Chan %d is now connected to host %s:%d\n", chan->channo, hostname, port);
		break;
	}
	if (chan->tcp_sock < 0) {
		sbridge_log(SBRIDGE_LOG_WARNING, "No TCP hosts available, forcing enable of all hosts\n");
		chan->tcp_sock = SBRIDGE_INVALID_FD;
		for (i = 0; i < config->numhosts; i++) {
			sbridge_spin_lock(&config->tcp_hosts[i].lock);
			config->tcp_hosts[i].disabled = SBRIDGE_FALSE;
			sbridge_spin_unlock(&config->tcp_hosts[i].lock);
		}
		return SBRIDGE_FALSE;
	}
	return SBRIDGE_TRUE;
}

#define CHAN_MEDIA_TRACE_OPEN(member) \
	snprintf(trace_file_name, sizeof(trace_file_name), "%s-%d-"#member, trace_prefix, chan->channo); \
	chan->member = fopen(trace_file_name, "wb"); \
	if (chan->member) { \
		sbridge_log(SBRIDGE_LOG_DEBUG, "Chan %d activated trace file %s\n", chan->channo, trace_file_name); \
	} else { \
		sbridge_log(SBRIDGE_LOG_ERROR, "Chan %d trace file '%s' could not be open: %s\n", chan->channo, trace_file_name, sbridge_os_get_last_error()); \
	} 
static void *bridge_channel_media(void *channel_data)
{
	unsigned char readframe[MEDIA_BUFFER_SIZE];
        unsigned char writeframe[MEDIA_BUFFER_SIZE];
	char trace_file_name[255];
	char *trace_prefix = NULL;
	wanpipe_tdm_api_t tdm_api;
	sbridge_chan_t *chan = channel_data;
	sangoma_pri_t *spri = &chan->span->spri;
	int32_t channo = chan->channo;

	memset(readframe, 0, sizeof(readframe));
	memset(writeframe, 0, sizeof(writeframe));

	/* need to disable cancellation while registering the cleanup handler
	 * if we get cancelled before registering, no harm done
	 * once the cleanup handler gets registered, it does not matter
	 * at which point of this thread we get cancelled, the cleanup routine
	 * will take care of closing sockets or any other resource we eventually allocate
	 * */
	if (!sbridge_thread_setcancelstate(SBRIDGE_THREAD_CANCEL_DISABLE, NULL)) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to disable thread cancellation in chanel %d\n", channo);
	       	sbridge_thread_exit(NULL);
	}
	if (!sbridge_thread_setcanceltype(SBRIDGE_THREAD_CANCEL_DEFERRED, NULL)) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to set deferred cancel type in chanel %d\n", channo);
		sbridge_thread_exit(NULL);
	}

	/* just grab the lock to start once the thread that launched us is done using the channel 
	 * channel manipulations in this routine do not require the channel lock because the signaling
	 * thread does not access the channel once the media thread is launched
	 * */
	sbridge_mutex_lock(&chan->lock);
	chan->running = SBRIDGE_TRUE;
	sbridge_mutex_unlock(&chan->lock);

	/* register the cleanup handler */
	sbridge_thread_cleanup_push(clean_thread_data, chan);

	/* restore the cancellation state */
	sbridge_thread_setcancelstate(SBRIDGE_THREAD_CANCEL_ENABLE, NULL);

	/* open sangoma media channel */
	if ((chan->sangoma_sock = sangoma_open_tdmapi_span_chan(spri->span, channo)) < 0) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to open TDM API descriptor for sangoma channel %d\n", channo);
		sbridge_thread_exit(NULL);
	}

	if (!get_tcp_sock(chan)) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to get TCP socket for channel %d\n", channo);
		sbridge_thread_exit(NULL);
	}

	/* open the traces */
	trace_prefix = chan->span->config->trace_prefix;
	if (strlen(trace_prefix)) {
		CHAN_MEDIA_TRACE_OPEN(trace_raw_in);
		CHAN_MEDIA_TRACE_OPEN(trace_raw_out);
	}

	/* set the channel read period */
	if (sangoma_tdm_set_usr_period(chan->sangoma_sock, &tdm_api, SBRIDGE_MEDIA_PERIOD) < 0) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to set sangoma channel read period to %d\n", SBRIDGE_MEDIA_PERIOD);
		sbridge_thread_exit(NULL);
	}

	/* everything will be passed thru as received, WP_NONE */
	if ((sangoma_tdm_set_codec(chan->sangoma_sock, &tdm_api, WP_NONE)) < 0) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to set native driver codec for sangoma channel %d\n", channo);
		sbridge_thread_exit(NULL);
	}

	chan->mtu = sangoma_tdm_get_usr_mtu_mru(chan->sangoma_sock, &tdm_api);
	if (chan->mtu != SBRIDGE_ALAW_MTU) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Unexpected MTU of size %d, cannot handle media\n", chan->mtu);
		sbridge_thread_exit(NULL);
	}

	sbridge_log(SBRIDGE_LOG_NOTICE, "Sangoma socket MTU is %d\n", chan->mtu);

	/* run the proper application depending on the chan mode */
	chan->app_func(chan);

	/* done, let's clean up our mess and notify the signaling thread */
	sbridge_log(SBRIDGE_LOG_NOTICE, "Media thread for channel %d completed application execution\n", channo);

	if (!sbridge_thread_setcancelstate(SBRIDGE_THREAD_CANCEL_DISABLE, NULL)) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to disable thread cancellation when terminating media thread for channel %d\n", 
				channo);
	       	sbridge_thread_exit(NULL);
	}

	/* this calls the cleanup handler to free resources */
	sbridge_thread_cleanup_pop(SBRIDGE_THREAD_CLEANUP_EXECUTE);

	sbridge_thread_exit(NULL);
	return NULL;
}

sbridge_bool_t sbridge_launch_media_thread(sbridge_chan_t *chan);
sbridge_bool_t sbridge_launch_media_thread(sbridge_chan_t *chan)
{
	/* we just lock the channel to avoid the thread running before we complete this routine */
	sbridge_mutex_lock(&chan->lock);
	if (chan->media_thread != SBRIDGE_INVALID_THREAD_ID) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Somehow channel %d still has a media thread, cannot launch another (%d)!\n", chan->channo, chan->running);
		sbridge_mutex_unlock(&chan->lock);
		return SBRIDGE_FALSE;
	}
	if (!sbridge_thread_create_foreground(&chan->media_thread, bridge_channel_media, chan)) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to create channel media thread for channel %d: %s\n", 
				chan->channo, sbridge_os_get_last_error());
		sbridge_mutex_unlock(&chan->lock);
		return SBRIDGE_FALSE;
	}
	sbridge_log(SBRIDGE_LOG_NOTICE, "Launched thread %llu to handle channel %d\n", chan->media_thread, chan->channo);
	sbridge_mutex_unlock(&chan->lock);
	return SBRIDGE_TRUE;
}

sbridge_media_frame_t *sbridge_media_read(sbridge_chan_t *chan)
{
	unsigned char *chandata;
	int res = 0;
	sbridge_media_frame_t *f = NULL;

	sbridge_assert(chan != NULL, NULL);
	sbridge_assert(chan->mtu <= SBRIDGE_MEDIA_MAX_FRAME_SIZE, NULL);

	/* allocate enough space for the frame and its payload */
	f = sbridge_calloc(1, sizeof(*f) + (SBRIDGE_MEDIA_MAX_FRAME_SIZE));
	f->datalen = 0;
	f->data.ptr = ((unsigned char *)f + sizeof(*f));
	res = sangoma_readmsg_tdm(chan->sangoma_sock, &chan->hdrframe, 
			sizeof(chan->hdrframe), f->data.ptr, chan->mtu, 0);
	if (res == -1) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to read from sangoma socket: %s\n", strerror(errno));
		free(f);
		return NULL;
	}
	if (res == 0) {
		sbridge_log(SBRIDGE_LOG_ERROR, "0 bytes read from sangoma socket, eof\n");
		free(f);
		return NULL;
	}
	f->datalen = res;
	chandata = f->data.ptr;
	if (chan->trace_raw_in) {
		int len = 0;
		len = fwrite(chandata, 1, f->datalen, chan->trace_raw_in);
		if (len != f->datalen) {
			sbridge_log(SBRIDGE_LOG_WARNING, "just wrote %d bytes of raw input when %d were requested\n", len, f->datalen);
		}
		fflush(chan->trace_raw_in);
	}
	return f;
}

sbridge_bool_t sbridge_media_write(sbridge_chan_t *chan, sbridge_media_frame_t *f)
{
	int32_t rc;
	if (chan->trace_raw_out) {
		int len = 0;
		len = fwrite(f->data.ptr, 1, f->datalen, chan->trace_raw_out);
		if (len != f->datalen) {
			sbridge_log(SBRIDGE_LOG_WARNING, "just wrote %d bytes of raw output when %d were requested\n", len, f->datalen);
		}
		fflush(chan->trace_raw_out);
	}
	rc = sangoma_writemsg_tdm(chan->sangoma_sock, &chan->hdrframe, sizeof(chan->hdrframe), 
			(unsigned char *)f->data.ptr, f->datalen, 0);
	if (rc < 0) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to write to sangoma channel %d: %s\n", chan->channo, strerror(errno));
		return SBRIDGE_FALSE;
	}
	if (rc == 0) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Wow, wrote 0 bytes to sangoma channel %d: %s\n", chan->channo, strerror(errno));
		return SBRIDGE_FALSE;
	}
	return SBRIDGE_TRUE;
}


sbridge_bool_t sbridge_media_voice(sbridge_chan_t *chan)
{
	fd_set readfds;
	sbridge_media_frame_t *frame;
	unsigned char tcprxdata[SBRIDGE_MEDIA_MAX_FRAME_SIZE];
	int32_t tcprxlen = 0;

	sbridge_log(SBRIDGE_LOG_NOTICE, "Entering voice reading loop\n");
	while (1) {
		FD_ZERO(&readfds);
		FD_SET(chan->sangoma_sock, &readfds);

		if (select(chan->sangoma_sock + 1, &readfds, NULL, NULL, NULL) < 0) {
			sbridge_log(SBRIDGE_LOG_ERROR, "select error when waiting for audio channel input: %s\n", sbridge_os_get_last_error());
			break;
		}

		/* read an incoming frame from the board */
		frame = sbridge_media_read(chan);
		if (!frame) {
			sbridge_log(SBRIDGE_LOG_ERROR, "failed to read voice frame from channel %d\n", chan->channo);
			break;
		}

		/* try to read data from the application */
		tcprxlen = read(chan->tcp_sock, tcprxdata, sizeof(tcprxdata));
		if (tcprxlen == 0) {
			sbridge_log(SBRIDGE_LOG_NOTICE, "TCP connection closed on read\n");
			break;
		}

		if (tcprxlen < 0 && errno != EAGAIN) {
			sbridge_log(SBRIDGE_LOG_ERROR, "Error when reading from TCP socket\n");
			break;
		} 

		if (!sbridge_media_write(chan, frame)) {
			sbridge_log(SBRIDGE_LOG_ERROR, "Failed to write %d bytes of voice to sangoma chan %d\n",
					frame->datalen, chan->channo);
			break;
		}

		sbridge_free(frame);
		frame = NULL;
	}

	if (frame) {
		sbridge_free(frame);
	}
	
	sbridge_log(SBRIDGE_LOG_NOTICE, "Done with voice loop on chan %d\n", chan->channo);

	return SBRIDGE_TRUE;

}



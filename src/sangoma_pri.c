/*****************************************************************************
 * sangoma_pri.c libpri Sangoma integration
 *
 * Author(s):	Anthony Minessale II <anthmct@yahoo.com>
 *              Nenad Corbic <ncorbic@sangoma.com>
 *              Moises Silva <moy@sangoma.com>
 *
 * Copyright:	(c) 2005 Anthony Minessale II
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 * ============================================================================
 */

#include <libsangoma.h>
#include "sangoma_pri.h"
#include "sbridge_os.h"
#include "sbridge_log.h"
#ifndef HAVE_GETTIMEOFDAY

#ifdef WIN32
#include <mmsystem.h>

static __inline int gettimeofday(struct timeval *tp, void *nothing)
{
#ifdef WITHOUT_MM_LIB
  SYSTEMTIME st;
  time_t tt;
  struct tm tmtm;
  /* mktime converts local to UTC */
  GetLocalTime (&st);
  tmtm.tm_sec = st.wSecond;
  tmtm.tm_min = st.wMinute;
  tmtm.tm_hour = st.wHour;
  tmtm.tm_mday = st.wDay;
  tmtm.tm_mon = st.wMonth - 1;
  tmtm.tm_year = st.wYear - 1900;  tmtm.tm_isdst = -1;
  tt = mktime (&tmtm);
  tp->tv_sec = tt;
  tp->tv_usec = st.wMilliseconds * 1000;
#else
  /**
   ** The earlier time calculations using GetLocalTime
   ** had a time resolution of 10ms.The timeGetTime, part
   ** of multimedia apis offer a better time resolution
   ** of 1ms.Need to link against winmm.lib for this
   **/
  unsigned long Ticks = 0;
  unsigned long Sec =0;
  unsigned long Usec = 0;
  Ticks = timeGetTime();

  Sec = Ticks/1000;
  Usec = (Ticks - (Sec*1000))*1000;
  tp->tv_sec = Sec;
  tp->tv_usec = Usec;
#endif /* WITHOUT_MM_LIB */
  (void)nothing;
  return 0;
}
#endif /* WIN32 */
#endif /* HAVE_GETTIMEOFDAY */

static struct sangoma_pri_event_list SANGOMA_PRI_EVENT_LIST[] = {
	{0, SANGOMA_PRI_EVENT_ANY, "ANY"},
	{1, SANGOMA_PRI_EVENT_DCHAN_UP, "DCHAN_UP"},
	{2, SANGOMA_PRI_EVENT_DCHAN_DOWN, "DCHAN_DOWN"},
	{3, SANGOMA_PRI_EVENT_RESTART, "RESTART"},
	{4, SANGOMA_PRI_EVENT_CONFIG_ERR, "CONFIG_ERR"},
	{5, SANGOMA_PRI_EVENT_RING, "RING"},
	{6, SANGOMA_PRI_EVENT_HANGUP, "HANGUP"},
	{7, SANGOMA_PRI_EVENT_RINGING, "RINGING"},
	{8, SANGOMA_PRI_EVENT_ANSWER, "ANSWER"},
	{9, SANGOMA_PRI_EVENT_HANGUP_ACK, "HANGUP_ACK"},
	{10, SANGOMA_PRI_EVENT_RESTART_ACK, "RESTART_ACK"},
	{11, SANGOMA_PRI_EVENT_FACNAME, "FACNAME"},
	{12, SANGOMA_PRI_EVENT_INFO_RECEIVED, "INFO_RECEIVED"},
	{13, SANGOMA_PRI_EVENT_PROCEEDING, "PROCEEDING"},
	{14, SANGOMA_PRI_EVENT_SETUP_ACK, "SETUP_ACK"},
	{15, SANGOMA_PRI_EVENT_HANGUP_REQ, "HANGUP_REQ"},
	{16, SANGOMA_PRI_EVENT_NOTIFY, "NOTIFY"},
	{17, SANGOMA_PRI_EVENT_PROGRESS, "PROGRESS"},
	{18, SANGOMA_PRI_EVENT_KEYPAD_DIGIT, "KEYPAD_DIGIT"}
};


const char *sangoma_pri_event_str(sangoma_pri_event_t event_id)
{ 
	return SANGOMA_PRI_EVENT_LIST[event_id].name;
}

static int __pri_sangoma_read(struct pri *pri, void *buf, int buflen)
{
	unsigned char tmpbuf[sizeof(wp_tdm_api_rx_hdr_t)];
	int prifd = pri_fd(pri);	

	/* NOTE: This code will be different for A104 
	 *       A104   receives data + 2byte CRC + 1 byte flag 
	 *       A101/2 receives data only */ 
	
	int res = sangoma_readmsg_socket(prifd, 
					 tmpbuf, sizeof(wp_tdm_api_rx_hdr_t), 
					 buf, buflen, 
					 0);
	if (res > 0){
#ifdef WANPIPE_LEGACY_A104
		/* Prior 2.3.4 release A104 API passed up
                 * 3 extra bytes: 2 CRC + 1 FLAG,
                 * PRI is expecting only 2 CRC bytes, thus we
                 * must remove 1 flag byte */
		res--;
#else
		/* Add 2 byte CRC and set it to ZERO */ 
		memset(&((unsigned char*)buf)[res],0,2);
		res+=2;
#endif
	} else if (res < 0) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to read PRI from sangoma device %d: %s\n", 
				prifd, sbridge_os_get_last_error());
	}

	return res;
}

static int __pri_sangoma_write(struct pri *pri, void *buf, int buflen)
{
	unsigned char tmpbuf[sizeof(wp_tdm_api_rx_hdr_t)];
	int res, myerrno;
	int prifd = pri_fd(pri);

	memset(&tmpbuf[0],0,sizeof(wp_tdm_api_rx_hdr_t));	

	if (buflen < 1){
		/* HDLC Frame must be greater than 2byte CRC */
		sbridge_log(SBRIDGE_LOG_ERROR, "Got short PRI frame of len %d\n", buflen);
		return -1;
	}

	/* FIXME: This might cause problems with other libraries
	 * We must remove 2 bytes from buflen because
	 * libpri sends 2 fake CRC bytes */
	res = sangoma_sendmsg_socket(prifd,
				   tmpbuf, sizeof(wp_tdm_api_rx_hdr_t),
				   buf, (unsigned short)buflen - 2,
				   0);	
	if (res > 0){
		res = buflen;
	} else if (res < 0) {
		myerrno = errno;
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to write %d bytes to PRI sangoma device %d: %s\n", 
				buflen, prifd, sbridge_os_get_last_error());
		if (errno == EBUSY) {
			/* TODO: flush buffers */
		}
	}
	
	return res;
}

int sangoma_init_pri(struct sangoma_pri *spri, int span, int dchan, int swtype, int node, int debug)
{
	int ret = -1;
	sng_fd_t dfd = 0;

	memset(spri, 0, sizeof(struct sangoma_pri));

	if((dfd = sangoma_open_tdmapi_span_chan(span, dchan)) < 0) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Unable to open PRI DCHAN %d for span %d (%s)\n", dchan, span, sbridge_os_get_last_error());
	} else {
		  if ((spri->pri = pri_new_cb((int)dfd, node, swtype, __pri_sangoma_read, __pri_sangoma_write, NULL))){
			spri->span = span;
			pri_set_debug(spri->pri, debug);
			sbridge_mutex_initialize(&spri->lock);
			ret = 0;
		} else {
			sbridge_log(SBRIDGE_LOG_ERROR, "Unable to create PRI for span %d\n", span);
		}
	}
	return ret;
}

int sangoma_close_pri(struct sangoma_pri *spri)
{
	int prifd = pri_fd(spri->pri);
	sangoma_socket_close(&prifd);
	return 0;
}

int sangoma_one_loop(struct sangoma_pri *spri)
{
	fd_set rfds;
	// we dont need efds unless we want to handle span alarms
	// right now sbridge does not makes calls, so there is no point
	// in handling alarms
	//fd_set efds; 
	struct timeval now = {0,0}, *next;
	pri_event *event;
    	int sel;
	
	sbridge_mutex_lock(&spri->lock);
	if (spri->on_loop) {
		spri->on_loop(spri);
	}
	FD_ZERO(&rfds);
	//FD_ZERO(&efds);

#ifdef _MSC_VER
//Windows macro for FD_SET includes a warning C4127: conditional expression is constant
#pragma warning(push)
#pragma warning(disable:4127)
#endif

	FD_SET(pri_fd(spri->pri), &rfds);
	//FD_SET(pri_fd(spri->pri), &efds);

#ifdef _MSC_VER
#pragma warning(pop)
#endif

	if ((next = pri_schedule_next(spri->pri))) {
		gettimeofday(&now, NULL);
		now.tv_sec = next->tv_sec - now.tv_sec;
		now.tv_usec = next->tv_usec - now.tv_usec;
		if (now.tv_usec < 0) {
			now.tv_usec += 1000000;
			now.tv_sec -= 1;
		}
		if (now.tv_sec < 0) {
			now.tv_sec = 0;
			now.tv_usec = 0;
		}
	}
	sbridge_mutex_unlock(&spri->lock);
	//sel = select(pri_fd(spri->pri) + 1, &rfds, NULL, &efds, next ? &now : NULL);
	sel = select(pri_fd(spri->pri) + 1, &rfds, NULL, NULL, next ? &now : NULL);
	sbridge_mutex_lock(&spri->lock);
	event = NULL;

	if (!sel) {
		event = pri_schedule_run(spri->pri);
	} else if (sel > 0) {
		event = pri_check_event(spri->pri);
	} else if (errno != EINTR){
		sbridge_log(SBRIDGE_LOG_ERROR, "select failed: %s\n", sbridge_os_get_last_error());
	} else {
		/* at this point we know there is a select error and is EINTR, ignore */
		sel = 0;
	}

	if (event) {
		event_handler handler;
		/* 0 is catchall event handler */
		const int eventmap_size = sizeof(spri->eventmap)/sizeof(spri->eventmap[0]);
		if (eventmap_size <= event->e) {
			sbridge_log(SBRIDGE_LOG_ERROR, "Event %d is too big, we did not expect it!.\n", event->e);
		} else {
			if ((handler = spri->eventmap[event->e] ? spri->eventmap[event->e] : spri->eventmap[0] ? spri->eventmap[0] : NULL)) {
				handler(spri, event->e, event);
			} else {
				sbridge_log(SBRIDGE_LOG_ERROR, "No event handler found for event %d.\n", event->e);
			}
		}
	}
	sbridge_mutex_unlock(&spri->lock);
	return sel;
}

int sangoma_run_pri(struct sangoma_pri *spri)
{
	int ret = 0;

	for (;;) {
		ret = sangoma_one_loop(spri);
		if (ret < 0){
			sbridge_log(SBRIDGE_LOG_ERROR, "error while looping on sangoma pri, quitting signaling loop, no more calls will be accepted in this span!!\n");
			break;
		}
	}

	return ret;

}


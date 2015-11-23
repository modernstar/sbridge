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

#ifndef __SBRIDGE_CHAN_H__
#define __SBRIDGE_CHAN_H__

#include <libsangoma.h>
#include <sangoma_pri.h>

struct sbridge_v110_s;
struct sbridge_v32_s;
struct sbridge_span_s;
struct sbridge_chan_s;

/*! \brief 20ms chunks of alaw at the typical PSTN 8Khz sampling rate */
#define MEDIA_BUFFER_SIZE 160

/*! \brief function to execute in the channel */
typedef sbridge_bool_t (*sbridge_app_func_t)(struct sbridge_chan_s *chan);

/*! \brief PRI call structure
 *
 * this is the root structure that will be shared between the media thread and the signaling thread.
 * This is a description of how the structure is used and the design of the sangoma bridge itself:
 *
 * When the process starts it will read the provided configuration file. For each span configured
 * it will launch 1 thread known as the signaling thread (signaling_thread member). The signaling thread is
 * responsible for *ALL* the signaling for all the channels on that span. It uses the sangoma PRI abstraction
 * which is based in libpri. This thread basically just sleeps in select() or poll() waiting for data in
 * the D-Channel for the given span. Each time new data arrives the data will be interpreted by libpri
 * and transformed into a call event which is delivered to one of the registered callbacks in this
 * file (on_ring, on_restart, on_hangup etc). There are 2 events of particular interest. 
 *
 * The on ring event will cause the signaling thread to spawn a new thread that will take care of
 * the media. This media thread will immediately open a TCP connection to the modem server 
 * and start sending data back and forth between the modem server and the sangoma socket 
 * (everything read from the sangoma socket is written to the TCP socket and everything read from the TCP
 * socket is written to the sangoma socket).
 *
 * At this point, 2 things may happen. Either the modem server shutdowns the TCP connection or the PRI
 * link shuts down the connection, in either case the call is terminated and the media thread destroyed. 
 *
 * The shutdown process requires synchronization between the media thread and the signaling thread.
 *
 * If the media thread wants to shutdown the call it needs to exit the thread and notify the signaling
 * thread during the shutdown routine registered using sbridge_thread_notify to the signaling thread.
 * The signaling thread then will issue a pri_hangup on the call
 *
 * If the signaling thread is notified by the PRI stack of the call hangup, it needs to notify
 * the media thread. It will do so by using sbridge_thread_cancel. At that point the shutdown
 * routine registered will be executed in the media thread closing the TCP connection and the 
 * sangoma socket.
 *
 * When the running member is set in this structure, the media and signaling threads know
 * the call is up and that they MUST notify the other thread. This helps to handle
 * the case when the signaling thread gets notified of hangup at the other end but the media thread
 * was already shutting down anyways or viceversa.
 *
 * */
typedef struct sbridge_chan_s {
	/*! \brief channel number */
	int32_t channo;

	/*! \brief Span this channel belongs to */
	struct sbridge_span_s *span;

	/*! \brief Media thread id */
	sbridge_thread_t media_thread;

	/*! \brief Sangoma socket of the channel where we read and write media to the E1 device */
	sbridge_fd sangoma_sock;

	/*! \brief TCP socket to the modem server to write media we read from the 
	 * sangoma socket and read data to write to the sangoma socket*/
	sbridge_fd tcp_sock;

	/*! \brief libpri call abstraction handle */
	q931_call *call;

	/*! \brief Whether or not the media thred is running. The signaling thread
	 * uses this field to determine if it should cancel the media thread. The call ends either 
	 * because the media thread ends (because the TCP connection ended) or because the PRI event
	 * hangup arrived and therefore it should cancel the media thread
	 * */
	sbridge_bool_t running;

	/*! \brief if we get an sql hit on this channel for blacklisting numbers */
	sbridge_bool_t sqlhit;

	/* read write header */
	sangoma_api_hdr_t hdrframe;

	/* mtu for this call */
	int32_t mtu;

	/* v110 state if any */
	struct sbridge_v110_s *v110;

	/* v32 state if any */
	struct sbridge_v32_s *v32;

	/* channel data logging path prefixes */
	char raw_trace_prefix[255];
	char data_trace_prefix[255];

	/* raw channel data if tracing is enabled */
	FILE *trace_raw_in;
	FILE *trace_raw_out;

	sbridge_bool_t is_digital;

	/*! \brief channel lock. */
	sbridge_mutex_t lock;

	/*! \brief function to execute */
	sbridge_app_func_t app_func;
} sbridge_chan_t;

#endif


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

#ifndef __SBRIDGE_V110_H__
#define __SBRIDGE_V110_H__

#include "sbridge_os.h"
#include "sbridge_chan.h"
#include "sbridge_media.h"

#ifndef __BYTE_ORDER
#error "__BYTE_ORDER must be defined to compile this application"
#endif

#define LOG_DEBUG stderr
#define LOG_WARNING stderr
#define LOG_ERROR stderr
#define LOG_NOTICE stderr
#define trace_log fprintf

#define URATE_EBITS	0 /* In E-bits or negotiated in-band */
#define URATE_600	1
#define URATE_1200	2
#define URATE_2400	3
#define URATE_3600	4
#define URATE_4800	5
#define URATE_7200	6
#define URATE_8000	7
#define URATE_9600	8
#define URATE_14400	9	/* isdn4linux abuses this for 38400 */
#define URATE_16000	10
#define URATE_19200	11
#define URATE_32000	12

#define URATE_48000	14
#define URATE_56000	15
#define URATE_135	21 /* 134.5 */
#define URATE_100	22
#define URATE_75_1200	23 /* 75 forward, 1200 back */
#define URATE_1200_75	24 /* 1200 forward, 75 back */
#define URATE_50	25
#define URATE_75	26
#define URATE_110	27
#define URATE_150	28
#define URATE_200	29
#define URATE_300	30
#define URATE_12000	31

#define IBUF_LEN 8192
#define OBUF_LEN 1024
#define OBUF_THRESH 16

typedef struct sbridge_v110_s {
	/* related channel */
	sbridge_chan_t *chan;

	/* Input v.110 frame buffer */
	unsigned char vframe_in[10];
	unsigned vframe_in_len;

	/* Input data buffer */
	unsigned char ibuf[IBUF_LEN];
	unsigned ibufend;
	unsigned ibufstart;
	unsigned nextibit;

	/* Output data buffer */
	unsigned char obuf[OBUF_LEN];
	unsigned obufend;
	unsigned obufstart;
	unsigned nextobit;

	/* Output v.110 frame buffer */
	unsigned nextoline;

	int bufwarning;
	unsigned char cts, rts, sbit;
	int synccount;

	/* frame data */
	unsigned char fdata[4096];
	sbridge_media_frame_t outgoing_frame;

	/* malloced each time a new frames gets in */
	sbridge_media_frame_t *incoming_frame;

	/* hooks to call depending on the rate */	
	void (*input_frame)(struct sbridge_v110_s *, sbridge_media_frame_t *);
	void (*fill_outframe)(struct sbridge_v110_s *, int);	

	/* in/out application data after/before being processed by V.110 */
	FILE *trace_data_in;
	FILE *trace_data_out;

	/* in/out V.110 frames */
	FILE *trace_frames_in;
	FILE *trace_frames_out;

	/* read and write file descriptors to buffers to write /read application data */
	int read_buffer;
	int write_buffer;

} sbridge_v110_t;

sbridge_bool_t sbridge_run_v110(sbridge_chan_t *chan);
sbridge_bool_t sbridge_v110_free(sbridge_v110_t **vs);

#endif


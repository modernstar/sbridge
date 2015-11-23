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

#include "sbridge_os.h"
#include "sbridge_log.h"
#include "sbridge_v32.h"
#include "sbridge_media.h"
#include "sbridge_chan.h"
#include "sbridge_config.h"
#include "sbridge_g711.h"

#define SBRIDGE_MAX_V32_DATA 9
sbridge_bool_t sbridge_run_v32(sbridge_chan_t *chan)
{
	V32_INTER *modem_v32;
	fd_set readfds;
	int res;
	int s;
	int32_t tcprxlen;
	int32_t tcptxlen;
	int32_t sanrxlen;
	unsigned char tcprxdata[SBRIDGE_MAX_V32_DATA];
	unsigned char tcptxdata[SBRIDGE_MEDIA_MAX_FRAME_SIZE];
	unsigned short slinear_incoming_data[SBRIDGE_MEDIA_MAX_FRAME_SIZE];
	unsigned short slinear_outgoing_data[SBRIDGE_MEDIA_MAX_FRAME_SIZE];

	sbridge_v32_t *bridge_v32 = sbridge_calloc(1, sizeof(*bridge_v32));	
	if (!bridge_v32) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Error allocating v32 structure\n");
		return SBRIDGE_FALSE;
	}
	modem_v32 = &bridge_v32->modem_v32;
	bridge_v32->chan = chan;
	chan->v32 = bridge_v32;

	bridge_v32->outgoing_frame.data.ptr = &bridge_v32->fdata[0];
	bridge_v32->outgoing_frame.datalen = sizeof(bridge_v32->fdata);

	v32int_start(modem_v32);
	sbridge_log(SBRIDGE_LOG_NOTICE, "bridge_v32->outgoing_frame.datalen = %d\n", bridge_v32->outgoing_frame.datalen);

	while (1) {

		FD_ZERO(&readfds);
		FD_SET(chan->sangoma_sock, &readfds);

		/* free the incoming frame if any */
		if (bridge_v32->incoming_frame) {
			sbridge_free(bridge_v32->incoming_frame);
			bridge_v32->incoming_frame = NULL;
		}

		if (select(chan->sangoma_sock + 1, &readfds, NULL, NULL, NULL) < 0) {
			sbridge_log(SBRIDGE_LOG_ERROR, "select error when waiting for v32 input: %s\n", sbridge_os_get_last_error());
			break;
		}

		/* read an incoming frame from the board */
		bridge_v32->incoming_frame = sbridge_media_read(chan);
		if (!bridge_v32->incoming_frame) {
			sbridge_log(SBRIDGE_LOG_ERROR, "failed to read media V32 frame\n");
			break;
		}
		sanrxlen = bridge_v32->incoming_frame->datalen;

		/* transcode the alaw read data to slinear */
		for (s = 0; s < sanrxlen; s++) {
			slinear_incoming_data[s] = sbridge_alaw_to_linear
				(((unsigned char *)(bridge_v32->incoming_frame->data.ptr))[s]);
		}
		sanrxlen = sanrxlen*2;

		/* try to read data from the application */
		tcprxlen = read(chan->tcp_sock, tcprxdata, sizeof(tcprxdata));
		if (tcprxlen == 0) {
			sbridge_log(SBRIDGE_LOG_NOTICE, "TCP connection closed on read\n");
			break;
		}

		if (tcprxlen < 0 && errno != EAGAIN) {
			sbridge_log(SBRIDGE_LOG_NOTICE, "Error when reading from v32 TCP socket\n");
			break;
		} else if (tcprxlen < 0) {
			/* EAGAIN, most likely not data in the socket at this point */
			tcprxlen = 0;
		}

		/*
		int v32_int(V32_INTER *v32int, unsigned char *TxBits, unsigned char *RxBits, 
		unsigned short *InSmp, unsigned short *OutSmp, int V32DataBytes);

		Called once per 10 msec.
		v32int : pointer to structure of the modem with the interface.
		TxBits : data to be sent, max 9 bytes.
		RxBits : received bits, 
		InSmp : Received 16-bit linear Samples.
		OutSmp : Transmitted 16-bit linear Samples.
		V32DataBytes : Number of byts to be sent.

		Return value : number of received bytes.
		*/

		/* I think we always must transmit whatever v32_int puts in outgoing frame */
		memset(slinear_outgoing_data, 0, sizeof(slinear_outgoing_data));

		tcptxlen = v32_int(modem_v32, 
				     tcprxdata,  /* data that we want to send to the PSTN */
				     tcptxdata,  /* buffer where the rx bits will be put and we will send to the tcp sock */
				     slinear_incoming_data,  /* buffer that we just read from the PSTN and 
									   want to process, the result will be put in tcptxdata */
				     slinear_outgoing_data, /* buffer where the tcp rx data once encoded will be filled
									 and sent to the PSTN
				                                      */
				     tcprxlen /* len of the incoming tcp buffer that will be processed and sent to the PSTN */
				     );

		if (tcptxlen > 0) {
			/* we have bytes to transmit to the TCP sock */
			res = write(chan->tcp_sock, tcptxdata, tcptxlen);
			if (res < 0) {
				sbridge_log(SBRIDGE_LOG_ERROR, "Failed to write %d bytes of V.32 data to TCP socket for chan %d\n",
						tcptxlen, chan->channo);
			}
		}

		/* transcode the outgoing frame to alaw */
		for (s = 0; s < sanrxlen/2; s++) {
			((unsigned char *)(bridge_v32->outgoing_frame.data.ptr))[s] = sbridge_linear_to_alaw(slinear_outgoing_data[s]);
		}

		if (!sbridge_media_write(chan, &bridge_v32->outgoing_frame)) {
			sbridge_log(SBRIDGE_LOG_ERROR, "Failed to write %d bytes of V.32 data to sangoma chan %d\n",
					bridge_v32->outgoing_frame.datalen, chan->channo);
			break;
		}
	}

	sbridge_v32_free(&chan->v32);
	sbridge_log(SBRIDGE_LOG_DEBUG, "Finishing V32 loop\n");

	return SBRIDGE_TRUE;
}

sbridge_bool_t sbridge_v32_free(sbridge_v32_t **vs_p)
{
	sbridge_v32_t *vs = *vs_p;
	if (!vs) {
		return SBRIDGE_FALSE;
	}
	if (vs->incoming_frame) {
		sbridge_free(vs->incoming_frame);
		vs->incoming_frame = NULL;
	}
	sbridge_free(vs);
	*vs_p = NULL;
	return SBRIDGE_TRUE;
}


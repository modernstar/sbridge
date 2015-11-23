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

/* this file is based on the GPL Asterisk application app_v110.c by David Woodhouse <dwmw2@infradead.org> */
/* Some references about V.110 
 * http://www.baty.hanse.de/gsmlink/gsmlink.txt
 * http://www.cisco.com/en/US/tech/tk801/tk36/technologies_tech_note09186a00802b2527.shtml */

#include "sbridge_os.h"
#include "sbridge_log.h"
#include "sbridge_v110.h"
#include "sbridge_media.h"
#include "sbridge_chan.h"
#include "sbridge_config.h"

static void v110_input_frame_x4(sbridge_v110_t *vs, sbridge_media_frame_t *);
static void v110_input_frame_x2(sbridge_v110_t *vs, sbridge_media_frame_t *);
static void v110_input_frame_x1(sbridge_v110_t *vs, sbridge_media_frame_t *);
static void v110_fill_outframe_x4(sbridge_v110_t *vs, int);
static void v110_fill_outframe_x2(sbridge_v110_t *vs, int);
static void v110_fill_outframe_x1(sbridge_v110_t *vs, int);

const unsigned char bit_reverse_table[256] = 
{
	0x00,0x80,0x40,0xc0,0x20,0xa0,0x60,0xe0,
	0x10,0x90,0x50,0xd0,0x30,0xb0,0x70,0xf0,
	0x08,0x88,0x48,0xc8,0x28,0xa8,0x68,0xe8,
	0x18,0x98,0x58,0xd8,0x38,0xb8,0x78,0xf8,
	0x04,0x84,0x44,0xc4,0x24,0xa4,0x64,0xe4,
	0x14,0x94,0x54,0xd4,0x34,0xb4,0x74,0xf4,
	0x0c,0x8c,0x4c,0xcc,0x2c,0xac,0x6c,0xec,
	0x1c,0x9c,0x5c,0xdc,0x3c,0xbc,0x7c,0xfc,
	0x02,0x82,0x42,0xc2,0x22,0xa2,0x62,0xe2,
	0x12,0x92,0x52,0xd2,0x32,0xb2,0x72,0xf2,
	0x0a,0x8a,0x4a,0xca,0x2a,0xaa,0x6a,0xea,
	0x1a,0x9a,0x5a,0xda,0x3a,0xba,0x7a,0xfa,
	0x06,0x86,0x46,0xc6,0x26,0xa6,0x66,0xe6,
	0x16,0x96,0x56,0xd6,0x36,0xb6,0x76,0xf6,
	0x0e,0x8e,0x4e,0xce,0x2e,0xae,0x6e,0xee,
	0x1e,0x9e,0x5e,0xde,0x3e,0xbe,0x7e,0xfe,
	0x01,0x81,0x41,0xc1,0x21,0xa1,0x61,0xe1,
	0x11,0x91,0x51,0xd1,0x31,0xb1,0x71,0xf1,
	0x09,0x89,0x49,0xc9,0x29,0xa9,0x69,0xe9,
	0x19,0x99,0x59,0xd9,0x39,0xb9,0x79,0xf9,
	0x05,0x85,0x45,0xc5,0x25,0xa5,0x65,0xe5,
	0x15,0x95,0x55,0xd5,0x35,0xb5,0x75,0xf5,
	0x0d,0x8d,0x4d,0xcd,0x2d,0xad,0x6d,0xed,
	0x1d,0x9d,0x5d,0xdd,0x3d,0xbd,0x7d,0xfd,
	0x03,0x83,0x43,0xc3,0x23,0xa3,0x63,0xe3,
	0x13,0x93,0x53,0xd3,0x33,0xb3,0x73,0xf3,
	0x0b,0x8b,0x4b,0xcb,0x2b,0xab,0x6b,0xeb,
	0x1b,0x9b,0x5b,0xdb,0x3b,0xbb,0x7b,0xfb,
	0x07,0x87,0x47,0xc7,0x27,0xa7,0x67,0xe7,
	0x17,0x97,0x57,0xd7,0x37,0xb7,0x77,0xf7,
	0x0f,0x8f,0x4f,0xcf,0x2f,0xaf,0x6f,0xef,
	0x1f,0x9f,0x5f,0xdf,0x3f,0xbf,0x7f,0xff
};

static sbridge_media_frame_t *v110_read_media_frame(sbridge_v110_t *s)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	unsigned char *chandata;
	unsigned b;
#endif
	sbridge_media_frame_t *f = sbridge_media_read(s->chan);
	if (!f) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to read V.110 data from channel %d\n", s->chan->channo);
		return NULL;
	}
#if __BYTE_ORDER == __LITTLE_ENDIAN
	chandata = f->data.ptr;
	for (b = 0; b < f->datalen; b++) {
		chandata[b] = bit_reverse_table[chandata[b]];
	}
#endif
	return f;
}

static sbridge_bool_t v110_write_media_frame(sbridge_v110_t *s)
{
	unsigned b;
	unsigned char *fdata = s->outgoing_frame.data.ptr;
	int fdatalen = s->outgoing_frame.datalen;
#if __BYTE_ORDER == __LITTLE_ENDIAN
	for (b = 0; b < fdatalen; b++) {
		fdata[b] = bit_reverse_table[fdata[b]];
	}
#endif
	return sbridge_media_write(s->chan, &s->outgoing_frame);
}

#define V110_TRACE_OPEN(member) \
	snprintf(trace_file_name, sizeof(trace_file_name), "%s-%d-"#member, trace_prefix, chan->channo); \
	vs->member = fopen(trace_file_name, "wb"); \
	if (vs->member) { \
		sbridge_log(SBRIDGE_LOG_DEBUG, "V.110 activated trace file %s\n", trace_file_name); \
	} else { \
		sbridge_log(SBRIDGE_LOG_ERROR, "V.110 trace file '%s' could not be open: %s\n", trace_file_name, strerror(errno)); \
	} 

sbridge_bool_t sbridge_run_v110(sbridge_chan_t *chan)
{
	int res = -1;
	int primelen = 0;
	short urate;
	char trace_file_name[255];
	const char *trace_prefix;
	sbridge_v110_t *vs = NULL;
	sbridge_bool_t rc = SBRIDGE_TRUE;
	fd_set readfds;

	urate = chan->span->config->v110_urate;
	chan->v110 = sbridge_calloc(1, sizeof(*chan->v110)); 
	if (!chan->v110) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Allocation of V.110 data structure failed\n");
		rc = SBRIDGE_FALSE;
		goto out;
	}
	vs = chan->v110;
	vs->chan = chan;
	trace_prefix = chan->span->config->trace_prefix;
	if (strlen(trace_prefix)) {
		V110_TRACE_OPEN(trace_data_in);
		V110_TRACE_OPEN(trace_data_out);
		V110_TRACE_OPEN(trace_frames_in);
		V110_TRACE_OPEN(trace_frames_out);
	}

	vs->read_buffer = chan->tcp_sock;
	vs->write_buffer = chan->tcp_sock;

	/* TODO: Waste bits between characters instead of relying on flow control */
	switch (urate) {
	case URATE_1200:
	case URATE_2400:
	case URATE_4800:
	case URATE_7200:
	case URATE_8000:
	case URATE_9600:
		vs->input_frame = v110_input_frame_x4;
		vs->fill_outframe = v110_fill_outframe_x4;
		primelen = 200;
		break;

	case URATE_12000:
	case URATE_16000:
	case URATE_19200:
		vs->input_frame = v110_input_frame_x2;
		vs->fill_outframe = v110_fill_outframe_x2;
		primelen = 200;
		break;

	case URATE_14400: /* NB. isdn4linux 38400 */
	case URATE_32000:
		vs->input_frame = v110_input_frame_x1;
		vs->fill_outframe = v110_fill_outframe_x1;
		primelen = 200;
		break;

	default:
		sbridge_log(SBRIDGE_LOG_NOTICE, "V.110 call at rate %d not supported\n", urate);
		goto out;
	}

	vs->nextoline = 10;
	vs->rts = 0x80;
	vs->sbit = 0x80;
	vs->cts = 1;
	vs->synccount = 5;
	vs->bufwarning = 5;
	vs->outgoing_frame.data.ptr = vs->fdata;

	sbridge_log(SBRIDGE_LOG_NOTICE, "Accepting V.110 call\n");

	vs->fill_outframe(vs, primelen);
	if (!v110_write_media_frame(vs)) {
		goto out;
	}
	res = 0;
	while (1) {
		int r, want;
		if (vs->incoming_frame) {
			free(vs->incoming_frame);
			vs->incoming_frame = NULL;
		}

		FD_ZERO(&readfds);
		FD_SET(chan->sangoma_sock, &readfds);

		if (select(chan->sangoma_sock + 1, &readfds, NULL, NULL, NULL) < 0) {
			sbridge_log(SBRIDGE_LOG_ERROR, "select error: %s\n", sbridge_os_get_last_error());
			break;
		}

		vs->incoming_frame = v110_read_media_frame(vs);
		if (!vs->incoming_frame) {
			sbridge_log(SBRIDGE_LOG_ERROR, "No incoming frame, terminating v110 loop\n");
			break;
		}

		vs->input_frame(vs, vs->incoming_frame);

		/* normal operation */
		want = vs->incoming_frame->datalen;
		while (want > 4096) {
			vs->fill_outframe(vs, 4096);
			if (!v110_write_media_frame(vs)) {
				res = -1;
				goto out;
			}
			want -= 4096;
		}

		vs->fill_outframe(vs, vs->incoming_frame->datalen);
		if (!v110_write_media_frame(vs)) {
			res = -1;
			break;
		}

		/* Flush v.110 incoming buffer */
		if (vs->ibufend > vs->ibufstart) {
			want = vs->ibufend - vs->ibufstart;
		} else if (vs->ibufend < vs->ibufstart) {
			want = IBUF_LEN - vs->ibufstart;
		} else {
			want = 0;
		}
		if (want) {
			if (vs->trace_data_in) {
				int len = 0;
				len = fwrite(&vs->ibuf[vs->ibufstart], 1, want, vs->trace_data_in);
				if (len != want) {
					sbridge_log(SBRIDGE_LOG_WARNING, "Just wrote %d bytes of input data when %d were requested\n", 
							len, want);
				}
				fflush(vs->trace_data_in);
			}
			r = write(vs->write_buffer, &vs->ibuf[vs->ibufstart], want);
			if (r == 0) {
				sbridge_log(SBRIDGE_LOG_NOTICE, "TCP connection closed on write\n");
				break;
			}
			if (r < 0 && errno == EAGAIN) {
				r = 0;
			}
			if (r < 0) {
				sbridge_log(SBRIDGE_LOG_WARNING, "Error writing data: %s\n", strerror(errno));
				res = -1;
				break;
			}
			vs->ibufstart += r;
			if (vs->ibufstart == IBUF_LEN) {
				vs->ibufstart = 0;
			}

			/* Set flow control state. */
			if (r < want) {
				vs->rts = 0x80;
			} else {
				vs->rts = 0;
			}
		}

		/* Replenish v.110 outgoing buffer */
		if (vs->obufend >= vs->obufstart) {
			if (vs->obufend - vs->obufstart < OBUF_THRESH) {
				want = OBUF_LEN - vs->obufend - !vs->obufstart;
			} else {
				want = 0;
			}
		} else {
			if (vs->obufstart + OBUF_LEN - vs->obufend < OBUF_THRESH) {
				want = vs->obufstart - vs->obufend - 1;
			} else {
				want = 0;
			}
		}
		if (want) {
			r = read(vs->read_buffer, &vs->obuf[vs->obufend], want);
			if (r == 0) {
				sbridge_log(SBRIDGE_LOG_NOTICE, "TCP connection closed on read\n");
				break;
			}
			if (r < 0 && errno == EAGAIN) {
				r = 0;
			}
			if (r < 0) {
				/* It's expected that we get error when the other end closes the pipe or pty */
				sbridge_log(SBRIDGE_LOG_NOTICE, "Error reading v110 buffer: %s\n", strerror(errno));
				break;
			}
			if (vs->trace_data_out) {
				int len = 0;
				len = fwrite(&vs->obuf[vs->obufend], 1, r, vs->trace_data_out);
				if (len != r) {
					sbridge_log(SBRIDGE_LOG_WARNING, "Just wrote %d bytes of output data when %d were requested\n", 
							len, want);
				}
				fflush(vs->trace_data_out);
			}
			vs->obufend += r;
			if (vs->obufend == OBUF_LEN) {
				vs->obufend = 0;
			}
		}
	}


out:
	sbridge_log(SBRIDGE_LOG_NOTICE, "Going out of app_v110\n");

	/* free structures */
	if (chan->v110) {
		sbridge_v110_free(&chan->v110);
	}

	return res;
}

sbridge_bool_t sbridge_v110_free(sbridge_v110_t **vs_p)
{
	sbridge_v110_t *vs = *vs_p;

	if (!vs) {
		return SBRIDGE_FALSE;
	}

	if (vs->incoming_frame) {
		free(vs->incoming_frame);
		vs->incoming_frame = NULL;
	}

	/* close traces */
	if (vs->trace_data_in) {
		fclose(vs->trace_data_in);
		vs->trace_data_in = NULL;
	}
	if (vs->trace_data_out) {
		fclose(vs->trace_data_out);
		vs->trace_data_out = NULL;
	}

	if (vs->trace_frames_in) {
		fclose(vs->trace_frames_in);
		vs->trace_frames_in = NULL;
	}
	if (vs->trace_frames_out) {
		fclose(vs->trace_frames_out);
		vs->trace_frames_out = NULL;
	}

	sbridge_free(vs);
	*vs_p = NULL;

	return SBRIDGE_TRUE;
}

static void v110_process_frame(sbridge_v110_t *vs);
static void v110_process_frame(sbridge_v110_t *vs) 
{
/*
 * http://www.baty.hanse.de/gsmlink/gsmlink.txt
 * The start bit is always 0,
 * followed by a fixed number (usually 8) of arbitrary data bits followed by
 * an arbitrary number (at least 1, which correspond to the the stop bit)
 * of fill bits which are always 1.
 * */
	int octet;

	if (0) {
		sbridge_log(SBRIDGE_LOG_NOTICE, "frame %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			vs->vframe_in[0], vs->vframe_in[1], vs->vframe_in[2],
			vs->vframe_in[3], vs->vframe_in[4], vs->vframe_in[5],
			vs->vframe_in[6], vs->vframe_in[7], vs->vframe_in[8], 
			vs->vframe_in[9]);
	}

	/* Check that line 5 (E-bits) starts '1011'. */
	if ((vs->vframe_in[5] & 0xf) != 0xd) {
		return;
	}

	/* Check that each other octet starts with '1' */
	if (!(vs->vframe_in[1] & vs->vframe_in[2] & vs->vframe_in[3] & 
	      vs->vframe_in[4] & vs->vframe_in[6] & vs->vframe_in[7] & 
	      vs->vframe_in[8] & vs->vframe_in[9] & 0x01)) {
		return;
	}
		
	/* Extract flow control signal from last octet */
	if (vs->synccount) {
		if (!--vs->synccount) {
			sbridge_log(SBRIDGE_LOG_NOTICE, "V.110 synchronisation achieved\n");
			vs->sbit = 0;
			vs->rts = 0;
		}
	} else {
		vs->cts = vs->vframe_in[7] & 0x80;
	}

	if (vs->trace_frames_in) {
		fwrite(vs->vframe_in, 10, 1, vs->trace_frames_in);
	}

	/* iterate over the V.110 frame octets */
	for (octet = 1; octet < 10; octet++) {
		unsigned char tmp;

		/* Skip E-bits in line 5 */
		if (octet == 5) {
			continue;
		}

		tmp = vs->vframe_in[octet] & 0x7e;

		/* Search for start bit if not yet found */
		if (!vs->nextibit) {

			/* First check for no zero bits. This will be common (01111110) */
			if (tmp == 0x7e) {
				continue;
			}

			/* optimizing start bit search by checking if its the last in the octet */
			/* Check for start bit being last in the octet (00111110)*/
			if (tmp == 0x3e) {
				vs->nextibit = 1; /* Expecting first data bit now */
				vs->ibuf[vs->ibufend] = 0;
				continue;
			}
			
			/* Scan for the start bit, copy the data bits (of which
			   there will be at least one) into the next byte of ibuf */
			vs->nextibit = 7;
			do {
				tmp >>= 1;
				vs->nextibit--;
			} while (tmp & 1);

			/* Start bit is now (host's) LSB */
			vs->ibuf[vs->ibufend] = tmp >> 1; 
			continue;
		}

		/* get rid of the status bit */
		tmp >>= 1;

		if (vs->nextibit < 9) {
			/* Add next bits of incoming byte to ibuf */
			vs->ibuf[vs->ibufend] |= tmp << (vs->nextibit-1);

			
			if (vs->nextibit <= 3) {
				/* Haven't finished this byte (including stop) yet */
				vs->nextibit += 6;
				continue;
			}
			tmp >>= (9 - vs->nextibit);
		}

		/* Check for stop bit */
		if (tmp & 1) {
			unsigned newend = (vs->ibufend + 1) & (IBUF_LEN-1);

			if (newend == vs->ibufstart) {
				/* Buffer full. This shouldn't happen because we should
				   have asserted flow control long ago */
				if (vs->bufwarning) {
					vs->bufwarning--;
					sbridge_log(SBRIDGE_LOG_NOTICE, "incoming buffer full\n");
				}
				continue;
			} else {
				vs->ibufend = newend;
			}
		} else {
			sbridge_log(SBRIDGE_LOG_NOTICE, "No stop bit\n");
		}
		
		/* Now, scan for next start bit */
		tmp >>= 1;
		vs->nextibit -= 4;
		while (vs->nextibit && (tmp & 1)) {
			tmp >>= 1;
			vs->nextibit--;
		}
		if (vs->nextibit > 1) {
			vs->ibuf[vs->ibufend] = tmp >> 1;
		}
			
	}

}

/* We don't handle multiple multiplexed channels. Nobody really does */
static void v110_input_frame_x4(sbridge_v110_t *vs, sbridge_media_frame_t *f)
{
	int datalen = f->datalen;
	unsigned char *frame_data = f->data.ptr;

	while (datalen) {
		/* find 4 3f in a row (3f 3f 3f 3f) which in binary is 00111111 
		 * for adaption rate 16Kbps (http://www.cisco.com/en/US/tech/tk801/tk36/technologies_tech_note09186a00802b2527.shtml)
		 * just 2 bytes out of each byte we get from the f->data.ptr belong to us, the other is just bit stuffing of the
		 * rate adaption, *frame_data &  3 is looking for these 2 bits (inverted, 11111100), which, once found 4 times, 
		 * that makes 1 octet of zeroes, if there is no zeroes then we reset vs->frame_in_len to 0 since they need to be 
		 * contiguous
		 * 00111111
		 * 00111111
		 * 00111111
		 * 00111111
		 *
		 * now, when we see 0x3f 0x3f 0x3f 0x3f in a file, 4 v110 octets later (16 raw bytes later) we will see 0xbf
		 * again, the 2 MSB in the wire are the first 2 bytes of the 5th octet of the vframe, which means 10111111
		 * the first bit on is always there for v.110 frames, the second bit is off and if we look for the following 3
		 * raw bytes (to get the remaining 6 bits of the 5th v.110 octet) we will see that E1, E2 and E3 are 011, which
		 * means 48 bits adaption
		 * */
		if (vs->vframe_in_len < 4) {
			/* Find zero octet in buffer */
			if ( (*frame_data) & 3 ) {
				vs->vframe_in_len = 0;
				frame_data++;
				datalen--;
				continue;
			}
			/* Found a suitable byte. Add it. */
			if (++vs->vframe_in_len == 4) {
				memset(vs->vframe_in, 0, 10);
			}
			frame_data++;
			datalen--;
			continue;
		}
		
		/* once we got the octet filled with zeroes, we take 2 bits out of each frame and add each other
		 * until we get the full byte, therefore we add 4 times before moving on to the next byte 
		 * vs->frame_in_len/4) */

		/* Add in these two bits */
		vs->vframe_in[vs->vframe_in_len/4] |= ((*frame_data) & 3) << ((vs->vframe_in_len & 3) * 2);

		vs->vframe_in_len++;
		frame_data++;
		datalen--;

		/* once we have 40 bytes of raw data, we have 10 bytes (1/4) of v110 data, therefore a full
		 * 80 bit frame
		 * */
		if (vs->vframe_in_len == 40) {
			v110_process_frame(vs);
			vs->vframe_in_len = 0;
		}
	}
}

static void v110_input_frame_x2(sbridge_v110_t *vs, sbridge_media_frame_t *f)
{
	int datalen = f->datalen;
	unsigned char *frame_data = f->data.ptr;

	while (datalen) {
		if (vs->vframe_in_len < 2) {
			/* Find zero octet in buffer */
			if ( (*frame_data) & 7 ) {
				vs->vframe_in_len = 0;
				frame_data++;
				datalen--;
				continue;
			}
			/* Found a suitable byte. Add it. */
			if (++vs->vframe_in_len == 2) {
				memset(vs->vframe_in, 0, 10);
			}
			frame_data++;
			datalen--;
			continue;
		}
		/* Add in these four bits */
		vs->vframe_in[vs->vframe_in_len/2] |= ((*frame_data) & 15) << ((vs->vframe_in_len & 1) * 4);

		vs->vframe_in_len++;
		frame_data++;
		datalen--;

		if (vs->vframe_in_len == 20) {
			v110_process_frame(vs);
			vs->vframe_in_len = 0;
		}
	}
}

static void v110_input_frame_x1(sbridge_v110_t *vs, sbridge_media_frame_t *f)
{
	int datalen = f->datalen;
	unsigned char *frame_data = f->data.ptr;

	while (datalen) {
		if (!vs->vframe_in_len) {
			/* Find zero octet in buffer */
			if ( (*frame_data)) {
				vs->vframe_in_len = 0;
				frame_data++;
				datalen--;
				continue;
			}
			/* Found a suitable byte. Add it. */
			vs->vframe_in_len++;
			memset(vs->vframe_in, 0, 10);
			frame_data++;
			datalen--;
			continue;
		}
		/* Add byte to frame */
		vs->vframe_in[vs->vframe_in_len] = *frame_data;

		vs->vframe_in_len++;
		frame_data++;
		datalen--;

		if (vs->vframe_in_len == 10) {
			v110_process_frame(vs);
			vs->vframe_in_len = 0;
		}
	}
}

/* Some bitmasks to ease calculation. */
static unsigned char helper1[] = { 0x81, 0x81, 0x81, 0xc1, 0xe1, 0xf1, 0xf9, 0xfd, 0xff };
static unsigned char helper2[] = { 0x81, 0x83, 0x87, 0x8f, 0x9f, 0xbf };

static unsigned char v110_getline(sbridge_v110_t *vs);
static unsigned char v110_getline(sbridge_v110_t *vs)
{
	unsigned char octet;
	int line = vs->nextoline++;
	int place = 2;

	if (line == 10) {
		vs->nextoline = 1;
		return 0x00; /* Header */
	} else if (line == 5) {
		return 0xfd; /* E-bits. 10111111 (reversed) */
	} else if (line == 2 || line == 7) {
		octet = 0x7f | vs->rts;
	} else {
		octet = 0x7f | vs->sbit;
	}

	/* If we're already sending a byte, finish it */
	if (vs->nextobit) {
		unsigned char tmp;

		/* Shift the data byte so that the bit we want is in bit 1 */
		tmp = vs->obuf[vs->obufstart] >> (vs->nextobit - 2);

		/* Mask in the bits we don't want to touch and the stop bit */
		tmp |= helper1[vs->nextobit - 1];

		/* Clear bits in the generated octet to match */
		octet &= tmp;

		if (vs->nextobit < 4) {
			/* There's some of this byte left; possibly just the stop bit */
			vs->nextobit += 6;
			return octet;
		}

		/* We've finished this byte */
		vs->obufstart++;
		if (vs->obufstart == OBUF_LEN) {
			vs->obufstart = 0;
		}

		if (vs->nextobit < 5) {
			/* But there's still no room in this octet for any more */
			vs->nextobit = 0;
			return octet;
		}
		/* Work out where to put the next data byte */
		place = 12 - vs->nextobit;
		vs->nextobit = 0;
	} else {
		/* Nothing to follow; start bit of new byte at bit 1 */
		place = 2;
	}

	/* Honour flow control when starting new characters */
	if (vs->cts || vs->obufstart == vs->obufend) {
		return octet;
	}

	/* 'place' is the location within the octet to start the new
	   data byte. It'll be 2 unless we've already got the tail of
	   a previous data byte in this octet. If you're starting at it
	   and think there's an off-by-one error, remember the start bit
	   which is zero, and in bit (place-1). */
	octet &= (vs->obuf[vs->obufstart] << place) | helper2[place-2];
	vs->nextobit = 8 - place;

	return octet;
}

static void v110_fill_outframe_x4(sbridge_v110_t *vs, int datalen)
{
	unsigned char *pos = vs->outgoing_frame.data.ptr;

	if (datalen & 3) {
		datalen = (datalen + 3) & ~3;
	}

	vs->outgoing_frame.datalen = datalen;

	while (datalen) {
		unsigned char tmp = v110_getline(vs);
		pos[0] = 0xfc | (tmp & 3);
		tmp >>= 2;
		pos[1] = 0xfc | (tmp & 3);
		tmp >>= 2;
		pos[2] = 0xfc | (tmp & 3);
		tmp >>= 2;
		pos[3] = 0xfc | tmp;
		pos += 4;
		datalen -= 4;
	}
}

static void v110_fill_outframe_x2(sbridge_v110_t *vs, int datalen)
{
	unsigned char *pos = vs->outgoing_frame.data.ptr;

	if (datalen & 1) {
		vs->outgoing_frame.datalen = datalen = datalen + 1;
	}

	vs->outgoing_frame.datalen = datalen;

	while (datalen) {
		unsigned char tmp = v110_getline(vs);
		pos[0] = 0xf0 | (tmp & 15);
		tmp >>= 4;
		pos[1] = 0xf0 | tmp;
		pos += 2;
		datalen -= 2;
	}
}

static void v110_fill_outframe_x1(sbridge_v110_t *vs, int datalen)
{
	unsigned char *pos = vs->outgoing_frame.data.ptr;

	vs->outgoing_frame.datalen = datalen;

	while (datalen) {
		*pos = v110_getline(vs);
		pos++;
		datalen--;
	}
}




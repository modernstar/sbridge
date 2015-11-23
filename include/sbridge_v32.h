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

#ifndef __SBRIDGE_V32_H__
#define __SBRIDGE_V32_H__

#include "sbridge_os.h"
#include "sbridge_chan.h"
#include "sbridge_media.h"
#include "v32_int.h"

typedef struct sbridge_v32_s {
	/* related channel */
	sbridge_chan_t *chan;

	/* v32 modem */
	V32_INTER modem_v32;

	sbridge_media_frame_t *incoming_frame;

	unsigned char fdata[SBRIDGE_MEDIA_MAX_FRAME_SIZE];
	sbridge_media_frame_t outgoing_frame;
} sbridge_v32_t;

sbridge_bool_t sbridge_run_v32(sbridge_chan_t *chan);
sbridge_bool_t sbridge_v32_free(sbridge_v32_t **vs);

#endif


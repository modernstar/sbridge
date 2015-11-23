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

#ifndef __SBRIDGE_MEDIA_H__
#define __SBRIDGE_MEDIA_H__

#include "sbridge_chan.h"

#define SBRIDGE_MEDIA_THREAD_DONE_SIGNAL SIGUSR1

/* assuming 10ms chunks, in the worst case 16 bit samples */
#define SBRIDGE_MEDIA_PERIOD 10
//#define SBRIDGE_MEDIA_MAX_FRAME_SIZE 160
//#define SBRIDGE_LINEAR_MTU 160
#define SBRIDGE_MEDIA_MAX_FRAME_SIZE 80
#define SBRIDGE_ALAW_MTU 80

typedef struct sbridge_data_frame_s {
	int datalen;
	union {
		void *ptr;
	} data;
} sbridge_media_frame_t;

/*! \brief Launches the media thread for the given channel
 * \param chan SBridge channel
 * \return true on success, false on failure
 */
sbridge_bool_t sbridge_launch_media_thread(sbridge_chan_t *chan);

/*! \brief Reads a media frame from the given channel
 * \param chan SBridge channel
 * \return the allocated media frame, when youre done with it you must free it with sbridge_free
 */
sbridge_media_frame_t *sbridge_media_read(sbridge_chan_t *chan);

/*! \brief Writes a media frame to the given channel
 * \param chan SBridge channel
 * \return true on success, false on failure
 */
sbridge_bool_t sbridge_media_write(sbridge_chan_t *chan, sbridge_media_frame_t *f);

/*! \brief Executes a simple voice application on the channel
 * \param chan SBridge channel
 * \return true on success, false on failure
 */
sbridge_bool_t sbridge_media_voice(sbridge_chan_t *chan);

#endif /* __SBRIDGE_MEDIA_H__ */


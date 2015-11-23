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

#ifndef __SBRIDGE_LOG_H__
#define __SBRIDGE_LOG_H__

#include "sbridge_os.h"

#define SBRIDGE_LOG_NAME "sbridge"

typedef enum {
	SBRIDGE_LOG_EXDEBUG = 0,
	SBRIDGE_LOG_DEBUG,
	SBRIDGE_LOG_NOTICE,
	SBRIDGE_LOG_WARNING,
	SBRIDGE_LOG_ERROR
} sbridge_log_level_t;

/*! \brief initialize the logging subsystem
 * \param level Desired logging level 
 * \return true on success, false on failure
 */
sbridge_bool_t sbridge_log_initialize(void);

/*! \brief set the logging level
 * \param level Desired logging level 
 * \return true on success, false on failure
 */
sbridge_bool_t sbridge_log_set_level(sbridge_log_level_t level);

/*! \brief Logs a message with the given format
 * \param level Desired logging level
 * \param message_format Message format
 * \param ... variable printf()-like arguments
 */
void sbridge_log(sbridge_log_level_t level, const char *message_format, ...);


/*! \brief Gets a string representing the given log level
 * \param level Desired logging level
 * \return string representing the log level
 */
const char *sbridge_log_get_level_string(sbridge_log_level_t level);

#endif


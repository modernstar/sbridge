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
#ifdef __linux__
#include <syslog.h>
#endif

typedef struct sbridge_log_globals_s {
	int32_t loglevel;
	sbridge_bool_t initialized;
	sbridge_mutex_t mutex;
} sbridge_log_globals_t;

static sbridge_log_globals_t sbridge_log_globals = 
{
	.initialized = 0,
	.mutex = SBRIDGE_MUTEX_DEFAULT_INITIALIZER,
	.loglevel = SBRIDGE_LOG_DEBUG
};

sbridge_bool_t sbridge_log_initialize(void)
{
#ifdef __linux__
	openlog(SBRIDGE_LOG_NAME, LOG_PID | LOG_CONS, LOG_DAEMON);
	syslog(LOG_INFO, "Starting sbridge log subsytem\n");
#endif
	sbridge_log_globals.initialized = 1;
	return SBRIDGE_TRUE;
}

sbridge_bool_t sbridge_log_set_level(sbridge_log_level_t level)
{
	sbridge_assert(sbridge_log_globals.initialized, -1);

	sbridge_mutex_lock(&sbridge_log_globals.mutex);
	sbridge_log_globals.loglevel = level;
	sbridge_mutex_unlock(&sbridge_log_globals.mutex);
	return SBRIDGE_TRUE;
}

void sbridge_log(sbridge_log_level_t level, const char *message_format, ...)
{
	va_list ap; 
	int32_t res; 
	sbridge_timeval_t currtime;
	sbridge_tm currtime_tm;
	sbridge_time_t currsec;
	char logmessage[512];
	char *strmessage = logmessage;

	sbridge_assert(sbridge_log_globals.initialized, );

	res = sbridge_gettimeofday(&currtime);
	currsec = sbridge_time(NULL);

	if (NULL == sbridge_localtime_r(&currsec, &currtime_tm)) {
		fprintf(stderr, "sbridge_localtime_r failed\n");
		return;
	}

	if (level >= sbridge_log_globals.loglevel) {
		va_start(ap, message_format);
		res = sprintf(strmessage, "[%02d:%02d:%03lu][%s] - ", currtime_tm.tm_min, currtime_tm.tm_sec, 
				currtime.tv_usec/1000, sbridge_log_get_level_string(level));
		strmessage += res;
		vsnprintf(strmessage, sizeof(logmessage)-res, message_format, ap);
		fprintf(stdout, "%s", logmessage);
		fflush(stdout);
#ifdef __linux__
		syslog(LOG_INFO, "%s", logmessage);
#endif
		va_end(ap);
	}

}

const char *sbridge_log_get_level_string(sbridge_log_level_t level)
{
	switch (level) {
	case SBRIDGE_LOG_EXDEBUG:
		return "EXDEBUG";
	case SBRIDGE_LOG_DEBUG:
		return "DEBUG";
	case SBRIDGE_LOG_NOTICE:
		return "NOTICE";
	case SBRIDGE_LOG_WARNING:
		return "WARNING";
	case SBRIDGE_LOG_ERROR:
		return "ERROR";
	}
	return "UnknownDebugLevel";
}


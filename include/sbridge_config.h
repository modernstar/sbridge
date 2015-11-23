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
#ifndef __SBRIDGE_CONFIG_H__
#define __SBRIDGE_CONFIG_H__

#include <sqlite3.h>
#include "sbridge_chan.h"

#define SBRIDGE_MAX_CONFIG_NAME_LEN 255
#define SBRIDGE_MAX_HOST_NAME_LEN 255
#define SBRIDGE_MAX_TRACE_PREFIX 255
#define SBRIDGE_MAX_SPANS 32
#define SBRIDGE_MAX_CHANS_PER_SPAN 32
#define SBRIDGE_MAX_HOSTS 10

/* wait up to 10 minutes before enabling the host again */
#define SBRIDGE_HOST_DISABLE_TIME 600

typedef struct sbridge_span_s {
	int32_t spanno;
	int32_t dchan;
	int32_t pridebug;
	int32_t numchans;
	sangoma_pri_switch_t switch_type;
	sangoma_pri_node_t node_type;
	sangoma_pri_t spri;
	sbridge_thread_t signaling_thread;
	sbridge_bool_t need_hangup;
	sbridge_chan_t channels[SBRIDGE_MAX_CHANS_PER_SPAN];
	struct sbridge_config_s *config;
} sbridge_span_t;

typedef struct sbridge_host_entry_s {
	sbridge_spinlock_t lock;
	char name[SBRIDGE_MAX_HOST_NAME_LEN];
	short port;
	sbridge_bool_t disabled;
	sbridge_time_t enable_time;
} sbridge_host_entry_t;

typedef struct sbridge_config_s {
	short numhosts;
	short v110_urate;
	int32_t loglevel;
	int32_t numspans;
	sqlite3 *db;
	sbridge_host_entry_t tcp_hosts[SBRIDGE_MAX_HOSTS];
	char trace_prefix[SBRIDGE_MAX_TRACE_PREFIX];
	sbridge_span_t spans[SBRIDGE_MAX_SPANS];
} sbridge_config_t;

typedef struct sbridge_command_options_s {
	char configname[SBRIDGE_MAX_CONFIG_NAME_LEN];
} sbridge_command_options_t;

/*!
 * \brief Parses the given configuration file and fills the configuration structure
 * \param Configuration file path
 * \param Configuration structure to fill
 * \return Return true on success or false on failure 
 */
sbridge_bool_t sbridge_config_parse(sbridge_config_t *configuration, const char *configuration_file_name);

/*!
 * \brief Parses the given command line options and fills the given structure
 * \param argc count of arguments
 * \param argv array of arguments
 * \param options command options structure to fill
 * \return Return true on success or false on failure 
 */
sbridge_bool_t sbridge_parse_arguments(int argc, char *argv[], sbridge_command_options_t *options);

#endif

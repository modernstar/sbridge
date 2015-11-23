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

#include <sqlite3.h>
#include "sbridge_os.h"
#include "sbridge_config.h"
#include "sbridge_log.h"
#include "sbridge_v110.h"

static sangoma_pri_node_t str_to_node(const char *str)
{
	sangoma_pri_node_t node_type = SANGOMA_PRI_CPE;
	return node_type;
}

static sangoma_pri_switch_t str_to_switch(const char *str)
{
	sangoma_pri_switch_t switch_type = SANGOMA_PRI_SWITCH_EUROISDN_E1;
	return switch_type;
}

static sbridge_log_level_t str_to_loglevel(const char *str)
{
	sbridge_log_level_t loglevel;
	if (!str) {
		/* if no loglevel, assume notice */
		loglevel = SBRIDGE_LOG_NOTICE;
	} else if (!strncasecmp(str, "EXDEBUG", sizeof("EXDEBUG")-1)) {
		loglevel = SBRIDGE_LOG_EXDEBUG;
	} else if (!strncasecmp(str, "DEBUG", sizeof("DEBUG")-1)) {
		loglevel = SBRIDGE_LOG_DEBUG;
	} else if (!strncasecmp(str, "NOTICE", sizeof("NOTICE")-1)) {
		loglevel = SBRIDGE_LOG_NOTICE;
	} else if (!strncasecmp(str, "WARNING", sizeof("WARNING")-1)) {
		loglevel = SBRIDGE_LOG_WARNING;
	} else if (!strncasecmp(str, "ERROR", sizeof("ERROR")-1)) {
		loglevel = SBRIDGE_LOG_ERROR;
	} else {
		sbridge_log(SBRIDGE_LOG_WARNING, "Invalid log level specified '%s', defaulting to NOTICE\n", str);
		loglevel = SBRIDGE_LOG_NOTICE;
	}
	return loglevel;
}

static short str_to_v110_urate(const char *str)
{
	short urate = URATE_9600;
	return urate;
}

static sbridge_bool_t create_hosts(const char *hosts, sbridge_config_t *configuration)
{
	char *host;
	char *hostsdup = NULL;
	char *portstr;
	short port, numhosts, maxhosts;
	hostsdup = strdupa(hosts);
	maxhosts = sizeof(configuration->tcp_hosts)/sizeof(configuration->tcp_hosts[0]);
	numhosts = 0;
	host = strsep(&hostsdup, ",");
	while (host && strlen(host) && numhosts < maxhosts) {
		portstr = strstr(host, ":");
		if (!portstr) {
			sbridge_log(SBRIDGE_LOG_ERROR, "Ignoring host %s with no port specification\n", host);
		} else {
			*portstr = '\0';
			portstr++;
			port = atoi(portstr);
			if (!port) {
				sbridge_log(SBRIDGE_LOG_ERROR, "Ignoring host %s with invalid port specification %s\n", host, portstr);
			} else {
				sbridge_snprintf(configuration->tcp_hosts[numhosts].name, 
						sizeof(configuration->tcp_hosts[numhosts].name), "%s", host);
				configuration->tcp_hosts[numhosts].port = port;
				configuration->tcp_hosts[numhosts].disabled = SBRIDGE_FALSE;
				sbridge_spin_initialize(&configuration->tcp_hosts[numhosts].lock);
				sbridge_log(SBRIDGE_LOG_NOTICE, "Added %s:%d to the list of hosts\n", host, port);
				numhosts++;
			}
		}
		host = strsep(&hostsdup, ",");
	}
	if (!numhosts) {
		sbridge_log(SBRIDGE_LOG_ERROR, "No hosts specified!\n");
		return SBRIDGE_FALSE;
	}
	configuration->numhosts = numhosts;
	return SBRIDGE_TRUE;
}

sbridge_bool_t sbridge_config_parse(sbridge_config_t *configuration, const char *configuration_file_name)
{
	const char *envstr = NULL;
	char *spansdup = NULL;
	char *spanstr = NULL;
	int32_t conf_dchan;
	int32_t conf_spanchans;
	int32_t conf_pridebug;
	int32_t spanno, maxspans, res;
	sangoma_pri_switch_t conf_switch;
	sangoma_pri_node_t conf_node;

	sbridge_assert(configuration != NULL, -1);
	sbridge_assert(configuration_file_name != NULL, -1);

	memset(configuration, 0, sizeof(*configuration));

	/* if its ever needed, a configuration file can be used here, for now environment variables work just fine */
	envstr = sbridge_getenv("sbridge_database");
	if (!envstr) {
		sbridge_log(SBRIDGE_LOG_ERROR, "sbridge_database not specified\n");
		return SBRIDGE_FALSE;
	}
	res = sqlite3_open(envstr, &configuration->db);
	if (res) {
		sbridge_log(SBRIDGE_LOG_ERROR, "failed to open sqlite3 database %s: %s\n", envstr, sqlite3_errmsg(configuration->db));
		sqlite3_close(configuration->db);
		return SBRIDGE_FALSE;
	}

	envstr = sbridge_getenv("sbridge_tcp_hosts");
	if (!envstr) {
		sbridge_log(SBRIDGE_LOG_ERROR, "sbridge_tcp_hosts not specified\n");
		return SBRIDGE_FALSE;
	}

	if (!create_hosts(envstr, configuration)) {
		sbridge_log(SBRIDGE_LOG_ERROR, "Failed to create hosts from string %s\n", envstr);
		return SBRIDGE_FALSE;
	}

	envstr = sbridge_getenv("sbridge_loglevel");
	configuration->loglevel = str_to_loglevel(envstr);
	if (configuration->loglevel == SBRIDGE_LOG_EXDEBUG) {
		sbridge_log(SBRIDGE_LOG_NOTICE, "PRI debugging will be enabled\n");
		conf_pridebug = PRI_DEBUG_ALL;
	} else {
		conf_pridebug = 0;
	}

	envstr = sbridge_getenv("sbridge_v110_rate");
	configuration->v110_urate = str_to_v110_urate(envstr);

	envstr = sbridge_getenv("sbridge_trace");
	if (envstr) {
		sbridge_snprintf(configuration->trace_prefix, sizeof(configuration->trace_prefix), "%s", envstr);
	}

	/* D-channel number */
	conf_dchan = 16; 

	/* channels per span */
	conf_spanchans = 31;

	/* PRI switch type */
	envstr = sbridge_getenv("sbridge_pri_switch");
	conf_switch = str_to_switch(envstr);

	/* PRI node type */
	envstr = sbridge_getenv("sbridge_pri_node");
	conf_node = str_to_node(envstr);

	envstr = sbridge_getenv("sbridge_pri_spans");
	if (!envstr) {
		sbridge_log(SBRIDGE_LOG_ERROR, "sbridge_spans was not specified\n");
		return SBRIDGE_FALSE;
	}

	spansdup = strdupa(envstr);
	spanstr = strsep(&spansdup, ",");
	maxspans = sizeof(configuration->spans)/sizeof(configuration->spans[0]);
	while (spanstr && strlen(spanstr) && configuration->numspans < maxspans) {
		spanno = atoi(spanstr);
		if (!spanno) {
			sbridge_log(SBRIDGE_LOG_ERROR, "Invalid span specified: %s\n", spanstr);
			return SBRIDGE_FALSE;
		}
		configuration->spans[configuration->numspans].spanno = spanno;
		configuration->spans[configuration->numspans].numchans = conf_spanchans;
		configuration->spans[configuration->numspans].dchan = conf_dchan;
		configuration->spans[configuration->numspans].pridebug = conf_pridebug;
		configuration->spans[configuration->numspans].switch_type = conf_switch;
		configuration->spans[configuration->numspans].node_type = conf_node;
		configuration->spans[configuration->numspans].signaling_thread = SBRIDGE_INVALID_THREAD_ID;
		configuration->spans[configuration->numspans].config = configuration;
		configuration->spans[configuration->numspans].need_hangup = SBRIDGE_FALSE;
		configuration->numspans++;
		spanstr = strsep(&spansdup, ",");
	}

	return SBRIDGE_TRUE;
}

sbridge_bool_t sbridge_parse_arguments(int argc, char *argv[], sbridge_command_options_t *options)
{
	memset(options, 0, sizeof(*options));
	/* not used for now, dummy config name */
	sbridge_snprintf(options->configname, sizeof(*options->configname), "%s", "sbridge.conf");
	return SBRIDGE_TRUE;
}



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

/*! \brief Operating system abstracted calls 
 * code in this file is *ALLOWED* to be operating system specific.
 * since this is the only file where ifdef __linux__ or similar
 * code is allowed. At this point only Linux is supported. Supporting
 * other operating systems (Windows, BSD etc) should be fairly easy
 * just implementing the functions on this file
 * */
#include "sbridge_log.h"
#include "sbridge_os.h"

const char *sbridge_os_get_last_error(void)
{
#ifdef __linux__
	return strerror(errno);
#endif
}

sbridge_bool_t sbridge_thread_setcancelstate(sbridge_thread_cancel_state_t cancelstate, sbridge_thread_cancel_state_t *oldcancelstate)
{
#ifdef __linux__
	if (pthread_setcanceltype(cancelstate, (int *)oldcancelstate)) {
		return SBRIDGE_FALSE;
	}
	return SBRIDGE_TRUE;
#endif
}

sbridge_bool_t sbridge_thread_setcanceltype(sbridge_thread_cancel_type_t canceltype, sbridge_thread_cancel_type_t *oldcanceltype)
{
#ifdef __linux__
	if (pthread_setcanceltype(canceltype, (int *)oldcanceltype)) {
		return SBRIDGE_FALSE;
	}
	return SBRIDGE_TRUE;
#endif
}

void sbridge_thread_testcancel(void)
{
#ifdef __linux__
	pthread_testcancel();
#endif
}

sbridge_bool_t sbridge_thread_notify(sbridge_thread_t tid, int sig)
{
#ifdef __linux__
	if (pthread_kill(tid, sig)) {
		return SBRIDGE_FALSE;
	}
	return SBRIDGE_TRUE;
#endif
}

void sbridge_thread_exit(void *exit_code)
{
	pthread_exit(exit_code);
}

sbridge_bool_t sbridge_thread_create_foreground(sbridge_thread_t *tid, sbridge_thread_execute_function_t thread_function, void *args)
{
#ifdef __linux__
	pthread_attr_t thread_attr;

	if (pthread_attr_init(&thread_attr)) {
		return SBRIDGE_FALSE;
	}
	if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE)) {
		return SBRIDGE_FALSE;
	}

	if (pthread_create(tid, &thread_attr, thread_function, args)) {
		return SBRIDGE_FALSE;
	}

	pthread_attr_destroy(&thread_attr);
	return SBRIDGE_TRUE;
#endif
}

sbridge_bool_t sbridge_thread_cancel(sbridge_thread_t tid)
{
#ifdef __linux__
	if (pthread_cancel(tid)) {
		return SBRIDGE_FALSE;
	}
	return SBRIDGE_TRUE;
#endif
}

sbridge_bool_t sbridge_thread_join(sbridge_thread_t tid, void **return_code)
{
#ifdef __linux__
	if (pthread_join(tid, return_code)) {
		return SBRIDGE_FALSE;
	}
	return SBRIDGE_TRUE;
#endif
}


sbridge_bool_t sbridge_mutex_lock(sbridge_mutex_t *mutex)
{
#ifdef __linux__
	if (pthread_mutex_lock(mutex)) {
		return SBRIDGE_FALSE;
	}
	return SBRIDGE_TRUE;
#endif
}

sbridge_bool_t sbridge_mutex_unlock(sbridge_mutex_t *mutex)
{
#ifdef __linux__
	if (pthread_mutex_unlock(mutex)) {
		return SBRIDGE_FALSE;
	}
	return SBRIDGE_TRUE;
#endif
}

sbridge_bool_t sbridge_spin_lock(sbridge_spinlock_t *mutex)
{
#ifdef __linux__
	if (pthread_spin_lock(mutex)) {
		return SBRIDGE_FALSE;
	}
	return SBRIDGE_TRUE;
#endif
}

sbridge_bool_t sbridge_spin_unlock(sbridge_spinlock_t *mutex)
{
#ifdef __linux__
	if (pthread_spin_unlock(mutex)) {
		return SBRIDGE_FALSE;
	}
	return SBRIDGE_TRUE;
#endif
}

sbridge_bool_t sbridge_spin_initialize(sbridge_spinlock_t *mutex)
{
#ifdef __linux__
	if (pthread_spin_init(mutex, PTHREAD_PROCESS_PRIVATE)) {
		return SBRIDGE_FALSE;
	}
	return SBRIDGE_TRUE;
#endif
}

sbridge_bool_t sbridge_mutex_initialize(sbridge_mutex_t *mutex)
{
#ifdef __linux__
	if (pthread_mutex_init(mutex, NULL)) {
		return SBRIDGE_FALSE;
	}
	return SBRIDGE_TRUE;
#endif
}

sbridge_bool_t sbridge_gettimeofday(sbridge_timeval_t *timeval)
{
	sbridge_assert(timeval != NULL, SBRIDGE_FALSE);
#ifdef __linux__

	if (gettimeofday(timeval, NULL)) {
		return SBRIDGE_FALSE;
	}
	return SBRIDGE_TRUE;
#endif

}

sbridge_time_t sbridge_time(sbridge_time_t *out_time)
{
#ifdef __linux__
	sbridge_time_t currtime = -1;
	if ((sbridge_time_t)-1 == (currtime = time(out_time))) {
		return (sbridge_time_t)-1;
	}
	return currtime;
#endif
}

sbridge_tm *sbridge_localtime_r(sbridge_time_t *time, sbridge_tm *tm)
{
#ifdef __linux__
	return localtime_r(time, tm);	
#endif
}

sbridge_bool_t sbridge_daemonize(void)
{
#ifdef __linux__
	/* not thread-safe but no one should be stupid enough to call this function twice anyways, let alone
	 * doing it after launching threads
	 * */
	static int daemonized = 0;
	sbridge_assert(daemonized != 1, SBRIDGE_FALSE);

	pid_t pid = fork();
	if (pid < 0) {
		return SBRIDGE_FALSE;
	}
	if (pid > 0) {
		/* Parent can die now */
		exit(0);
	}

	/* I'm the child */
	umask(0);
	daemonized = 1;

	/* get our own process group to be the leaders */
	setsid();

	chdir("/");

	freopen("/dev/null", "r", stdin);
	freopen("/var/log/sbridge.log", "w", stdout);
	freopen("/var/log/sbridge.log", "w", stderr);
	
	struct rlimit limits;
	limits.rlim_cur = RLIM_INFINITY;
	limits.rlim_max = RLIM_INFINITY;
	if (setrlimit(RLIMIT_CORE, &limits)) {
		return SBRIDGE_FALSE;
	}
	daemonized = 1;
	return SBRIDGE_TRUE;
#endif
}

struct hostent *sbridge_gethostbyname(const char *host, sbridge_hostent_t *hp)
{
#ifdef __linux__
	int res;
	int herrno;
	int dots = 0;
	const char *s;
	struct hostent *result = NULL;
	s = host;
	res = 0;
	while (s && *s) {
		if (*s == '.')
			dots++;
		else if (!isdigit(*s))
			break;
		s++;
	}
	if (!s || !*s) {
		if (dots != 3) {
			return NULL;
		}
		memset(hp, 0, sizeof(*hp));
		hp->hp.h_addrtype = AF_INET;
		hp->hp.h_addr_list = (void *) hp->buf;
		hp->hp.h_addr = hp->buf + sizeof(void *);
		if (inet_pton(AF_INET, host, hp->hp.h_addr) > 0) {
			return &hp->hp;
		}
		return NULL;
		
	}

	res = gethostbyname_r(host, &hp->hp, hp->buf, sizeof(hp->buf), &result, &herrno);

	if (res || !result || !hp->hp.h_addr_list || !hp->hp.h_addr_list[0]) {
		return NULL;
	}
	return &hp->hp;
#endif
}


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

#ifndef __SBRIDGE_OS_H__
#define __SBRIDGE_OS_H__

#ifdef __linux__

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>

/*! \brief general data types */
#define SBRIDGE_FALSE 0
#define SBRIDGE_TRUE 1
typedef uint32_t sbridge_bool_t;

/*! \brief Time data types */
typedef struct timeval sbridge_timeval_t;
typedef time_t sbridge_time_t;
typedef struct tm sbridge_tm;

/*! \brief Thread and synch types */
#define SBRIDGE_INVALID_THREAD_ID 0
#define SBRIDGE_MUTEX_DEFAULT_INITIALIZER PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#define SBRIDGE_THREAD_CLEANUP_EXECUTE 1
#define SBRIDGE_THREAD_CLEANUP_NOT_EXECUTE 0

typedef enum {
	SBRIDGE_THREAD_CANCEL_DISABLE = PTHREAD_CANCEL_DISABLE,
	SBRIDGE_THREAD_CANCEL_ENABLE = PTHREAD_CANCEL_ENABLE
} sbridge_thread_cancel_state_t;

typedef enum {
	SBRIDGE_THREAD_CANCEL_DEFERRED = PTHREAD_CANCEL_DEFERRED,
	SBRIDGE_THREAD_CANCEL_ASYNCHRONOUS = PTHREAD_CANCEL_ASYNCHRONOUS
} sbridge_thread_cancel_type_t;

#define sbridge_thread_self pthread_self
#define sbridge_thread_equal pthread_equal
#define sbridge_thread_cleanup_push pthread_cleanup_push
#define sbridge_thread_cleanup_pop pthread_cleanup_pop
#define sbridge_malloc malloc
#define sbridge_getenv getenv
#define sbridge_calloc calloc
#define sbridge_free free
#define sbridge_atoi atoi
#define sbridge_snprintf snprintf

typedef pthread_t sbridge_thread_t;
typedef pthread_mutex_t sbridge_mutex_t;
typedef pthread_spinlock_t sbridge_spinlock_t;
typedef void *(*sbridge_thread_execute_function_t)(void *args);

/*! \brief I/O types */
#define SBRIDGE_INVALID_FD -1
typedef int32_t sbridge_fd;

/*! \brief general macros */
#ifdef SBRIDGE_DEBUG
#define sbridge_assert(expression, retval) \
	if (!(expression)) { \
		sbridge_log(SBRIDGE_LOG_ERROR, "Assert at %s:%s failed!\n", __FILE__, __LINE__); \
		abort(); \
		return retval; \
	} 
#else
#define sbridge_assert(expression, retval) \
	if (!(expression)) { \
		sbridge_log(SBRIDGE_LOG_ERROR, "Assert at %s:%s failed!\n", __FILE__, __LINE__); \
		return retval; \
	} 
#endif

/*!\brief Network types */
typedef struct sbridge_hostent_s {
	struct hostent hp;
	char buf[1024];
} sbridge_hostent_t;

/*!
 * \brief Get last thread operating system error string
 * \return operating system owned string describing the error
 */
const char *sbridge_os_get_last_error(void);

/*!
 * \brief Sends the given signal to the specified thread
 * \param tid Thread to send the signal to
 * \param sig Signal to deliver
 * \return true on success, false on error
 */
sbridge_bool_t sbridge_thread_notify(sbridge_thread_t tid, int sig);

/*!
 * \brief Set the cancellation state for the current thread
 * \param cancelstate New cancel state
 * \param oldcancelstate Pointer to store the old cancel state or NULL if not needed
 * \return true on success, false on error
 */
sbridge_bool_t sbridge_thread_setcancelstate(sbridge_thread_cancel_state_t cancelstate, sbridge_thread_cancel_state_t *oldcancelstate);

/*!
 * \brief Set the cancellation type for the current thread
 * \param canceltype New cancel type
 * \param oldcanceltype Pointer to store the old cancel type or NULL if not needed
 * \return true on success, false on error
 */
sbridge_bool_t sbridge_thread_setcanceltype(sbridge_thread_cancel_type_t canceltype, sbridge_thread_cancel_type_t *oldcanceltype);

/*!
 * \brief Test for cancellation pending on the thread
 */
void sbridge_thread_testcancel(void);

/*!
 * \brief Test for cancellation pending on the thread
 * \param exit_value Generic pointer to the exit value 
 */
void sbridge_thread_exit(void *exit_value);

/*!
 * \brief Initialize the mutex with default attributes
 * \param mutex Mutex object to initialize
 */
sbridge_bool_t sbridge_mutex_initialize(sbridge_mutex_t *mutex);

/*!
 * \brief Initialize the mutex with default attributes
 * \param mutex Mutex object to initialize
 */
sbridge_bool_t sbridge_spin_initialize(sbridge_spinlock_t *mutex);

/*!
 * \brief Requests cancellation of the given thread
 * \param tid Thread that you want to cancel
 * \return true on success, false on error
 */
sbridge_bool_t sbridge_thread_cancel(sbridge_thread_t tid);

/*!
 * \brief Requests cancellation of the given thread
 * \param tid Thread that you want to join 
 * \param return_code void pointer to the pointer the target thread used as return code
 * \return true on success, false on error
 */
sbridge_bool_t sbridge_thread_join(sbridge_thread_t tid, void **return_code);

/*!
 * \brief Launch a thread that can be cancelled and waited for (in Linux thread terms, Joinable)
 * \param tid Thread ID pointer
 * \param thread_function Function to be used as starting point for the thread
 * \param args Pointer to the generic arguments to be provided to the thread function
 * \return true on success, false on error
 */
sbridge_bool_t sbridge_thread_create_foreground(sbridge_thread_t *tid, sbridge_thread_execute_function_t thread_function, void *args);

/*!
 * \brief Lock the given mutex object
 * \param mutex object to acquire
 * \return true on success, false on error
 */
sbridge_bool_t sbridge_mutex_lock(sbridge_mutex_t *mutex);

/*!
 * \brief Unlock the given mutex object
 * \param mutex object to release 
 * \return true on success, false on error
 */
sbridge_bool_t sbridge_mutex_unlock(sbridge_mutex_t *mutex);

/*!
 * \brief Lock the given mutex object
 * \param mutex object to acquire
 * \return true on success, false on error
 */
sbridge_bool_t sbridge_spin_lock(sbridge_spinlock_t *mutex);

/*!
 * \brief Unlock the given mutex object
 * \param mutex object to release 
 * \return true on success, false on error
 */
sbridge_bool_t sbridge_spin_unlock(sbridge_spinlock_t *mutex);

/*!
 * \brief Unlock the given mutex object
 * \param timeval pointer to sbridge_timeval_t structure to fill
 * \return true on success, false on error
 */
sbridge_bool_t sbridge_gettimeofday(sbridge_timeval_t *timeval);

/*!
 * \brief Gets the epoch time in seconds
 * \param time pointer to sbridge_time_t 
 * \return Epoch time
 */
sbridge_time_t sbridge_time(sbridge_time_t *time);

/*!
 * \brief Converts epoch time to a broken down version of time in sbridge_tm
 * \param time pointer to sbridge_time_t of the time to convert
 * \param time pointer to sbridge_tm structure to fill
 * \return NULL on failure or the sbridge_tm result pointer on success
 */
sbridge_tm *sbridge_localtime_r(sbridge_time_t *time, sbridge_tm *tm);

/*!
 * \brief Get host address by name
 * \param host Host char name
 * \param hp Pointer to a host entry structure sbridge_hostent_t
 * \return NULL on failure or the sbridge_tm result pointer on success
 */
struct hostent *sbridge_gethostbyname(const char *host, sbridge_hostent_t *hp);

/*!
 * \brief Become a proper service process
 * \return true on success or false on failure
 */
sbridge_bool_t sbridge_daemonize(void);

#else

#error "Unsupported platform"

#endif /* ifdef __linux__ */

#endif /* __SBRIDGE_OS_H__ */


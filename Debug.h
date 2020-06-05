/*
 * File:   Debug.h
 * Author: Kai Niessen
 *
 * Created on January 16, 2016, 7:50 PM
 *
 * Contains definitions for tracing, debugging, profiling and logging
 *
 * To enable trace / debug / rpofiling messages, uncomment the associated
 * defines or configure them in your IDE / make file.
 */

#ifndef DEBUG_H
#define DEBUG_H

#undef TRACE
#undef DEBUG
#undef MSG

//#define TRACING
//#define DEBUGGING
//#define PROFILING

#ifdef DEBUGGING
#define DEBUG(x...) fprintf(stderr, __FILE__ " - " x)
#else
#define DEBUG(x...)
#endif // DEBUGGING
#ifdef TRACING
#define TRACE(x...) fprintf(stderr, __FILE__ " - " x)
#else
#define TRACE(x...)
#endif // TRACING
#define MSG(x...) fprintf(stderr, __FILE__ " - " x)
#ifdef PROFILING
#define PROFILE_START bigtime_t timer = system_time()
#define PROFILE_MEASURE(action) \
	TRACE("%s took %2.6f seconds\r\n", action, \
		(system_time() - timer) / 1000000.0f)
#else
#define PROFILE_START
#define PROFILE_MEASURE(action)
#endif // PROFILING

#endif // DEBUG_H

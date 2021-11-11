/*
 * Copyright (C) 2017 Kai Niessen <kai.niessen@online.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>

/*
 * Contains definitions for tracing, debugging, profiling and logging
 *
 * To enable trace / debug / rpofiling messages, uncomment the associated
 * defines or configure them in your IDE / make file.
 */


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


#endif // _DEBUG_H

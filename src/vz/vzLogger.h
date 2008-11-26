/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2008 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2005.

    ViZualizator is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    ViZualizator is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ViZualizator; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

ChangeLog:
    2008-09-24:
        *inherite logger code from 'cmapdb-agents' project
*/


#ifndef VZLOGGER_H
#define VZLOGGER_H


#ifdef VZLOGGER_EXPORTS
#define VZLOGGER_API __declspec(dllexport)
#else
#define VZLOGGER_API __declspec(dllimport)
#pragma comment(lib, "vzLogger.lib") 
#endif

/* do it for remote trace ops */
/* #define ENABLE_TRACE_POINT */

#ifndef ENABLE_TRACE_POINT
#define TRACE_POINT()
#else /* ENABLE_TRACE_POINT */
#define TRACE_POINT() logger_printf(2, "TRACE:%s:%d:[%s]", __FILE__, __LINE__, __FUNCTION__);
#endif /* ENABLE_TRACE_POINT */


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 * Initialize logger module
 *
 * @param[in] log_file filename of log file where datas will written to
 * @param[in] rotate_interval value in seconds after log file will be renamed
 * with datestamp and new file created.
 *
 * @return 0 on success, negative value otherwise
 */
VZLOGGER_API int logger_setup(char* log_file, int rotate_interval, int dup_to_stderr);

/**
 * Output log message
 *
 * @param[in] type 0-NOTICE,1-ERROR,2-WARNING
 * @param[in] message message string formatted in printf style, if additional
 * argument required, these submitted throw the list
 *
 * @return 0 on success, negative value otherwise
 */
VZLOGGER_API int logger_printf(int type, char* message, ...);

/**
 * Enable output to console
 */
VZLOGGER_API int logger_dup_to_console();

/**
 * init logger datas and start thread
 */
VZLOGGER_API int logger_init();

/**
 * stop logger output thread and realse some datas
 */
VZLOGGER_API int logger_release();

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif

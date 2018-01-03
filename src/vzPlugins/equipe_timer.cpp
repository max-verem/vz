/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2005 Maksym Veremeyenko.
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

ChangleLog:
	2006-12-14:
		*constructor updates, added two parameters 'scene' 'parent_container'

	2005-10-02:
		*new version.


Protocol description:

1. RS-232/RS-422 port is used to recieve command (timestamps)
    BaudRate = (UINT) CBR_38400
    ByteSize = (BYTE) 8
    Parity = (BYTE) ODDPARITY
    StopBits = (BYTE) ONESTOPBIT

2. Command structure:
Command has 6 bytes length

   5X      YY YY YY YY   ZZ
--------   -----------  ----
STX/DESC   TIMESTAMP     CS

BYTE #0  - START/DESC
bit 7-4: 0101 - start header, constant
bit 3: rised if timer is running
bit 2: rised if timer is counting down
bit 1-0: timer id (0..3)

BYTE #1..#4 - TIMESTAMP
unsigned 32-bit value of timer, LSB first (little endian presentation of 32-bit value)

BYTE #5 - control sum
arithmetic sum of unsigned bytes #0..#4 
*/

static char* _plugin_description = 
""
;

static char* _plugin_notes = 
""
;

#define _CRT_SECURE_NO_WARNINGS

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"
#include "../vz/vzMain.h"

#include <time.h>
#include <stdio.h>
#include "do_format_string.h"

// declare name and version of plugin
DEFINE_PLUGIN_INFO_EXT("equipe_timer","NONSTD");

#define MAX_TIMERS 4
#define MAX_BUFFER_SIZE 128
#define EQUIPE_CMD_LEN 6
static char* working_param = "s_text";
static char* default_str_format = "%Y-%m-%d %H:%M:%S";

struct timers_description
{
	long value;
	long running;
	long countdown;
};

static struct timers_description timers[MAX_TIMERS];
static HANDLE timers_lock = INVALID_HANDLE_VALUE;
static HANDLE timers_reader = INVALID_HANDLE_VALUE;
static long timers_reader_exit = 0;
static char* default_format_string = "HH:MM:SS:FF(.DDD)";

static unsigned long WINAPI timers_reader_proc(void* data)
{
	char* serial_port_name = (char*)data;

	logger_printf(0, DEBUG_LINE_ARG "Starting 'timers_reader_proc' for port '%s'", DEBUG_LINE_PARAM, serial_port_name);

	/* open port */
	HANDLE com_port = CreateFile
	(
		serial_port_name,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	/* check port opens */
	if (com_port == INVALID_HANDLE_VALUE)
	{
		logger_printf(0, DEBUG_LINE_ARG " ERROR opening port '%s'", DEBUG_LINE_PARAM, serial_port_name);
		return 0;
	};

	/* configure */
	DCB lpdcb;
	memset(&lpdcb, 0, sizeof(DCB));
	lpdcb.BaudRate = (UINT) CBR_38400;		// 38400 b/s
											// a. 1 start bit ( space )
	lpdcb.ByteSize = (BYTE) 8;				// b. 8 data bits
	lpdcb.Parity = (BYTE) ODDPARITY;		// c. 1 parity bit (odd)
	lpdcb.StopBits = (BYTE) ONESTOPBIT;		// d. 1 stop bit (mark)
	lpdcb.fParity = 1;
	lpdcb.fBinary = 1;
	if (!(SetCommState(com_port, &lpdcb)))
	{
		logger_printf(1, DEBUG_LINE_ARG " ERROR configuring port '%s'", DEBUG_LINE_PARAM, serial_port_name);
		CloseHandle(com_port);
		return 0;
	};

	/* instance an object of COMMTIMEOUTS. */
    COMMTIMEOUTS comTimeOut;                   
    comTimeOut.ReadIntervalTimeout = /* MAXDWORD; */ 15;
    comTimeOut.ReadTotalTimeoutMultiplier = /* MAXDWORD; // */  10;
    comTimeOut.ReadTotalTimeoutConstant = 1000;
    comTimeOut.WriteTotalTimeoutMultiplier = MAXDWORD; //3;
    comTimeOut.WriteTotalTimeoutConstant = MAXDWORD; //2;
	if (!(SetCommTimeouts(com_port,&comTimeOut)))
	{
		logger_printf(1, DEBUG_LINE_ARG " ERROR in 'SetCommTimeouts' for port '%s'", DEBUG_LINE_PARAM, serial_port_name);
		CloseHandle(com_port);
		return 0;
	};

	/* ignore events from port like break... */
	SetCommMask(com_port, 0);

	/* endless loop */
	PurgeComm(com_port, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
	while(!(timers_reader_exit))
	{
		unsigned char cmd_buffer[EQUIPE_CMD_LEN];
		unsigned long read_len;

		/* read data */
//		PurgeComm(com_port, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
		ReadFile(com_port, &cmd_buffer, EQUIPE_CMD_LEN, &read_len, NULL);
		if(EQUIPE_CMD_LEN != read_len)
		{
#ifdef _DEBUG
			logger_printf(1, DEBUG_LINE_ARG " (EQUIPE_CMD_LEN != read_len) - 'read_len=%d'", DEBUG_LINE_PARAM, read_len);
#endif
			PurgeComm(com_port, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
			continue;
		};

		/* check header */
		if (0x50 != (0xF0 & cmd_buffer[0]))
		{
#ifdef _DEBUG
			logger_printf(0, DEBUG_LINE_ARG " (0x50 != (0xF0 & cmd_buffer[0])", DEBUG_LINE_PARAM);
#endif
			PurgeComm(com_port, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
			continue;
		};

		/* checksum */
		unsigned char cs = 0;
		for(int i=0; i< (EQUIPE_CMD_LEN - 1) ; i++) cs += cmd_buffer[i];
		if(cs != cmd_buffer[EQUIPE_CMD_LEN - 1])
		{
#ifdef _DEBUG
			logger_printf(0, DEBUG_LINE_ARG " (cs != cmd_buffer[EQUIPE_CMD_LEN - 1])", DEBUG_LINE_PARAM);
#endif
			PurgeComm(com_port, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
			continue;
		};

		/* fill data */
		struct timers_description timer;
		timer.running = (cmd_buffer[0] & (1<<3))?1:0;
		timer.countdown = (cmd_buffer[0] & (1<<2))?1:0;
		timer.value = *((long*)(&cmd_buffer[1]));

		/* set data */
		WaitForSingleObject(timers_lock, INFINITE);
		timers[ cmd_buffer[0] & 0x03 ] = timer;
		ReleaseMutex(timers_lock);
	};

	CloseHandle(com_port);
	return 0;
};

BOOL APIENTRY DllMain
(
	HANDLE hModule, 
	DWORD  ul_reason_for_call, 
	LPVOID lpReserved
)
{
	int i;
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:

			/* create locks and clear timers struct */
			timers_lock = CreateMutex(NULL,FALSE,NULL);
			memset(&timers, 0, MAX_TIMERS * sizeof(struct timers_description));

			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			
			/* rise exit flag */
			timers_reader_exit = 1;

			/* wait for 'timers_reader_proc' finish */
			if(INVALID_HANDLE_VALUE != timers_reader)
			{
				WaitForSingleObject(timers_reader, INFINITE);
				CloseHandle(timers_reader);
			};

			// close mutex
			CloseHandle(timers_lock);
    }
    return TRUE;
}

// internal structure of plugin
typedef struct
{
	char* s_format_running;
	char* s_format_stale;
	long l_offset;
	long l_timer_id;

	char* _buffer;
	HANDLE _lock_update;
} vzPluginData;

// default value of structure
static vzPluginData default_value = 
{
	NULL,
	NULL,
	0,
	0,

	NULL,
	INVALID_HANDLE_VALUE
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"s_format_running", "Time format for stale timer", PLUGIN_PARAMETER_OFFSET(default_value,s_format_running)},
	{"s_format_stale", "Time format for running timer", PLUGIN_PARAMETER_OFFSET(default_value,s_format_stale)},
	{"l_offset", "Timer offset (miliseconds)", PLUGIN_PARAMETER_OFFSET(default_value,l_offset)},
	{"l_timer_id", "Timer identificator (0..3)", PLUGIN_PARAMETER_OFFSET(default_value,l_timer_id)},
	{NULL,NULL,0}
};

PLUGIN_EXPORT int load(void* config_ptr)
{
    int r = 0;
	char* serial_port_name;

	/* detect if we need to start async reader */
	if(serial_port_name = vzConfigParam(config_ptr, "equipe_timer", "serial_port_name"))
	{
		/* run async reader */
		unsigned long thread;
		logger_printf(0, DEBUG_LINE_ARG " running 'timers_reader_proc'", DEBUG_LINE_PARAM);
		timers_reader = CreateThread(0, 0, timers_reader_proc, serial_port_name, 0, &thread);
	}
	else
    {
        r = 1;
		logger_printf(1, DEBUG_LINE_ARG " Warning 'serial_port_name' in section 'equipe_timer' not defined!", DEBUG_LINE_PARAM);
    };

    return r;
};


PLUGIN_EXPORT void* constructor(void* scene, void* parent_container)
{
	// init memmory for structure
	vzPluginData* data = (vzPluginData*)malloc(sizeof(vzPluginData));

	// copy default value
	*data = default_value;

	// create mutexes
	_DATA->_lock_update = CreateMutex(NULL,FALSE,NULL);

	// return pointer
	return data;
};

PLUGIN_EXPORT void destructor(void* data)
{
	if (_DATA->_buffer) free(_DATA->_buffer);
	CloseHandle(_DATA->_lock_update);
	free(data);
};

PLUGIN_EXPORT void prerender(void* data,vzRenderSession* session)
{
};

PLUGIN_EXPORT void postrender(void* data,vzRenderSession* session)
{
};

PLUGIN_EXPORT void render(void* data,vzRenderSession* session)
{

};

PLUGIN_EXPORT void notify(void* data, char* param_name)
{

};

PLUGIN_EXPORT long datasource(void* data,vzRenderSession* render_session, long index, char** name, char** value)
{
	/* we configure only one parameter */
	if(index >= 1)
		return 0;

	/* allocate buffer */
	if (_DATA->_buffer == NULL)
		_DATA->_buffer = (char*)malloc(MAX_BUFFER_SIZE);

	/* check if timer id is in range */
	if(_DATA->l_timer_id >= MAX_TIMERS)
		return 0;

	/* create a local copy of timer struct */
	WaitForSingleObject(timers_lock, INFINITE);
	struct timers_description timer = timers[_DATA->l_timer_id];
	ReleaseMutex(timers_lock);

	/* fix timer value if process is running */
	if(timer.running)
	{
		/* fix timer */
		if(timer.countdown)
			timer.value -= _DATA->l_offset;
		else
			timer.value += _DATA->l_offset;

		/* check if timer outof range */
		if(timer.value < 0) timer.value = 0;
	};

	/* allocate tmp buffer */
	char* tmp;
	memset(tmp = (char*)malloc(MAX_BUFFER_SIZE), 0, MAX_BUFFER_SIZE);

	/* calc value to submit */
	WaitForSingleObject(_DATA->_lock_update,INFINITE);
	do_format_string
	(
		timer.value,
		(timer.running)
		?
		( (_DATA->s_format_running)?_DATA->s_format_running:default_format_string )
		:
		( (_DATA->s_format_stale)?_DATA->s_format_stale:default_format_string )
		,
		tmp
	);
	ReleaseMutex(_DATA->_lock_update);

	/* compare prev string and current */
	if(0 == strcmp(tmp, _DATA->_buffer))
	{
		free(tmp);
		return 0;
	}

	/* exchange buffer */
	free(_DATA->_buffer);
	_DATA->_buffer = tmp;
	*value = tmp;
	*name = working_param;
	return 1;
};

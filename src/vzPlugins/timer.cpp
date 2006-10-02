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
	2005-09-29: 
		*new version introducing 

*/

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"

#include <stdio.h>
#pragma comment(lib, "winmm.lib")

#define _ABS(V) ( (V<0)?(-(V)):V )
#define _SIGN(A,B) ( (A>B)?(-1):1 )
#define MAX_BUF_LEN 1024

// declare name and version of plugin
PLUGIN_EXPORT vzPluginInfo info =
{
	"timer",
	1.0,
	"rc7"
};

static char* working_param = "s_text";

// internal structure of plugin
typedef struct
{
	char* s_format;
	long l_trig_start;
	long l_trig_stop;
	long l_trig_reset;
	long l_trig_cont;
	long l_start_value;
	long l_stop_value;

	long _value;
	long _running;
	long _prev;
	char *_buffer;
	HANDLE _lock_update;
} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
	NULL,
	0,
	0,
	0,
	0,
	0,
	1<<30,

	0,
	0,
	0,
	NULL,
	INVALID_HANDLE_VALUE
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"s_format", "Timer format", PLUGIN_PARAMETER_OFFSET(default_value,s_format)},
	{"l_trig_start", "Start counting trigger", PLUGIN_PARAMETER_OFFSET(default_value,l_trig_start)},
	{"l_trig_stop", "Stop counting trigger", PLUGIN_PARAMETER_OFFSET(default_value,l_trig_stop)},
	{"l_trig_reset", "Reset values counting trigger", PLUGIN_PARAMETER_OFFSET(default_value,l_trig_reset)},
	{"l_trig_cont", "Continue counting trigger", PLUGIN_PARAMETER_OFFSET(default_value,l_trig_cont)},
	{"l_start_value", "Reset values counting trigger", PLUGIN_PARAMETER_OFFSET(default_value,l_start_value)},
	{"l_stop_value", "Continue counting trigger", PLUGIN_PARAMETER_OFFSET(default_value,l_stop_value)},
	{NULL,NULL,0}
};

static char* default_format_string = "HH:MM:SS:FF(.DDD)";

static void do_format_string(long value, char* f, char* o)
{
	int l = 0;

	/* parser */
	while(*f)
	{
		switch(*f)
		{
			case 'H':
			case 'M':
			case 'S':
			case 'F':
			case 'D':
				/* our char - increment length*/
				l++;
				/* check if future character is not the same */
				if((*f) != (*(f + 1)))
				{
					char local_format[128];
					char local_data[128];
					long local_value;

					/* create format string */
					sprintf(local_format, "%%%d.%dd", l, l);

					/* determinate value by format */
					switch(*f)
					{
						case 'H':
							local_value = (value)/1000/3600;
							break;
						case 'M':
							local_value = (value)/1000/60;
							local_value = local_value % 60;
							break;
						case 'S':
							local_value = (value)/1000;
							local_value = local_value % 60;
							break;
						case 'F':
							local_value = (value % 1000) / 40;
							break;
						case 'D':
							switch(l)
							{
								case 1:
									local_value = (value % 1000) / 100;
									break;
								case 2:
									local_value = (value % 1000) / 10;
									break;
								default:
									local_value = (value % 1000);
									break;
							};
							break;
					};

					/* create new value */
					sprintf(local_data, local_format, local_value);

					/* append */
					strcat(o, local_data);
					o += strlen(local_data);

					/* reset counter */
					l = 0;
				};
				break;
			default:
				/* just append to main string */
				*o = *f; 
				o++;
				break;
		};
		/* next char */
		f++;
	};
	/* terminate string */
	*o = 0;
};


PLUGIN_EXPORT void* constructor(void)
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
	// try to lock struct
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	
	long d = _DATA->l_start_value - _DATA->l_stop_value; d = _ABS(d);
	if(_DATA->_running)
	{
		/* count value */
		long t = timeGetTime();
		_DATA->_value += (t - _DATA->_prev);
		_DATA->_prev = t;

		/* detect if we need to stop */
		if(d < _DATA->_value)
		{
			_DATA->_running = 0;
			_DATA->_value = d;
		};
	};
	// unlock
	ReleaseMutex(_DATA->_lock_update);
};

PLUGIN_EXPORT void render(void* data,vzRenderSession* session)
{

};

PLUGIN_EXPORT void notify(void* data)
{
	// try to lock struct
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	/* start processing */
	if((_DATA->l_trig_start) && (!(_DATA->_running)))
	{
		_DATA->_value = 0;
		_DATA->_running = 1;
		_DATA->_prev = timeGetTime();
	}
	_DATA->l_trig_start = 0;

	/* start processing */
	if((_DATA->l_trig_cont) && (!(_DATA->_running)))
	{
		_DATA->_running = 1;
		_DATA->_prev = timeGetTime();

	}
	_DATA->l_trig_cont = 0;

	/* stop processing */
	if((_DATA->l_trig_stop) && (_DATA->_running))
	{
		long tmp = timeGetTime();
		_DATA->_running = 0;
		_DATA->_value += (tmp - _DATA->_prev);
	}
	_DATA->l_trig_stop = 0;


	/* stop processing */
	if(_DATA->l_trig_reset)
	{
		_DATA->_running = 0;
		_DATA->_value = 0;
	}
	_DATA->l_trig_reset = 0;

	// unlock
	ReleaseMutex(_DATA->_lock_update);
};

PLUGIN_EXPORT long datasource(void* data,vzRenderSession* render_session, long index, char** name, char** value)
{
	/* check params count */
	if(index != 0) return 0;

	/* prepare buffer */
	if (_DATA->_buffer == NULL)
		memset(_DATA->_buffer = (char*)malloc(MAX_BUF_LEN), 0, MAX_BUF_LEN);

	/* allocate tmp buffer */
	char* tmp;
	memset(tmp = (char*)malloc(MAX_BUF_LEN), 0, MAX_BUF_LEN);

	/* calc value to submit */
	WaitForSingleObject(_DATA->_lock_update,INFINITE);
	do_format_string
	(
		_DATA->l_start_value + _SIGN(_DATA->l_start_value, _DATA->l_stop_value)*_DATA->_value,
		(_DATA->s_format)?_DATA->s_format:default_format_string,
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

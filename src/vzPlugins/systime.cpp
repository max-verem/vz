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
		*additional lock
		*tzset() to setup timezone info

	2005-09-28:
		*new version.

*/

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"

#include <time.h>

// declare name and version of plugin
PLUGIN_EXPORT vzPluginInfo info =
{
	"systime",
	1.0,
	"rc8"
};

static char* working_param = "s_text";
#define MAX_BUFFER_SIZE 128
static char* default_str_format = "%Y-%m-%d %H:%M:%S";

// internal structure of plugin
typedef struct
{
	long l_offset;
	long l_UTC;
	long l_PM;
	char* s_format;
	time_t _prev;
	char* _buffer;
	HANDLE _lock_update;
} vzPluginData;

// default value of structore
static vzPluginData default_value = 
{
	0,
	0,
	0,
	NULL,
	0,
	NULL,
	INVALID_HANDLE_VALUE
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"s_format", "Time format (see 'strftime' manual for details)", PLUGIN_PARAMETER_OFFSET(default_value,s_format)},
	{"l_offset", "Offset in seconds from current time", PLUGIN_PARAMETER_OFFSET(default_value,l_offset)},
	{"l_PM", "Flag to append 'PM' or 'AM' suffixes", PLUGIN_PARAMETER_OFFSET(default_value,l_PM)},
	{"l_UTC", "Use UTC time", PLUGIN_PARAMETER_OFFSET(default_value,l_UTC)},
	{NULL,NULL,0}
};


PLUGIN_EXPORT void* constructor(void)
{
	// init memmory for structure
	vzPluginData* data = (vzPluginData*)malloc(sizeof(vzPluginData));

	// copy default value
	*data = default_value;

	/* setup timezone */
	tzset();

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

PLUGIN_EXPORT void notify(void* data)
{

};

PLUGIN_EXPORT long datasource(void* data,vzRenderSession* render_session, long index, char** name, char** value)
{
	time_t ltime;
	struct tm *rtime;
	
	/* we configure only one parameter */
	if(index >= 1)
		return 0;

	/* allocate buffer */
	if (_DATA->_buffer == NULL)
		_DATA->_buffer = (char*)malloc(MAX_BUFFER_SIZE);

	/* request time */
	time( &ltime );

	/* fix offset */
	ltime += _DATA->l_offset;

	/* check if time changed */
	if(_DATA->_prev == ltime)
		return 0;
	else
		_DATA->_prev = ltime;

	/* covert time */
	if(_DATA->l_UTC)
		/* UTC. */
		rtime = gmtime( &ltime );
	else
		/* local */
		rtime = localtime( &ltime );

	/* expand in text */
	WaitForSingleObject(_DATA->_lock_update,INFINITE);
	strftime( _DATA->_buffer , MAX_BUFFER_SIZE - 5, (_DATA->s_format)?_DATA->s_format:default_str_format , rtime );
	ReleaseMutex(_DATA->_lock_update);

	/* append text */
	if (_DATA->l_PM)
	{
		if(rtime->tm_hour > 12)
			strcat(_DATA->_buffer, "PM");
		else
			strcat(_DATA->_buffer, "AM");
	};
	
	/* return values */
	*name = working_param;
	*value = _DATA->_buffer;
	return 1;
};

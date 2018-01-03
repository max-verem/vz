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

	2006-11-21:
		*new version started

*/
static char* _plugin_description = 
"This plugin perform datasource interface for precomputated static data "
"placed in text file. Each colunm (splited by TAB) is values of "
"parameters. Mapping parameters to column number performed by plugin "
"parameter 's_map', file name of data submitted throw 's_filename' "
"parameter."
;

static char* _plugin_notes = 
""
;

#define _CRT_SECURE_NO_WARNINGS

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"

#pragma comment(lib, "winmm.lib")

#include "trajectory.h"

// declare name and version of plugin
DEFINE_PLUGIN_INFO("trajectory_table");

// internal structure of plugin
typedef struct
{
	/* public params */
	char* s_filename;
	char* s_map;
	long l_index;
	
	/* internal state params */
	char* _s_filename;
	char* _s_map;
	char** _map;
	long _map_len;
	char*** _table;
	long _table_len;

	HANDLE _lock_update;
} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
	/* public params */
	NULL,					// char* s_filename;
	NULL,					// char* s_map;
	0,						// long l_index;
	
	/* internal state params */
	NULL,					// char* _s_filename;
	NULL,					// char* _s_map;
	NULL,					// char** _map;
	0,						// long _map_len;
	NULL,					// char*** _table;
	0,						// long _table_len;

	INVALID_HANDLE_VALUE	// HANDLE _lock_update;
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"s_filename", "Name of text file with table", PLUGIN_PARAMETER_OFFSET(default_value,s_filename)},
	{"s_map", "Mapping of parameters to col number in text file (e.g 0:f_x;1:f_y)", PLUGIN_PARAMETER_OFFSET(default_value,s_map)},
	{"l_index", "Index of table (number of row).", PLUGIN_PARAMETER_OFFSET(default_value,l_index)},
	{NULL,NULL,0}
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
	/* delete map */
	free_map(_DATA->_map);

	/* delete table */
	free_table(_DATA->_table);

	/* delete locks */
	CloseHandle(_DATA->_lock_update);

	/* free struct description */
	free(data);
};

PLUGIN_EXPORT void prerender(void* data,vzRenderSession* session)
{
};

PLUGIN_EXPORT void postrender(void* data,vzRenderSession* session)
{
	// try to lock struct
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	// unlock
	ReleaseMutex(_DATA->_lock_update);
};

PLUGIN_EXPORT void render(void* data,vzRenderSession* session)
{

};


PLUGIN_EXPORT void notify(void* data, char* param_name)
{
	// try to lock struct
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	/* new table given */
	if(_DATA->_s_filename != _DATA->s_filename)
	{
		/* free datas */
		if(_DATA->_table)
			free_table(_DATA->_table);

		/* init */
		_DATA->_table = parse_table(_DATA->s_filename, &_DATA->_table_len);

		/* sync */
		_DATA->_s_filename = _DATA->s_filename;

		logger_printf(0, "trajectory_table: table '%s' has %d rows", _DATA->s_filename, _DATA->_table_len);
	};


	/* new map given */
	if(_DATA->_s_map != _DATA->s_map)
	{
		/* free datas */
		if(_DATA->_map)
			free_map(_DATA->_map);

		/* init */
		_DATA->_map = parse_map(_DATA->s_map, &_DATA->_map_len);

		/* sync */
		_DATA->_s_map = _DATA->s_map;

		/* verbose */
		logger_printf(0, "trajectory_table: map '%s' has %d pairs", _DATA->s_map, _DATA->_map_len);
	};

	// unlock
	ReleaseMutex(_DATA->_lock_update);
};

PLUGIN_EXPORT long datasource(void* data,vzRenderSession* render_session, long index, char** name, char** value)
{
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	/* check params count */
	if
	(
		(index <0)
		||
		(index >= _DATA->_map_len)
		||
		(0 == _DATA->_table_len)
	)
	{
		ReleaseMutex(_DATA->_lock_update);
		return 0;
	};

	/* normalize index */
	if(_DATA->l_index >= _DATA->_table_len)
		_DATA->l_index = (_DATA->_table_len - 1);
	else if(_DATA->l_index < 0)
		_DATA->l_index = 0;

	/* assign name */
	*name = _DATA->_map[index*2 + 1];

	/* assign value */
	if
	(
		NULL 
		== 
		(
			*value = lookup_table_value
			(
				*name, 
				_DATA->l_index, 
				_DATA->_map, 
				_DATA->_table, 
				_DATA->_table_len
			)
		)
	)
		*value = "";

	ReleaseMutex(_DATA->_lock_update);

	return 1;
};

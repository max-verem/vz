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

ChangeLog:
    2008-09-24:
        *logger use for message outputs

    2005-06-08: Code cleanup

*/

#include "memleakcheck.h"

#include "vzFunctions.h"
#include "plugin.h"
#include "vzLogger.h"

#include <io.h>
#include <stdio.h>

vzFunctions::vzFunctions(void* config) : vzHash<vzFunction*>() 
{
	_config = config;
};

vzFunctions::~vzFunctions()
{
	// delete vzFunction objects
	if(count())
		for(unsigned int i=0;i<count();i++)
			delete value(i);
};

void vzFunctions::load()
{
	// look into catalogue where plugins stored
	// and load in
	
	struct _finddata_t fd;
	long hfile = _findfirst( PLUGIN_PATH "*" PLUGIN_EXT,  &fd);
	if (hfile != -1L)
	{
		do
		{
			// prepare file name
			char function_file[1024];
			sprintf(function_file, PLUGIN_PATH "%s",fd.name);

			logger_printf(0, "Loading function file: '%s'... ", function_file);
			// init function object
			vzFunction* function = new vzFunction();
			// load
			char* function_name = function->load(_config,function_file);
			// check loaded
			if(function_name)
			{
				// store
				push(function_name,function);
				logger_printf
				(
					0,
					"Loaded function file: '%s' (%s-%s)",
					function_file,
					function->info()->name,
					function->info()->version
				);
			}
			else
			{
				// wrong dll? - delete object
				delete function;
				logger_printf(1, "Failed to load: '%s'... ", function_file);
			};

		}
		while (_findnext(hfile, &fd) == 0);
		_findclose(hfile);
	}
}

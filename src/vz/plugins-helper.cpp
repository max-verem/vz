/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2007 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1.tv, 2007.

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
	2007-11-18:
		*VS2005 migrations patches
*/
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>

#include "plugin.h"
#include <windows.h>


void usage()
{
	fprintf
	(
		stderr,
		"Usage: vzPluginsMan.exe <plugin file> [<plugin info file>]\n"
	);
};


int main(int argc, char** argv)
{
	vzPluginInfo* info;
	vzPluginParameter* parameters;
	HINSTANCE lib;
	FILE* f;
	int r = 0, i;

	/* check arguments */
	if((3 != argc) && (2 != argc))
	{
		fprintf(stderr, "Error! Not enough arguments\n");
		usage();
		exit(-1);
	}
	else
	{
		if (2 == argc)
			f = stdout;
		else
		{
			if(NULL == (f = fopen(argv[2], "wt")))
			{
				fprintf(stderr, "Error! Unable to create '%s'\n", argv[2]);
				usage();
				exit(-1);
			};
		};
	};


	/* try to load lib */
	if(NULL == (lib = LoadLibrary(argv[1])))
	{
		fclose(f);
		fprintf(stderr, "Error! Unable to load file '%s'.\n", argv[1]);
		exit(-1);
	};

	/* open info */
	if(NULL != (info = (vzPluginInfo*)GetProcAddress(lib,"info")))
	{
		if(NULL != (parameters = (vzPluginParameter*)GetProcAddress(lib,"parameters")))
		{
			/* list info*/
			fprintf
			(
				f,
				"FUNCTION:\n    %s\n\nVERSION:\n    %s\n\nDESCRIPTION:\n    %s\n\nNOTES:\n    %s\n\n",
				info->name,
				info->version,
				info->description,
				info->notes
			);

			/* list parameters */
			fprintf
			(
				f,
				"PARAMETRS:\n\n"
			);
			for(i=0;(parameters[i].name);i++)
			{
				fprintf
				(
					f,
					"    '%s' - %s\n\n",
					parameters[i].name,
					parameters[i].title
				);
			};
		}
		else
		{
			r = -1;
			fprintf(stderr, "Error! No exported PARAMETERS by '%s'.\n", argv[1]);
		};
	}
	else
	{
		r = -1;
		fprintf(stderr, "Error! No exported INFO by '%s'.\n", argv[1]);
	};

	FreeLibrary(lib);
	fclose(f);
	return r;
};

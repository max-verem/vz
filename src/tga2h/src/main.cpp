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

Changelog:
	2007-11-16: 
		*Visual Studio 2005 migration.
*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>

#include "vzImage.h"

#define COLUMNS 5

static void usage()
{
	fprintf
	(
		stderr,
		"Usage:\n"
		"    tga2h.exe <tga file> <header file> <prefix name>\n"
		"Where:\n"
		"    <tga file>     - Input 32bit TGA format file.\n"
		"    <header file>  - Output header file name.\n"
		"    <prefix name>  - Prefix of constant.\n"
	);

}


int main(int argc, char** argv)
{
	vzImage* image;
	char* e;
	FILE* f;

	/* check args */
	if(argc != 4)
	{
		fprintf
		(
			stderr, 
			"ERROR! Not enough arguments.\n"
		);
		usage();
		exit(-1);
	};

	char 
		*filename_tga = argv[1],
		*filename_h = argv[2],
		*const_prefix = argv[3]
		;


	/* Image file */
	if(NULL == (image = vzImageLoadTGA(filename_tga, &e)))
	{
		/* notify */
		fprintf(stderr, "ERROR! Unable to open file (%s)\n", e);
		exit(-1);
	};

	/* Create header file */
	if(NULL == (f = fopen(filename_h, "wt")))
	{
		fprintf(stderr, "ERROR! Unable to create file '%s'\n", filename_h);
		vzImageFree(image);
		exit(-1);
	};

	/* start */
	fprintf
	(
		f,
		"static long %s_width = %d;\n"
		"static long %s_height = %d;\n"
		"static unsigned long %s_surface_temp[] =\n"
		"{",
		const_prefix, image->width,
		const_prefix, image->height,
		const_prefix
	);

	for(int j = 0; j < (image->width*image->height); j++)
	{
		if(!(j % COLUMNS)) fprintf(f, "\n\t");
		fprintf(f, " 0x%.8X", ((unsigned long*)image->surface)[j]);
		if( (j + 1) != (image->width*image->height) ) fprintf(f, ",");
	};

	/* stop */
	fprintf
	(
		f,
		"\n};\n"
		"static void *%s_surface = %s_surface_temp;\n",
		const_prefix, const_prefix
	);


	fclose(f);
	vzImageFree(image);

	return 0;
};
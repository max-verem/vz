/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2011 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2011.

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
*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>

#include "../../vz/vzImage.h"

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
	FILE* f;
    int r;

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
    r = vzImageLoad(&image, filename_tga);
	if(r)
	{
		/* notify */
		fprintf(stderr, "ERROR! Unable to open file [%s], r=%d\n", filename_tga, r);
		exit(-1);
	};

	/* Create header file */
	if(NULL == (f = fopen(filename_h, "wt")))
	{
		fprintf(stderr, "ERROR! Unable to create file '%s'\n", filename_h);
		vzImageRelease(&image);
		exit(-1);
	};

    /* start */
    fprintf
    (
        f,
        "#ifndef %s_h\n"
        "#define %s_h\n"
        "static int _load_img_%s(vzImage* img)\n"
        "{\n"
        "    static const unsigned long surface[] =\n"
        "    {\n",
        const_prefix, const_prefix, const_prefix
    );

    for(int j = 0; j < (image->width*image->height); j++)
    {
        if(!(j % COLUMNS)) fprintf(f, "\n        ");
        fprintf(f, " 0x%.8X", ((unsigned long*)image->surface)[j]);
        if( (j + 1) != (image->width*image->height) ) fprintf(f, ",");
    };

    /* stop */
    fprintf
    (
        f,
        "\n"
        "    };\n"
        "\n"
    );

    fprintf(f, "    memset(img, 0, sizeof(vzImage));\n");
    fprintf(f, "    img->width = %d;\n", image->width);
    fprintf(f, "    img->height = %d;\n", image->height);
    fprintf(f, "    img->base_width = %d;\n", image->base_width);
    fprintf(f, "    img->base_height = %d;\n", image->base_height);
    fprintf(f, "    img->line_size = %d;\n", image->line_size);
    fprintf(f, "    img->bpp = %d;\n", image->bpp);
    fprintf(f, "    img->pix_fmt = %d;\n", image->pix_fmt);
    fprintf(f, "    img->surface = (void*)surface;\n");

    fprintf
    (
        f,
        "\n"
        "    return 0;\n"
        "};\n"
        "#endif\n"
    );

	fclose(f);
	vzImageRelease(&image);

	return 0;
};
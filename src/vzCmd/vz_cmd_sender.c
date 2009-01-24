/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2009 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2009.

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
	2009-01-24:
		*start
*/
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "vz_cmd_send.h"

#ifdef __linux__
#define PROGRAM_NAME "vz_cmd_sender"
#define PROGRAM_VER "-r" SVNVERSION
#else /* __linux__ */
#define PROGRAM_NAME "vzCmdSender"
#define PROGRAM_VER " build on [" __DATE__ " " __TIME__ "]"
#endif /* __linux__ */

#define MSG_PREFIX PROGRAM_NAME ": "


#define PROGRAM_USAGE \
"Usage:\n" \
"    %s  <host[:port]> <CMD> [<CMD> ... ]\n" \
"Where:\n" \
"    <host[:port]        - hostname where command to send\n" \
"    <CMD>               - command, see control_udp.txt for more info\n"

static void usage()
{
    fprintf(stderr, PROGRAM_USAGE, PROGRAM_NAME);
};

static void info()
{
	/* output info */
    fprintf(stderr, PROGRAM_NAME PROGRAM_VER " Copyright by Maksym Veremeyenko, 2009\n");
};

int main(int argc, char** argv)
{
	int r;
	char error[1024] = "";

	/* check if all arguments added */
	if(argc < 3)
	{
		info();
		fprintf(stderr, MSG_PREFIX "ERROR! Not enough arguments\n");
		usage();
		return 1;
	};

	/* init sender */
	r = vz_cmd_send_init();
	if (0 != r)
	{
		info();
		fprintf(stderr, MSG_PREFIX "vz_cmd_send_init FAILED with code %d\n", -r);
		return 1;
	};

	/* transform symbols */
	r = vz_cmd_send_strlist_udp(argv[1], &argv[2], argc - 2, error);
	if (0 != r)
	{
		info();
		fprintf(stderr, MSG_PREFIX "vz_cmd_send_strlist_udp FAILED with code %d, [%s]\n", r, error);
	};


	/* release sender */
	r = vz_cmd_send_release();

	return 0;
};

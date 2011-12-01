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
	2006-12-13:
		*Added parameter container to plugin session, some code formatting

    2005-06-08:
		*Code cleanup

*/


// define plugin exports types

#ifndef PLUGIN_H
#define PLUGIN_H

#include "memleakcheck.h"

#define PLUGIN_PATH ".\\plugins\\"
#define PLUGIN_EXT ".dll"

typedef struct
{
	float f_alpha;
	long frame;
	long field;
	long fill;
	long key;
	long order; /* second bit specifies INTERLACED flag, first bit FIRST FIELD */
	void* container;
} vzRenderSession;

typedef struct
{
	char* name;
	char* version;
	char* description;
	char* notes;
} vzPluginInfo;

typedef struct
{
	char* name;
	char* title;
	unsigned long offset;
} vzPluginParameter;

#define FOURCC_TO_LONG(ARG_0,ARG_1,ARG_2,ARG_3) \
( \
	((long)ARG_0) \
	| \
	((long)ARG_1) << 8 \
	| \
	((long)ARG_2) << 16 \
	| \
	((long)ARG_3) << 24 \
)

// left-top
#define GEOM_CENTER_LT FOURCC_TO_LONG('_','L','T','_')
// center-top
#define GEOM_CENTER_CT FOURCC_TO_LONG('_','C','T','_')
// right-top
#define GEOM_CENTER_RT FOURCC_TO_LONG('_','R','T','_')

// left-middle
#define GEOM_CENTER_LM FOURCC_TO_LONG('_','L','M','_')
// center-middle
#define GEOM_CENTER_CM FOURCC_TO_LONG('_','C','M','_')
// right-middle
#define GEOM_CENTER_RM FOURCC_TO_LONG('_','R','M','_')

// left-bottom
#define GEOM_CENTER_LB FOURCC_TO_LONG('_','L','B','_')
// center-bottom
#define GEOM_CENTER_CB FOURCC_TO_LONG('_','C','B','_')
// right-bottom
#define GEOM_CENTER_RB FOURCC_TO_LONG('_','R','B','_')

#endif
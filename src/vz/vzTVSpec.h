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
	2008-09-23:
		*vzTVSpec rework

    2005-06-08:
		*Base parameters in struct that describe dims and time of TV frame.
		Adopting for multiple tv system required variable for width, height
		and frame duration.

*/

#ifndef VZTVSPEC_H
#define VZTVSPEC_H

struct _vzTVSpecSA
{
	int x[2];
	int y[2];
};

struct _vzTVSpec
{
	char* NAME;					/** mode name */

	/* geom */
	long TV_FRAME_HEIGHT;	// frame height (px)
	long TV_FRAME_WIDTH;	// frame width (px)

	/* progressive interlaced */
	long TV_FRAME_INTERLACED;	/** interlaced module should used */
	long TV_FRAME_1ST;			/** first field in frame */

	/* frame rate */
	long TV_FRAME_PS_NOM;		/** frame per sec nominator (chislitel) */
	long TV_FRAME_PS_DEN;		/** frame per sec nominator (znamenatel) */

	/* display/pixel aspect ratios */
	long TV_FRAME_DAR_H;		/** display aspect ratio (16:, 4:)*/
	long TV_FRAME_DAR_V;		/** display aspect ratio (:9, :3)*/

	long TV_FRAME_PAR_H;		/** pixel aspect ratio */
	long TV_FRAME_PAR_V;		/** pixel aspect ratio */

	/* safe area */
	struct _vzTVSpecSA* sa;
};
typedef struct _vzTVSpec vzTVSpec;

#endif

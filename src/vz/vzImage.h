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
    2005-06-08: Code cleanup

*/


#ifndef VZIMAGE_H
#define VZIMAGE_H

#include "memleakcheck.h"

#ifdef VZIMAGE_EXPORTS
#define VZIMAGE_API __declspec(dllexport)
#else
#define VZIMAGE_API __declspec(dllimport)
#pragma comment(lib, "vzImage.lib") 
#endif

typedef struct vzImageDesc
{
    long width;
	long height;
    void *surface;
	long base_x;
	long base_y;
	long base_width;
	long base_height;

    /** array of lines pointers */
    void** lines_ptr;

    /** line size in bytes */
    int line_size;

    /** bytes per pixel */
    int bpp;

    /** pixel format, see VZIMAGE_PIXFMT_XXX */
    int pix_fmt;

    /** sys_id */
    long long sys_id;
} vzImage;

#define VZIMAGE_PIXFMT_BGR      1
#define VZIMAGE_PIXFMT_BGRA     2
#define VZIMAGE_PIXFMT_RGB      3
#define VZIMAGE_PIXFMT_GRAY     4
#define VZIMAGE_PIXFMT_RGBA     5
#define VZIMAGE_PIXFMT_RGBX     6
#define VZIMAGE_PIXFMT_BGRX     7
#define VZIMAGE_PIXFMT_XBGR     8

#define VZIMAGE_ALIGN_LINE      4

VZIMAGE_API vzImage* vzImageNewFromVB(long width = 720, long height = 576);
VZIMAGE_API int vzImageSaveTGA(char* filename, vzImage* vzimage, char** error_log, int flipped = 1);

/**
 * Expand image canvas to PowerOfTwo sizes
 *
 * @param[in] img pinter to vzImage struct pointer
 *
 * @return 0 on success, otherwise negative number
 */
VZIMAGE_API int vzImageExpandPOT(vzImage** img);

/**
 * Flip Image vertically
 *
 * @param[in] img pinter to vzImage struct
 *
 * @return 0 on success, otherwise negative number
 */
VZIMAGE_API int vzImageFlipVertical(vzImage* image);

/**
 * Create Image descriptive struct
 *
 * @param[in] img pinter to vzImage struct pointer variable where create struct will be saved
 * @param[in] width image width
 * @param[in] height image height
 * @param[in] pix_fmt image pixel format
 *
 * @return 0 on success, otherwise negative number
 */
VZIMAGE_API int vzImageCreate(vzImage** img, int width, int height, long pix_fmt = VZIMAGE_PIXFMT_BGRA);

/**
 * Free mem of image struct and surface.
 *
 * @param[in] img pinter to vzImage struct pointer variable where create struct will be saved
 *
 * @return 0 on success, otherwise negative number
 */
VZIMAGE_API int vzImageRelease(vzImage** img);

/**
 * Load image from file.
 *
 * @param[in] img pinter to vzImage struct pointer variable where create struct will be saved
 * @param[in] pix_fmt image desired pixel format for loaded image
 *
 * @return 0 on success, otherwise negative number
 */
VZIMAGE_API int vzImageLoad(vzImage** img, char* filename, long pix_fmt = VZIMAGE_PIXFMT_BGRA);

/**
 * Convert (map) pixel format type to OpenGL texture type
 *
 * @param[in] pix_fmt pixel format
 *
 * @return GL_XXX constant, otherwise zero
 */
VZIMAGE_API int vzImagePixFmt2OGL(int pix_fmt);

/**
 * Convert image to disired pixel format
 *
 * @param[in] img pinter to vzImage struct
 * @param[in] pix_fmt target pixel format
 *
 * @return 0 on success, otherwise negative number
 */
VZIMAGE_API int vzImagePixFmtConv(vzImage** img, int pix_fmt);

#endif

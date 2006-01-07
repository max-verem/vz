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

#ifdef VZIMAGE_EXPORTS
#define VZIMAGE_API __declspec(dllexport)
#else
#define VZIMAGE_API __declspec(dllimport)
#pragma comment(lib, "vzImage.lib") 
#endif

typedef struct 
{
    long width;
	long height;
	int surface_type;
    void *surface;
	long base_x;
	long base_y;
	long base_width;
	long base_height;
} vzImage;

VZIMAGE_API void vzImageFree(vzImage* image);
VZIMAGE_API vzImage* vzImageLoadTGA(char* filename, char** error_log = 0);
VZIMAGE_API vzImage* vzImageNewFromVB(long width = 720, long height = 576);
VZIMAGE_API int vzImageSaveTGA(char* filename, vzImage* vzimage, char** error_log, int flipped = 1);
VZIMAGE_API void vzImageBGRA2YUAYVA(void* src, void* dst_yuv, void* dst_alpha, long count);
VZIMAGE_API void vzImageBGRA2YUAYVA_0(vzImage* image,void* yuv,void* alpha);
VZIMAGE_API vzImage* vzImageNew(int width,int height, long surface_size = 0);
VZIMAGE_API void vzImageFlipVertical(vzImage* image);
VZIMAGE_API vzImage* vzImageExpand2X(vzImage* src);

// local procs
#ifdef VZIMAGE_EXPORTS
#endif

/*
VZAUX_API void blend(vzAUX_RGBImageRec* bottom, vzAUX_RGBImageRec* top,long x, long y);
VZAUX_API bool auxTGAImageSave(vzAUX_RGBImageRec* image_record,char* filename);
VZAUX_API vzAUX_RGBImageRec* auxTGAImageLoad(char* filename);
*/

#endif
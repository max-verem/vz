/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2005 Maksym Veremeyenko.
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
#ifndef VZOUTPUT_H
#define VZOUTPUT_H

#include "memleakcheck.h"

#include "../vz/vzImage.h"
#include "../vz/vzTVSpec.h"

#ifdef VZOUTPUT_EXPORTS
#define VZOUTPUT_API __declspec(dllexport)
#else
#define VZOUTPUT_API __declspec(dllimport)
#pragma comment(lib, "vzOutput.lib") 
#endif

/** initialize output engine and load/attach listed modules */
VZOUTPUT_API int vzOutputNew(void** obj, void* config, char* names, vzTVSpec* tv);

/** release output engine */
VZOUTPUT_API int vzOutputFree(void** obj);

/**  */
VZOUTPUT_API int vzOutputInit(void* obj, HANDLE sync_event, unsigned long* sync_cnt);
VZOUTPUT_API int vzOutputRelease(void* obj);

/** prepare OpenGL buffers download */
VZOUTPUT_API int vzOutputPostRender(void* obj);

/** finish OpenGL buffers download */
VZOUTPUT_API int vzOutputPreRender(void* obj);

/** run output */
VZOUTPUT_API int vzOutputRun(void* obj);

/** release output */
VZOUTPUT_API int vzOutputStop(void* obj);

/** retrieve output image */
VZOUTPUT_API int vzOutputOutGet(void* obj, vzImage* img);

/** release ouput image */
VZOUTPUT_API int vzOutputOutRel(void* obj, vzImage* img);

/** */
VZOUTPUT_API int vzOutputRenderSlots(void* obj);

#endif
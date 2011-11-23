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
	2007-11-16: 
		*Visual Studio 2005 migration.

	2006-11-26:
		*Hard sync scheme

	2005-06-28:
		*TBC operation modifications.

	2005-06-25:
		*Modified TBC scheme.

    2005-06-08:
		*Code cleanup
		*Added 'tv' parameter to contructor call
		*Added private parameter _tv (vzTVSpec) for describing TV spec selected

*/


#ifndef VZOUTPUTINTERNAL_H
#define VZOUTPUTINTERNAL_H

#include "memleakcheck.h"

#ifdef VZOUTPUT_EXPORTS
#define VZOUTPUT_API __declspec(dllexport)
#else
#define VZOUTPUT_API __declspec(dllimport)
#pragma comment(lib, "vzOutput.lib") 
#endif

#include <windows.h>

#include "../vz/vzImage.h"
#include "../vz/vzTVSpec.h"

#define DMA_PAGE_SIZE 4096

typedef long (*output_proc_vzOutput_FindBoard)(char** error_log /* = NULL */);
typedef long (*output_proc_vzOutput_SelectBoard)(unsigned long id,char** error_log /* = NULL */);
typedef long (*output_proc_vzOutput_InitBoard)(void*);
typedef void (*output_proc_vzOutput_StartOutputLoop)(void*);
typedef void (*output_proc_vzOutput_StopOutputLoop)(void);
typedef void (*output_proc_vzOutput_SetConfig)(void*);
typedef long (*output_proc_vzOutput_SetSync)(void*);
typedef void (*output_proc_vzOutput_GetBuffersInfo)(struct vzOutputBuffers *b);
typedef void (*output_proc_vzOutput_AssignBuffers)(struct vzOutputBuffers *b);

typedef void (*frames_counter_proc)();


class VZOUTPUT_API vzOutput
{
	// functions pointers
	output_proc_vzOutput_FindBoard			FindBoard;
	output_proc_vzOutput_SelectBoard		SelectBoard;
	output_proc_vzOutput_InitBoard			InitBoard;
	output_proc_vzOutput_StartOutputLoop	StartOutputLoop;
	output_proc_vzOutput_StopOutputLoop		StopOutputLoop;
	output_proc_vzOutput_SetConfig			SetConfig;
	output_proc_vzOutput_SetSync			SetSync;
	output_proc_vzOutput_GetBuffersInfo		GetBuffersInfo;
	output_proc_vzOutput_AssignBuffers		AssignBuffers;

	// tv spec
	vzTVSpec* _tv;

	// library handle
	HINSTANCE _lib;

	// config
	void* _config;

	/* buffers data */
	struct vzOutputBuffers _buffers;

	/* internal aux method */
	void post_render_aux();

public:
	vzOutput(void* config,char* name, void* tv);
	~vzOutput();

	inline int ready(){return (_lib)?1:0;};

	long set_sync_proc(void* fc);

	void notify_frame_start();
	void notify_frame_stop();
	void notify_field0_start();
	void notify_field0_stop();
	void notify_field1_start();
	void notify_field1_stop();

	/* renderer interface */
	void post_render(void);
	void pre_render(void);
	int render_slots();

	/* output driver interface for output buffer */
	void lock_io_bufs(int* buf_idx);
	void unlock_io_bufs();
};

#endif //VZOUTPUTINTERNAL_H

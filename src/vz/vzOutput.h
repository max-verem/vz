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
	2006-11-26:
		*Hard sync scheme.

	2005-06-25:
		*Modified TBC's scheme.
		Added methods:
			'vzOutputInitBuffers' - initialization of the 3-frame TBC
			'vzOuputPostRender' - notify render of frame started
			'vzOuputPreRender' - notity render of frame compleeted
			

    2005-06-08:
		*Code cleanup
		*Modified 'vzOutputNew' to accept additional parameter 'tv'.

*/


#ifndef VZOUTPUT_H
#define VZOUTPUT_H


#ifdef VZOUTPUT_EXPORTS
#define VZOUTPUT_API __declspec(dllexport)
#else
#define VZOUTPUT_API __declspec(dllimport)
#pragma comment(lib, "vzOutput.lib") 
#endif

typedef void (*frames_counter_proc)();

VZOUTPUT_API void* vzOutputNew(void* config, char* name, void* tv);
VZOUTPUT_API void vzOutputFree(void* &obj);
VZOUTPUT_API int vzOutputReady(void* obj);

VZOUTPUT_API int vzOutputSync(void* obj,void* fc);

VZOUTPUT_API void vzOuputPostRender(void* obj);
VZOUTPUT_API void vzOuputPreRender(void* obj);

#endif
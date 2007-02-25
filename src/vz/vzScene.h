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
	2005-06-13:
		*Added parameter for selecting field-based or frame-based rendering mode.

    2005-06-08:
		*Code cleanup

*/


#ifndef VZSCENE_H
#define VZSCENE_H

class vzContainer;
class vzContainerFunction;
class vzMotion;

#include "vzFunctions.h"
#include "vzContainer.h"
#include "vzContainerFunction.h"
#include "vzMotion.h"
#include "vzMotionTimeline.h"
#include "vzMotionControlKey.h"
#include "vzConfig.h"
#include "vzTVSpec.h"

#include "../templates/hash.hpp"

class vzScene
{
	vzContainer* _tree;
	vzMotion* _motion;
	vzFunctions* _functions;

	vzHash<vzContainerFunction*> _id_functions;
	vzHash<vzContainerFunction*> _id_datasources;
	vzHash<vzContainer*> _id_containers;
	vzHash<vzMotionDirector*> _id_directors;
	vzHash<vzMotionTimeline*> _id_timelines;
	vzHash<vzMotionControlKey*> _id_controlkeys;
	// locks
	HANDLE _lock_for_command;

	// config
	vzConfig* _config;
	vzTVSpec* _tv;

	// OpenGL render specifics
	long _blend_dst_alpha;
	long _enable_glBlendFuncSeparateEXT;
	long _enable_GL_SRC_ALPHA_SATURATE;

	// use fields based
	long _fields;
	long _stencil_done;

	void draw(long frame,long field,long fill,long key,long order);

public:
	vzScene(vzFunctions* functions,void* config,vzTVSpec* tv);
	int load(char* file_name);
	void display(long frame);
	~vzScene();

	void* get_ided_object(char* name, char** name_local);

	// we need an a absolute access
	// register 'identified' objects
	inline void register_container(char* id,vzContainer* container){_id_containers.push(id,container);};
	inline void register_function(char* id,vzContainerFunction* function){_id_functions.push(id,function);};
	inline void register_datasource(char* id,vzContainerFunction* function){_id_datasources.push(id,function);};
	inline void register_timeline(char* id,vzMotionTimeline* timeline){_id_timelines.push(id,timeline);};
	inline void register_director(char* id,vzMotionDirector* director){_id_directors.push(id,director);};
	inline void register_controlkey(char* id,vzMotionControlKey* control){_id_controlkeys.push(id,control);};

	// processor method describing
	int command(char* cmd,char** error_log);
	int command(int cmd, int index, void* buf);

#ifdef _DEBUG
	void list_registred();
#endif
};

#endif
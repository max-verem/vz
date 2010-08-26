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
    2008-09-24:
        *logger use for message outputs

	2008-09-23:
		*vzTVSpec rework

	2007-11-16: 
		*Visual Studio 2005 migration.

	2007-02-25:
		*interlaced stencil buffer has new scheme to build.
		*dropped variants about 'blend_dst_alpha', 'enable_glBlendFuncSeparateEXT'.
		GL_SRC_ALPHA_SATURATE options is under investigation.

	2007-02-19:
		*binary coded command execution added.

	2006-11-26:
		OpenGL extension scheme load changes.

	2006-09-30:
		*Some fixes in blending function. Added posibility to define use
		of GL_ONE_MINUS_DST_ALPHA or GL_SRC_ALPHA_SATURATE in function
		'glBlendFuncSeparateEXT' for destination bases rendering.

	2006-09-27:
		*New stencil control.

	2006-05-01: 
		*_stencil always allocates - one alloc for all tree.

	2005-07-07:
		*No AUXi buffers disallowed - modification of stencil buffer ALWAYS 
		uses AUX0 stencil buffer as base prebuild!
		*Prebuilding of stencil buffer moved to constructor.
		*Base stencil bits:
			0	pixel in FIRST field (for PAL is EVEN, for NTSC os ODD)
			1	pixel in SECOND field (for PAL is ODD, for NTSC is EVEN)
			2-7	available for MASK function

	2005-06-23:
		*Field-based drawing seems to solved. For card that support aux buffer
		stencil is stored in that one. In other case updated each frame from
		main memmory buffer (slowly....). AUX buffers are real-hardware on
		FX5500 (should be tested on FX5300);

	2005-06-20:
		*Fighting with speed of stencil buffer update......

	2005-06-13:
		*Added private attribute '_tv' and parameter to contsructor to sumbit
		tv options;

	2005-06-11:
		*Modified rendering scheme, avoid disabling extension blending 
		function for blend_dst_alpha mode - artifacts will appear!
		*To avoid scene rebuilding added parameter to 'draw()' function
		indicated draw order!

	2005-06-10:
		*Added parameter config constructor,
		*Added processing additional two parameters 'blend_dst_alpha', 
		'enable_glBlendFuncSeparateEXT'
		*Dramaticaly modified rendering scheme. It's 2-pass rendering if
		GL-extentions function 'glBlendFuncSeparateEXT' not supported or
		disabled due to performance issue on some card (seems not true support).
		1-pass rendering is used when 'glBlendFuncSeparateEXT' is supported and
		enabled.
		*Rendering could be used in two modes 'destination alpha blending' and
		'source alpha blending'. First mode should be used if you use external 
		keyer device that accept fill and key from video output adapter.
		*'destination alpha blending' ft. 'glBlendFuncSeparateEXT' NOT TESTED!!!
		due to hardware issuie.

	2006-04-23:
		*XML init section removed

	2005-06-09:
		* disabled fields rendering.... 

    2005-06-08:
		*Code cleanup

*/
#define _CRT_SECURE_NO_WARNINGS

#include "memleakcheck.h"

#include "vzScene.h"
#include "xerces.h"
#include <stdio.h>
#include <windows.h>
#include "vzGlExt.h"
#include "../vzCmd/vz_cmd.h"
#include "vzLogger.h"
#include <stdint.h>

static const XMLCh tag_tree[] = {'t', 'r', 'e', 'e',0};
static const XMLCh tag_motion[] = {'m', 'o', 't', 'i', 'o', 'n',0};

static unsigned int _stencil_tex;

vzScene::vzScene(vzFunctions* functions, void* config, vzTVSpec* tv)
{
	_tv = tv;
	_config = (vzConfig*)config;
	_functions = functions;
	_tree = NULL;
	_motion = NULL;
	_lock_for_command = CreateMutex(NULL,FALSE,NULL);
	_stencil_done = 0;

	// parameters from config
	_enable_GL_SRC_ALPHA_SATURATE = (_config->param("vzMain","enable_GL_SRC_ALPHA_SATURATE"))?1:0;
};


int vzScene::load(char* file_name)
{
	logger_printf(0, "Loading scene '%s'... ",file_name);

	//init parser
	XercesDOMParserX *parser = new XercesDOMParserX;

	logger_printf(0, "Parsing scene '%s'... ",file_name);

    try
    {
        parser->parse(file_name);
    }
    catch (...)
    {
		delete parser;
		logger_printf(1, "Failed to parse scene '%s'",file_name);
		return 0;
    }


	DOMDocumentX* doc = parser->getDocument();
	if(!doc)
	{
		logger_printf(1, "Failed load scene (NO Document)!");
		delete parser;
		return 0;
	};

	DOMElementX* scene = doc->getDocumentElement();

	/* check if scene properly loaded */
	if(!(scene))
	{
		logger_printf(1, "Failed load scene (NO Element)!");
		delete parser;
		return 0;
	};

	DOMNodeListX* scene_components = scene->getChildNodes();

	unsigned int i;

	logger_printf(0, "Looking for tree... ");
	// Find First <tree> and load into container
	for(i=0;(i<scene_components->getLength()) && (_tree == NULL);i++)
	{
		// getting child 
		DOMNodeX* scene_component = scene_components->item(i);

		// checking type
		if(scene_component->getNodeType() !=  DOMNodeX::ELEMENT_NODE)
			continue;

		// check node name
		if (XMLStringX::compareIString(scene_component->getNodeName(),tag_tree) == 0)
		{
			logger_printf(0, "Found containers.... ");
			_tree = new vzContainer(scene_component,_functions,this);
			logger_printf(0, "Containers loaded");
			continue;
		};
	};

	logger_printf(0, "Looking for motion");
	// Find First <motion? and load into _motion
	for(i=0;(i<scene_components->getLength()) && (_motion == NULL);i++)
	{
		// getting child 
		DOMNodeX* scene_component = scene_components->item(i);

		// checking type
		if(scene_component->getNodeType() !=  DOMNodeX::ELEMENT_NODE)
			continue;

		// check node name
		if (XMLStringX::compareIString(scene_component->getNodeName(),tag_motion) == 0)
		{
			logger_printf(0, "Found Motion!!");
			_motion = new vzMotion(scene_component,this);
			logger_printf(0, "Loaded montion");
			continue;
		};
	};

	delete parser;

	logger_printf(0, "Scene loaded OK!");

	return 1;
};

vzScene::~vzScene()
{
	TRACE_POINT();
	CloseHandle(_lock_for_command);
	TRACE_POINT();

	TRACE_POINT();
	if(_tree) delete _tree;
	TRACE_POINT();

	TRACE_POINT();
	if(_motion) delete _motion;
	TRACE_POINT();
};

void vzScene::motion_assign(long frame,long field)
{
    if(_motion)
        _motion->assign(frame, field);
};

void vzScene::display(long frame, long renders_count, vzScene** renderers_list)
{

	WaitForSingleObject(_lock_for_command,INFINITE);


	// Clear The Screen And The Depth Buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glClearColor(0.0, 0.0, 0.0, 0.0);

	// Reset The View
	glLoadIdentity();										

	// disable depth tests - direct order
	glDisable(GL_DEPTH_TEST);

	// enable stencil test
	glEnable(GL_STENCIL_TEST);

	// alpha function always works
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_NOTEQUAL,0);

	/* setup stencil */
	if(0 == _tv->TV_FRAME_INTERLACED)
	{
		/* setup stencil buffer if no fields */
		glStencilMask(0xFF);
		glClearStencil( 0x03 );
		glClear(GL_STENCIL_BUFFER_BIT);
	}
	else
	{
		// setup interlaced stencil buffer
		if(!(_stencil_done))
		{
			glStencilMask(0xFF);
			glClearStencil( 2 - _tv->TV_FRAME_1ST );
			glClear(GL_STENCIL_BUFFER_BIT);

			glStencilFunc(GL_NEVER, 1 + _tv->TV_FRAME_1ST, 3);
			glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);

			unsigned int tex, tex_data = 0xFFFFFFFF;

			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexImage2D
			(
				GL_TEXTURE_2D, 
				0, 
				GL_RGBA8, 
				1, 1, 
				0, 
				GL_BGRA_EXT, GL_UNSIGNED_BYTE, 
				&tex_data
			);

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, tex);

			for(float X = 0.0f, Y = 1.0f , W = 2048.0f, H = 1.0f; Y < _tv->TV_FRAME_HEIGHT; Y += 2)
			{

				// Draw a quad (ie a square)
				glBegin(GL_QUADS);

				glTexCoord2f(0.0f, 0.0f);
				glVertex3f(X, Y , 0.0f);

				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(X, Y + H, 0.0f);

				glTexCoord2f(1.0f, 1.0f);
				glVertex3f(X + W, Y + H, 0.0f);

				glTexCoord2f(1.0f, 0.0f);
				glVertex3f(X + W, Y, 0.0f);

				glEnd(); // Stop drawing QUADS
			};

			glDisable(GL_TEXTURE_2D);

			glDeleteTextures_D(1, &tex);

			_stencil_done = 1;
		};
	};

	// draw fields/frame
	for(int field = 0; field <= _tv->TV_FRAME_INTERLACED; field++)
	{
		// set directors for propper position
        if(renders_count && renderers_list)
        {
            for(int l = 0; l < renders_count; l++)
                if(renderers_list[renders_count - l - 1])
                    renderers_list[renders_count - l - 1]->motion_assign(frame, field);
        }
        else
            motion_assign(frame, field);

		/*
			setup stencil function 
			in field mode drawing allowed in pixel where stincil bit flag
			setted to equal field number. i.e. odd rows has rised bit 0, even
			rised bit 1. Two bits of stencil buffer is used to perform interlaced 
			drawing. For "masking" function its possible to use 6 other bits.
		*/
		glStencilMask(0xFF - 3);
		glClearStencil( 0x0 );
		glClear(GL_STENCIL_BUFFER_BIT);
		// set stencil buffer from memmory
		glStencilFunc(GL_EQUAL, 1<<field, 1<<field);
		glStencilOp(GL_REPLACE,GL_KEEP,GL_KEEP);

		// disable wiriting to stencil buffer
		glStencilMask(0);


		// DRAW FRAME

		/* configure blending function */
		if(!(_enable_GL_SRC_ALPHA_SATURATE))
		{
			/*	BEST #1 */
			glBlendFuncSeparateEXT
			(
				GL_ONE_MINUS_DST_ALPHA,
				GL_DST_ALPHA,
				GL_ONE_MINUS_DST_ALPHA,
				GL_ONE
			);
		}
		else
		{
			/* BEST #2 */
			glBlendFuncSeparateEXT
			(
				GL_ONE_MINUS_DST_ALPHA,
				GL_DST_ALPHA,
				GL_SRC_ALPHA_SATURATE,
				GL_ONE
			);
		};

		/* honor PAR - set H scale */
		glPushMatrix();
		if(!_tv->anamorphic)
			glScalef(((float)_tv->TV_FRAME_PAR_DEN)/_tv->TV_FRAME_PAR_NOM, 1.0f, 1.0f);

		/* draw */
        if(renders_count && renderers_list)
        {
            for(int l = 0; l < renders_count; l++)
                if(renderers_list[renders_count - l - 1])
                    renderers_list[renders_count - l - 1]->draw(frame,field,1,1, 0);
        }
        else
            draw(frame,field,1,1, 0);

		/* honor PAR - restore H scale */
		glPopMatrix();
	};

	// disable some features
	glDisable( GL_STENCIL_TEST );
	glDisable( GL_ALPHA_TEST );

	ReleaseMutex(_lock_for_command);
};

static char* target_id_seps = " ,;:";

void vzScene::draw(long frame,long field,long fill,long key,long order)
{
	// init render session and set defaults
	vzRenderSession _render_session = 
	{
		1.0f,
		frame,
		field,
		fill,
		key,
		order
	};
	
	if(_tree)
	{
		/* datasource deals */
		for(unsigned int d = 0; d <_id_datasources.count(); d++)
		{
			int i = 0, r = 0; 
			char *name;
			char *value;
			char* targets_id_found;
			char* targets_id;
			char* target_id_state;
			char* target_id;

			/* get source function */
			vzContainerFunction* source = _id_datasources.value(d);

			/* find ids list */
			targets_id_found = source->get_datatarget();

			/* check if ids found */
			if(NULL != targets_id_found)
			{
				/* allocate space for string */
				targets_id = (char*)malloc(strlen(targets_id_found) + 1);
				strcpy(targets_id, targets_id_found);

				/* lookup first */
				target_id = strtok_s( targets_id, target_id_seps, &target_id_state);

				/* check if found */
				while(NULL != target_id)
				{
					/* detect target function */
					vzContainerFunction* target = _id_functions.find(target_id);

					/* check if target,source is ok */
					if((target)&&(source))
					{
						/* try to retrive params */
						i = 0;
						while
						(
							r = source->datasource
							(
								&_render_session,
								i,
								&name,
								&value
							)
						)
						{
							i++;
							target->set_data_param_fromtext(name, value);
						};

						/* notify */
//						if(i) target->notify();
					};

					/* lookup next */
					target_id = strtok_s( NULL, target_id_seps, &target_id_state);
				};

				/* free list */
				free(targets_id);
			};
		};

		// if container is initializes - start draw method
		_tree->draw(&_render_session);
	};
};

#ifdef _DEBUG
void vzScene::list_registred()
{
	unsigned int i;

	logger_printf(0, "vzScene: Registred functions instances: ");
	for(i=0;i<_id_functions.count();i++)
		logger_printf(0, "|- '%s'", _id_functions.key(i));

	logger_printf(0, "vzScene: Registred containers instances: ");
	for(i=0;i<_id_containers.count();i++)
		logger_printf(0, "|- '%s'", _id_containers.key(i));

	logger_printf(0, "vzScene: Registred directors instances: ");
	for(i=0;i<_id_directors.count();i++)
		logger_printf(0, "|- '%s'", _id_directors.key(i));

	logger_printf(0, "vzScene: Registred timelines instances: ");
	for(i=0;i<_id_timelines.count();i++)
		logger_printf(0, "|- '%s'", _id_timelines.key(i));
};
#endif

// stupid way !!!!!
const char* _tag_tree_function="tree.function.";
void* vzScene::get_ided_object(char* name, char** name_local)
{
// name = "tree.function.plashka2_translate.f_x"

	char *temp, *temp_funcname;

	// check if it deals with tree
	if(strncmp(name,_tag_tree_function,strlen(_tag_tree_function)) == 0)
	{
		// tree function found!!

		// shift to next word
		name += strlen(_tag_tree_function);

		// find "."
		temp = strstr(name,".");
		// if no "." found - return NULL;
		if (temp == NULL)
			return NULL;

		// copy function name into new var
		strncpy(temp_funcname = (char*)malloc(temp - name + 1), name, temp-name);
		temp_funcname[temp-name] = '\0';

		// try to find it
		vzContainerFunction* temp_func = _id_functions.find(temp_funcname);

		// increment name
		name += temp - name + 1;
		*name_local = name;

		// free temp str
		free(temp_funcname);

		return temp_func;
	};

	return NULL;

};

/*

// function params modification
tree.function.<function_id>.<parameter_name>=<value>;

// container function
tree.container.<container_id>.visible=<value>;

// directors control
tree.motion.director.<director_id>.start([<position>]);
tree.motion.director.<director_id>.stop();
tree.motion.director.<director_id>.continue();
tree.motion.director.<director_id>.reset([<position>]);

// timeline modification
tree.motion.timeline.<timeline_id>.<t1|t2|y1|y2>=<value>;

*/

typedef enum LITERALS
{
	LITERAL_NOT_FOUND = 0,
	TAG_FUNCTION,
	TAG_CONTAINER,
	TAG_DIRECTOR,
	TAG_TIMELINE,
	METHOD_DIR_START,
	METHOD_DIR_STOP,
	METHOD_DIR_RESET,
	METHOD_DIR_CONT,
	TIMELINE_T1,
	TIMELINE_T2,
	TIMELINE_Y1,
	TIMELINE_Y2,
};

static char* literals[] = 
{
	"",
	"tree.function.",
	"tree.container.",
	"tree.motion.director.",
	"tree.motion.timeline.",
	"start(",
	"stop(",
	"reset(",
	"cont(",
	"t1=",
	"t2=",
	"y1=",
	"y2="
};

inline LITERALS FIND_FROM_LITERAL(char* &src,LITERALS literal)
{
	if
	(
		0 == strncmp
		(
			src,
			literals[literal],
			strlen(literals[literal])
		)
	)
	{
		src += strlen(literals[literal]);
		return literal;
	}
	else
		return LITERAL_NOT_FOUND;
};

inline void FIND_TO_TERM(char* &src,char* &buf, char* term)
{
	char* temp;

	if(buf)
	{
		free(buf);
		buf = NULL;
	};

	if(temp = strstr(src,term))
	{
		strncpy
		(
			(char*)memset
			(
				buf = (char*)malloc(temp - src + 1),
				0,
				temp - src + 1
			),
			src,
			temp - src
		);
		src = temp + strlen(term);
	};
};

int vzScene::command(char* cmd,char** error_log)
{
	WaitForSingleObject(_lock_for_command,INFINITE);

	char* cmd_p = cmd;
	char* buf = NULL;
	char* temp = NULL;

	if(FIND_FROM_LITERAL(cmd,TAG_FUNCTION))
	{
		// tree.function.<function_id>.<parameter_name>=<value>;

		// find block to point
		FIND_TO_TERM(cmd,buf,".");
		if(buf)
		{
			// id found
			// try to find it
			vzContainerFunction* func = _id_functions.find(buf);
			if(func)
			{
				// function found
				FIND_TO_TERM(cmd,buf,"=");
				if(buf)
				{
					// func - pointer to function
					// buf - parameter name
					// cmd - value
					
					// try to setup param
					if(func->set_data_param_fromtext(buf,cmd))
					{
						*error_log = "OK";
						free(buf);
						ReleaseMutex(_lock_for_command);
						return 1;
					}
					else
					{
						*error_log = "unable to set parameter value (incorrect param value or name)";
					};
				}
				else
				{
					*error_log = "not correct function' param assigment";
				};

			}
			else
			{
				*error_log = "function with specified id not found";
			};
		}
		else
		{
			*error_log = "no function id given";
		};
	}
	else if(FIND_FROM_LITERAL(cmd,TAG_CONTAINER))
	{
		// tree.container.<container_id>.visible=<value>;

		// find block to point
		FIND_TO_TERM(cmd,buf,".");
		if(buf)
		{
			// id found
			// try to find it
			vzContainer* container = _id_containers.find(buf);
			if(container)
			{
				// container found
				// find method
				FIND_TO_TERM(cmd,buf,"=");
				if(buf)
				{
					// method specified

					// buf - property
					// cmd - value
					// container - container

					if(0 == strcmp(buf,"visible"))
					{
						// method visible given
						container->visible(cmd);
						*error_log = "OK";
						free(buf);
						ReleaseMutex(_lock_for_command);
						return 1;
					}
					else
					{
						*error_log = "no such property of container";
					};
				}
				else
				{
					*error_log = "property of container not correctly specified";
				};
			}
			else
			{
				*error_log = "no containers found with specified id";
			};

		}
		else
		{
			*error_log = "not correct container's id";
		};
	}
	else if(FIND_FROM_LITERAL(cmd,TAG_DIRECTOR))
	{
		// tree.motion.director.<director_id>.start();

		// find block to point
		FIND_TO_TERM(cmd,buf,".");
		if(buf)
		{
			// id found
			// try to find it
			vzMotionDirector* director = _id_directors.find(buf);
			if(director)
			{
				// directors id found;
				long method;
				if
				(
					(method = FIND_FROM_LITERAL(cmd,METHOD_DIR_START))
					||
					(method = FIND_FROM_LITERAL(cmd,METHOD_DIR_STOP))
					||
					(method = FIND_FROM_LITERAL(cmd,METHOD_DIR_CONT))
					||
					(method = FIND_FROM_LITERAL(cmd,METHOD_DIR_RESET))
				)
				{
					// start method
					FIND_TO_TERM(cmd,buf,")");
					if(buf)
					{ 
						switch(method)
						{
							case METHOD_DIR_START:
								director->start(buf);*error_log="OK";free(buf);ReleaseMutex(_lock_for_command);return 1;break;
							case METHOD_DIR_STOP:
								director->stop(buf);*error_log="OK";free(buf);ReleaseMutex(_lock_for_command);return 1;break;
							case METHOD_DIR_CONT:
								director->cont(buf);*error_log="OK";free(buf);ReleaseMutex(_lock_for_command);return 1;break;
							case METHOD_DIR_RESET:
								director->reset(buf);*error_log="OK";free(buf);ReleaseMutex(_lock_for_command);return 1;break;
							default:
								*error_log = "strange... expected method not found... ";
								break;
						};
					}
					else
					{
						*error_log = "expected ')' after parameter";
					};
				}
				else
				{
					*error_log = "not method of director";
				};
			}
			else
			{
				*error_log = "director with specified id not found";
			};
		}
		else
		{
			*error_log = "director's id not correctly specified";
		};
	}
	else if(FIND_FROM_LITERAL(cmd,TAG_TIMELINE))
	{
		// tree.motion.timeline.<timeline_id>.<t1|t2|y1|y2>=<value>;

		// find block to point
		FIND_TO_TERM(cmd,buf,".");
		if(buf)
		{
			// id found
			// try to find it
			vzMotionTimeline* timeline = _id_timelines.find(buf);
			if(timeline)
			{
				// timeline found
				long param;
				if
				(
					(param = FIND_FROM_LITERAL(cmd,TIMELINE_T1))
					||
					(param = FIND_FROM_LITERAL(cmd,TIMELINE_T2))
					||
					(param = FIND_FROM_LITERAL(cmd,TIMELINE_Y1))
					||
					(param = FIND_FROM_LITERAL(cmd,TIMELINE_Y2))
				)
				{
					switch(param)
					{
						case TIMELINE_T1:
							timeline->set_t1(buf);*error_log="OK";free(buf);ReleaseMutex(_lock_for_command);return 1;break;
						case TIMELINE_T2:
							timeline->set_t2(buf);*error_log="OK";free(buf);ReleaseMutex(_lock_for_command);return 1;break;
						case TIMELINE_Y1:
							timeline->set_y1(buf);*error_log="OK";free(buf);ReleaseMutex(_lock_for_command);return 1;break;
						case TIMELINE_Y2:
							timeline->set_y2(buf);*error_log="OK";free(buf);ReleaseMutex(_lock_for_command);return 1;break;
						default:
							*error_log = "strange... expected param of timeline not found... ";
							break;
					}
				}
				else
				{
					*error_log = "incorrect param of timeline";
				};

			}
			else
			{
				*error_log = "no such timeline";
			};
		}
		else
		{
			*error_log = "incorrect id syntax for timeline";
		};
	}
	else
	{
		*error_log = "command not recongized";
	};

	if(buf)
		free(buf);

	ReleaseMutex(_lock_for_command);
	return 0;
};

int vzScene::command(int cmd, int index, void* buf)
{
	int r = 0;

	WaitForSingleObject(_lock_for_command,INFINITE);

	switch(cmd)
	{
		/* function field set */
		case VZ_CMD_SET:
		{
			char* function_id;
			char* field_name;
			char* field_value;

			/* map params */
			if
			(
				0 
				==
				vz_serial_cmd_parseNmap
				(
					buf,
					index,
					(void*)&function_id,
					(void*)&field_name,
					(void*)&field_value
				)
			)
			{
				logger_printf(0, "vzScene: VZ_CMD_SET('%s', '%s', '%s')", function_id, field_name, field_value);
				vzContainerFunction* func = _id_functions.find(function_id);
				if(func)
					func->set_data_param_fromtext(field_name, field_value);
				else
					r = -2;
			}
			else
				r = -1;
		};		
		break;

		/* container visibility */
		case VZ_CMD_CONTAINER_VISIBLE:
		{
			char* container_id;
			int32_t* v;

			/* map params */
			if
			(
				0 
				==
				vz_serial_cmd_parseNmap
				(
					buf,
					index,
					(void*)&container_id,
					(void*)&v
				)
			)
			{
				logger_printf(0, "vzScene: VZ_CMD_CONTAINER_VISIBLE('%s', '%d')", container_id, (int)*v);
				vzContainer* container = _id_containers.find(container_id);
				if(container)
					container->visible(*v);
				else
					r = -2;
			}
			else
				r = -1;
		};
		break;

		/* director control */
		case VZ_CMD_START_DIRECTOR:
		case VZ_CMD_RESET_DIRECTOR:
		case VZ_CMD_CONTINUE_DIRECTOR:
		case VZ_CMD_STOP_DIRECTOR:
		{
			char* director_id;
			uint32_t* frame = NULL;

			if
			(
				0 
				==
				vz_serial_cmd_parseNmap
				(
					buf,
					index,
					(void*)&director_id,
					(void*)&frame
				)
			)
			{
				logger_printf(0, "vzScene: VZ_CMD_???_DIRECTOR('%s', '%d')\n", director_id, (frame)?((int)*frame):-1);
				vzMotionDirector* director = _id_directors.find(director_id);
				if(director)
				{
					char frame_str[128] = ""; 
					if(frame)
						sprintf(frame_str, "%d", *frame);

					switch(cmd)
					{
						case VZ_CMD_START_DIRECTOR:		director->start(frame_str);	break;
						case VZ_CMD_RESET_DIRECTOR:		director->reset(frame_str);	break;
						case VZ_CMD_CONTINUE_DIRECTOR:	director->cont(frame_str);	break;
						case VZ_CMD_STOP_DIRECTOR:		director->stop(frame_str);	break;
					};
				}
				else
					r = -2;
			}
			else
				r = -1;
		}
		break;

	};

	ReleaseMutex(_lock_for_command);

	return r;
};

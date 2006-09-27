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

#include "vzScene.h"
#include "xerces.h"
#include <stdio.h>
#include <windows.h>
#include <GL/glut.h>

static const unsigned short tag_tree[] = {'t', 'r', 'e', 'e',0};
static const unsigned short tag_motion[] = {'m', 'o', 't', 'i', 'o', 'n',0};


#include "gl_exts.cpp"

vzScene::vzScene(vzFunctions* functions, void* config, vzTVSpec* tv)
{
	_tv = tv;
	_config = (vzConfig*)config;
	_functions = functions;
	_tree = NULL;
	_motion = NULL;
	_lock_for_command = CreateMutex(NULL,FALSE,NULL);
	_GL_AUX_BUFFERS = -1;

	 // parameters from config
	 _enable_glBlendFuncSeparateEXT = (_config->param("vzMain","enable_glBlendFuncSeparateEXT"))?1:0;
	 _blend_dst_alpha = (_config->param("vzMain","blend_dst_alpha"))?1:0;
	 _fields = (_config->param("vzMain","fields"))?1:0;
	_stencil = malloc(_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT);

	 // build 8-bit stencil buffers
	 if(_fields)
	 {
		// prepare interlaced 8-bit stencil buffer
		for(int i=0,f=1 - _tv->TV_FRAME_1ST;i<_tv->TV_FRAME_HEIGHT;i++,f=1-f)
			memset
			(
				((unsigned char*)_stencil) + i*_tv->TV_FRAME_WIDTH,
				1<<f,
				_tv->TV_FRAME_WIDTH
			);
	 }	 
	 else
	 {
		// prepare non interlaces
		memset(_stencil,1 ,_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT);
	 };
};


int vzScene::load(char* file_name)
{
	printf("Loading scene '%s'... ",file_name);

	//init parser
	XercesDOMParser *parser = new XercesDOMParser;

	printf("Parsing... ");

    try
    {
        parser->parse(file_name);
    }
    catch (...)
    {
		delete parser;
		printf("Failed!\n");
		return 0;
    }


	DOMDocument* doc = parser->getDocument();
	if(!doc)
	{
		printf("Failed (NO doc)!\n");
		return 0;
	};

	DOMElement* scene = doc->getDocumentElement();

	DOMNodeList* scene_components = scene->getChildNodes();

	unsigned int i;

	printf("Looking for tree... ");
	// Find First <tree> and load into container
	for(i=0;(i<scene_components->getLength()) && (_tree == NULL);i++)
	{
		// getting child 
		DOMNode* scene_component = scene_components->item(i);

		// checking type
		if(scene_component->getNodeType() !=  DOMNode::ELEMENT_NODE)
			continue;

		// check node name
		if (XMLString::compareIString(scene_component->getNodeName(),tag_tree) == 0)
		{
			printf("Found containers.... ");
			_tree = new vzContainer(scene_component,_functions,this);
			printf("Loaded ");
			continue;
		};
	};

	printf("Looking for motion... ");
	// Find First <motion? and load into _motion
	for(i=0;(i<scene_components->getLength()) && (_motion == NULL);i++)
	{
		// getting child 
		DOMNode* scene_component = scene_components->item(i);

		// checking type
		if(scene_component->getNodeType() !=  DOMNode::ELEMENT_NODE)
			continue;

		// check node name
		if (XMLString::compareIString(scene_component->getNodeName(),tag_motion) == 0)
		{
			printf("Found Motion!!.... ");
			_motion = new vzMotion(scene_component,this);
			printf("Loaded ");
			continue;
		};
	};

	delete parser;

	printf("OK!\n");

	return 1;
};

vzScene::~vzScene()
{
	CloseHandle(_lock_for_command);
	if(_tree) delete _tree;
	if(_motion) delete _motion;
	if(_stencil) free(_stencil);
};


void vzScene::display(long frame)
{

	WaitForSingleObject(_lock_for_command,INFINITE);


	// Clear The Screen And The Depth Buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0, 0.0, 0.0, 0.0);
  //  glClearStencil( 0x1 );
	// Reset The View
	glLoadIdentity();										

	// disable depth tests - direct order
	glDisable(GL_DEPTH_TEST);

	// enable stencil test
	glEnable(GL_STENCIL_TEST);

	// alpha function always works
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_LEQUAL,1);

	// load gl/wgl EXTensions
	load_GL_EXT();

	// setup base stencil buffer
	if(_stencil)
	{
		glStencilMask(0xFF);
		// set stencil buffer from memmory
		glDrawPixels
		(
			_tv->TV_FRAME_WIDTH,
			_tv->TV_FRAME_HEIGHT,
			GL_STENCIL_INDEX,
			GL_UNSIGNED_BYTE,
			_stencil
		);
		free(_stencil);
		_stencil = NULL;
	};

	// draw fields/frame
	for(int field = 0; field<=_fields;field++)
	{
		// set directors for propper position
		if(_motion)
			_motion->assign(frame, field);

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

		// if Separate function supported
		if ((glBlendFuncSeparateEXT) && (_enable_glBlendFuncSeparateEXT))
		{
			// Separate function supported
			// change blending mode!
			if (!(_blend_dst_alpha))
			{
				glBlendFuncSeparateEXT
				(
					GL_SRC_ALPHA,
					GL_ONE_MINUS_SRC_ALPHA,
					GL_SRC_ALPHA_SATURATE,
					GL_ONE
				);
			}
			else
			{
				glBlendFuncSeparateEXT
				(
					GL_ONE_MINUS_DST_ALPHA,
					GL_DST_ALPHA,
					GL_SRC_ALPHA_SATURATE,
					GL_ONE
				);
			};
			// draw
			draw(frame,field,1,1,(_blend_dst_alpha)?0:1);
		}
		else
		{
			// separate function not supported
			// draw separate alpha and fill

			if (!(_blend_dst_alpha))
			{	
				// source alpha
				glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE);
				glColorMask(0,0,0,1); 
				draw(frame,field,0,1,1);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glColorMask(1,1,1,0);
				draw(frame,field,1,0,1);
				glColorMask(1,1,1,1);
			}
			else
			{

				// draw with destination
				glColorMask(1,1,1,1);
				glClearColor(0.0, 0.0, 0.0, 0.0);
				glClear(GL_COLOR_BUFFER_BIT);
				glBlendFunc(GL_ONE_MINUS_DST_ALPHA,GL_DST_ALPHA);
				draw(frame,field,1,1,0);
				// clear rendered alpha
				glColorMask(0,0,0,1);
				glClearColor(0.0, 0.0, 0.0, 0.0);
				glClear(GL_COLOR_BUFFER_BIT);
				// draw alpha lpha channel
				glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE);
				draw(frame,field,0,1,0);
				glColorMask(1,1,1,1);
			};
		};
	};

	// disable some features
	glDisable( GL_STENCIL_TEST );
	glDisable( GL_ALPHA_TEST );


	// flush all
//    glFlush();

	// and swap pages
//	glutSwapBuffers();

	ReleaseMutex(_lock_for_command);
};

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
	
	// if container is initializes - start draw method
	if (_tree)
		_tree->draw(&_render_session);
};

#ifdef _DEBUG
void vzScene::list_registred()
{
	unsigned int i;

	printf(__FILE__ "::\t" "Registred functions instances: ");
	for(i=0;i<_id_functions.count();i++)
		printf("'%s' ", _id_functions.key(i));

	printf(__FILE__ "::\t" "Registred containers instances: ");
	for(i=0;i<_id_containers.count();i++)
		printf("'%s' ", _id_containers.key(i));

	printf(__FILE__ "::\t" "Registred directors instances: ");
	for(i=0;i<_id_directors.count();i++)
		printf("'%s' ", _id_directors.key(i));

	printf(__FILE__ "::\t" "Registred timelines instances: ");
	for(i=0;i<_id_timelines.count();i++)
		printf("'%s' ", _id_timelines.key(i));
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
tree.motion.director.<director_id>.start();
tree.motion.director.<director_id>.stop();
tree.motion.director.<director_id>.continue();
tree.motion.director.<director_id>.reset();

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
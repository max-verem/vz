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
	2007-03-07:
		*migration to wider TTFont parameters list;

    2006-12-14:
		*constructor updates, added two parameters 'scene' 'parent_container'

	2006-09-28:
		*new async render method.

	2006-04-22:
		*cleanup handle of asynk text loader

	2005-06-08: Code cleanup


*/

static char* _plugin_description = 
""
;

static char* _plugin_notes = 
""
;

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"
#include "../vz/vzImage.h"
#include "../vz/plugin-procs.h"
#include "../templates/hash.hpp"
#include "../vz/vzTTFont.h"

#pragma comment(lib, "winmm.lib")

#include <process.h>
#include <stdio.h>

#include "get_font.h"

// set flag to dump images
#ifdef _DEBUG
//#define  _DUMP_IMAGES
#endif

#ifdef _DUMP_IMAGES
HANDLE _dump_counter_lock;
long _dump_counter = 0;
#endif

#define WAIT_TIMEOUT_VALUE 2000

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	int i;
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			// create mutex to lock list
			_fonts_list_lock = CreateMutex(NULL,FALSE,NULL);
#ifdef _DUMP_IMAGES
			_dump_counter_lock = CreateMutex(NULL,FALSE,NULL);
#endif

			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			// close mutex
			CloseHandle(_fonts_list_lock);
			// delete font instances
			for(i=0;i<vzTTFontList.count();i++)
				delete vzTTFontList.value(i);
			break;
    }
    return TRUE;
}

// declare name and version of plugin
DEFINE_PLUGIN_INFO("ttfont");

/* text param description struct */
struct text_params
{
	char* s_text;
	char* s_font;
	struct vzTTFontParams font_params;
	long h_font_colour;
	long h_stroke_colour;
	struct vzTTFontLayoutConf layout;
	float f_max_width;
};

#define default_text_params													\
{																			\
	NULL,																	\
	NULL,																	\
	vzTTFontParamsDefault,													\
	0,																		\
	0,																		\
	vzTTFontLayoutConfDefaultData,											\
	0.0f																	\
}

// internal structure of plugin
typedef struct
{
	// public
	long L_center;
	struct text_params params;

	// internal
	unsigned int _ready;
	char* _s_text_buf;
	float _width;
	float _height;
	float _offset_x;
	float _offset_y;
	float _base_width;
	float _base_height;
	unsigned int _texture;
	unsigned int _texture_initialized;
	HANDLE _lock_update;
	vzImage* _image;	// image is FLAG!!!!

	// state change detection
	struct text_params _params;
	HANDLE _async_renderer_handle;
	HANDLE _async_renderer_lock;
	struct text_params **_async_renderer_queue;
	long _exit_flag;

} vzPluginData;

// default value of structore
static vzPluginData default_value = 
{
	// public
	GEOM_CENTER_CM,
	default_text_params,

	// internal
	0,
	NULL,
	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,
	0,
	0,
	NULL,
	NULL,

	// state change detection
	default_text_params,
	INVALID_HANDLE_VALUE,
	INVALID_HANDLE_VALUE,
	NULL,
	0
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"s_font", "Font name", PLUGIN_PARAMETER_OFFSET(default_value,params.s_font)},
	{"s_text", "Text content", PLUGIN_PARAMETER_OFFSET(default_value,params.s_text)},
	{"L_center", "Center of image", PLUGIN_PARAMETER_OFFSET(default_value,L_center)},
	{"h_colour", "Text's colour (in hex)", PLUGIN_PARAMETER_OFFSET(default_value,params.h_font_colour)},
	{"h_stroke_colour", "Text's stroke colour (in hex)", PLUGIN_PARAMETER_OFFSET(default_value,params.h_stroke_colour)},
	{"f_line_space", "Line space multiplier", PLUGIN_PARAMETER_OFFSET(default_value,params.layout.line_space)},
	{"l_limit_width", "Limit width of text to expected", PLUGIN_PARAMETER_OFFSET(default_value,params.layout.limit_width)},
	{"l_limit_height", "Limit height of text", PLUGIN_PARAMETER_OFFSET(default_value,params.layout.limit_height)},
	{"l_break_word", "Break words during wrapping (flag)", PLUGIN_PARAMETER_OFFSET(default_value,params.layout.break_word)},
	{"l_fixed_kerning", "Fixed kerning width (0 if not used)", PLUGIN_PARAMETER_OFFSET(default_value,params.layout.fixed_kerning)},
	{"f_advance_ratio", "Advance ratio (char dist mult.)", PLUGIN_PARAMETER_OFFSET(default_value,params.layout.advance_ratio)},

	{"l_height", 
		"Font height", 
		PLUGIN_PARAMETER_OFFSET(default_value,params.font_params.height)},
	{"l_width", 
		"Font width", 
		PLUGIN_PARAMETER_OFFSET(default_value,params.font_params.width)},
	{"f_stroke_radius", 
		"Font stroke radius", 
		PLUGIN_PARAMETER_OFFSET(default_value,params.font_params.stroke_radius)},
	{"l_stroke_line_cap", 
		"Font stroke line cap style:\n"
		"\t0 - The end of lines is rendered as a full stop on the last point itself [FT_STROKER_LINECAP_BUTT];\n"
		"\t1 - The end of lines is rendered as a half-circle around the last point [FT_STROKER_LINECAP_ROUND];\n"
		"\t2 - The end of lines is rendered as a square around the last point [FT_STROKER_LINECAP_SQUARE];", 
		PLUGIN_PARAMETER_OFFSET(default_value,params.font_params.stroke_line_cap)},
	{"l_stroke_line_join",
		"Font line join style:\n"
		"\t0 - Used to render rounded line joins. Circular arcs are used to join two lines smoothly [FT_STROKER_LINEJOIN_ROUND];\n"
		"\t1 - Used to render beveled line joins; i.e., the two joining lines are extended until they intersect [FT_STROKER_LINEJOIN_BEVEL];\n"
		"\t2 - Same as beveled rendering, except that an additional line break is added if the angle between the two joining lines is too closed (this is useful to avoid unpleasant spikes in beveled rendering) [FT_STROKER_LINEJOIN_MITER];",
		PLUGIN_PARAMETER_OFFSET(default_value,params.font_params.stroke_line_join)},
	{"f_max_width",
		"Max font width. If defined, scale width to given value", 
		PLUGIN_PARAMETER_OFFSET(default_value,params.f_max_width)},
	
	{NULL,NULL,0}
};

//#define _DUMP_TEXT_PARAMS

#ifdef _DUMP_TEXT_PARAMS
#define dump_text_params(___p)	\
{								\
	fprintf						\
	(							\
		stderr,					\
		DEBUG_LINE_ARG  " text_params: s_text=%.8X, s_font=%.8X, l_height=%d, h_colour=%.8X\n", DEBUG_LINE_PARAM, \
		(___p)->s_text, (___p)->s_font, (___p)->l_height, (___p)->h_colour	\
	);							\
};
#else
#define dump_text_params(p)
#endif



static void free_text_params(struct text_params *p)
{
	if (p->s_text) free(p->s_text);
	if (p->s_font) free(p->s_font);
	free(p);
}

static unsigned long WINAPI _async_renderer(void* data)
{
	vzTTFont* font;
	vzImage* image;

	timeBeginPeriod(1);

#ifdef _DEBUG
	fprintf(stderr, DEBUG_LINE_ARG  "_async_renderer started\n", DEBUG_LINE_PARAM);
#endif /* _DEBUG */

	while(!(_DATA->_exit_flag))
	{
		WaitForSingleObject(_DATA->_async_renderer_lock, WAIT_TIMEOUT_VALUE);
		if(_DATA->_async_renderer_queue[0])
		{
			// element present
			ReleaseMutex(_DATA->_async_renderer_lock);

			// load font
			if
			(
				(_DATA->_async_renderer_queue[0]->s_font)
				&&
				(_DATA->_async_renderer_queue[0]->s_text)
				&&
				(
					font = get_font
					(
						_DATA->_async_renderer_queue[0]->s_font,
						&_DATA->_async_renderer_queue[0]->font_params
					)
				)
			)
			{
				// font load successfull


				// its was able to load font
				image = font->render
				(
					_DATA->_async_renderer_queue[0]->s_text,
					_DATA->_async_renderer_queue[0]->h_font_colour,
					_DATA->_async_renderer_queue[0]->h_stroke_colour,
					&_DATA->_async_renderer_queue[0]->layout
				);

				// make its 2^X
				vzImageExpand2X(image);

				// sync value
				// lock main struct
				WaitForSingleObject(_DATA->_lock_update, WAIT_TIMEOUT_VALUE);				
				if(_DATA->_image)
				// free image
					vzImageFree(_DATA->_image);
				_DATA->_image = image;
				// release mutex
				ReleaseMutex(_DATA->_lock_update);
			};

			// free fist element and shift queue
			free_text_params(_DATA->_async_renderer_queue[0]);
			WaitForSingleObject(_DATA->_async_renderer_lock, WAIT_TIMEOUT_VALUE);
			_DATA->_async_renderer_queue[0] = _DATA->_async_renderer_queue[1];
			_DATA->_async_renderer_queue[1] = NULL;
			ReleaseMutex(_DATA->_async_renderer_lock);
		}
		else
		{
			ReleaseMutex(_DATA->_async_renderer_lock);
			Sleep(5);
		};
	};

#ifdef _DEBUG
	fprintf(stderr, DEBUG_LINE_ARG  "_async_renderer finished\n", DEBUG_LINE_PARAM);
#endif /* _DEBUG */

	timeEndPeriod(1);

	ExitThread(0);
	return 0;
};


PLUGIN_EXPORT void* constructor(void* scene, void* parent_container)
{
	// init memmory for structure
	vzPluginData* data = (vzPluginData*)malloc(sizeof(vzPluginData));

	// copy default value
	*data = default_value;

	// create mutexes
	_DATA->_lock_update = CreateMutex(NULL,FALSE,NULL);

	// init space for internal text buffer
	_DATA->_s_text_buf = (char*)malloc(0);

	// async renderer
	unsigned long thread;
	_DATA->_async_renderer_queue = (struct text_params **)malloc(2*sizeof(struct text_params *));
	_DATA->_async_renderer_queue[0] = NULL;
	_DATA->_async_renderer_queue[1] = NULL;
	_DATA->_async_renderer_lock = CreateMutex(NULL,FALSE,NULL);
	_DATA->_async_renderer_handle = CreateThread(0, 0, _async_renderer, data, 0, &thread);
        SetThreadPriority(_DATA->_async_renderer_handle , VZPLUGINS_AUX_THREAD_PRIO);

	dump_text_params(&_DATA->params);

	// return pointer
	return data;
};

PLUGIN_EXPORT void destructor(void* data)
{
	int r;

	// check if texture initialized
	if(_DATA->_texture_initialized)
		glDeleteTextures (1, &(_DATA->_texture));

	// async renderer
	_DATA->_exit_flag = 1;
	WaitForSingleObject(_DATA->_async_renderer_handle, WAIT_TIMEOUT_VALUE);
	CloseHandle(_DATA->_async_renderer_handle);
	CloseHandle(_DATA->_async_renderer_lock);
	free(_DATA->_async_renderer_queue);

	// try to lock struct
	if(WAIT_OBJECT_0 != (r = WaitForSingleObject(_DATA->_lock_update, WAIT_TIMEOUT_VALUE)))
	{
#ifdef _DEBUG
		fprintf(stderr, DEBUG_LINE_ARG  "unable to lock: %s\n", DEBUG_LINE_PARAM, (r == WAIT_ABANDONED)?"WAIT_ABANDONED":"WAIT_TIMEOUT");
#endif /* _DEBUG */
		return;
	};

	// free image data if it's not released
	if(_DATA->_image)
	{
		vzImageFree(_DATA->_image);
		_DATA->_image = NULL;
	};

	// unlock
	ReleaseMutex(_DATA->_lock_update);

	// free space for internal text buffer
	free(_DATA->_s_text_buf);

	// close
	CloseHandle(_DATA->_lock_update);

	free(data);
};

PLUGIN_EXPORT void prerender(void* data,vzRenderSession* session)
{
	// try to lock struct
	if(WaitForSingleObject(_DATA->_lock_update,0) != WAIT_OBJECT_0)
		// was unable to lock 
		// do not wait - return
		return;

	// struct is locked

	// check if image is submitted
	if (_DATA->_image)
	{
		// image submitted!!!
		unsigned int texture,prev_texture;
		
		// generate new texture id
		glGenTextures(1, &texture);

		// load texture
		glBindTexture(GL_TEXTURE_2D, texture);

		gluBuild2DMipmaps
		(
			GL_TEXTURE_2D, 4, 
			_DATA->_image->width,
			_DATA->_image->height,
			GL_BGRA_EXT, //_DATA->_image->surface_type, //GL_BGRA_EXT,
			GL_UNSIGNED_BYTE, 
			_DATA->_image->surface
		);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	
		// subtiture texture
		prev_texture = _DATA->_texture;
		_DATA->_texture = texture;
		texture = prev_texture;

		// assign dimensions
		_DATA->_width = _DATA->_image->width;
		_DATA->_height = _DATA->_image->height;
		_DATA->_base_width = _DATA->_image->base_width;
		_DATA->_base_height = _DATA->_image->base_height;
		_DATA->_offset_x = _DATA->_image->base_x;
		_DATA->_offset_y = _DATA->_image->base_y;

		// free image
		vzImageFree(_DATA->_image);
		_DATA->_image = NULL;

		// check if need to release old texture
		if(_DATA->_texture_initialized)
		{
			// release previous texture
			glDeleteTextures (1, &texture);	
		};

		// set flag about new texture initialized
		_DATA->_texture_initialized = 1;
	};

	// release mutex
	ReleaseMutex(_DATA->_lock_update);
};

PLUGIN_EXPORT void postrender(void* data,vzRenderSession* session)
{
};


PLUGIN_EXPORT void render(void* data,vzRenderSession* session)
{
	// check if texture initialized
	if(_DATA->_texture_initialized)
	{
		/* scale factor */
		float f_max_width_scale = 0.0f;

		// determine center offset 
		float co_X = 0.0f, co_Y = 0.0f, co_Z = 0.0f;

		// translate coordinates accoring to base image
		center_vector(_DATA->L_center,_DATA->_base_width,_DATA->_base_height,co_X,co_Y);

		// translate coordinate according to real image
		co_Y += _DATA->_offset_y + _DATA->_base_height - _DATA->_height;
		co_X += (-1.0f) * _DATA->_offset_x;
		
		/* check if horizontal scale is needed */
		if(0.0f != _DATA->params.f_max_width)
		{
			if(_DATA->params.f_max_width < _DATA->_base_width)
				f_max_width_scale = _DATA->params.f_max_width / _DATA->_base_width;
		};

		/* scale for max size */
		if(0 != f_max_width_scale)
		{
			glPushMatrix();
			glScalef(f_max_width_scale, 1.0f, 1.0f);
		};

		// begin drawing
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _DATA->_texture);

		// Draw a quad (ie a square)
		glBegin(GL_QUADS);

		glColor4f(1.0f,1.0f,1.0f,session->f_alpha);

		glTexCoord2f(0.0f, 1.0f);
		glVertex3f(co_X + 0.0f, co_Y + 0.0f, co_Z + 0.0f);

		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(co_X + 0.0f, co_Y + _DATA->_height, co_Z + 0.0f);

		glTexCoord2f(1.0f, 0.0f);
		glVertex3f(co_X + _DATA->_width, co_Y + _DATA->_height, co_Z + 0.0f);

		glTexCoord2f(1.0f, 1.0f);
		glVertex3f(co_X + _DATA->_width, co_Y + 0.0f, co_Z + 0.0f);

		glEnd(); // Stop drawing QUADS

		glDisable(GL_TEXTURE_2D);

		/* scale for max size */
		if(0 != f_max_width_scale)
		{
			glPopMatrix();
		};
	}
	else
	{

	};
};

PLUGIN_EXPORT void notify(void* data)
{
	int r;

	//wait for mutext free
	if(WAIT_OBJECT_0 != (r = WaitForSingleObject(_DATA->_lock_update, WAIT_TIMEOUT_VALUE)))
	{
#ifdef _DEBUG
		fprintf(stderr, DEBUG_LINE_ARG  "unable to lock: %s\n", DEBUG_LINE_PARAM, (r == WAIT_ABANDONED)?"WAIT_ABANDONED":"WAIT_TIMEOUT");
#endif /* _DEBUG */
		return;
	};

	{
		char* tmp2;

		dump_text_params(&_DATA->params);

		/* copy data */
		struct text_params* tmp = (struct text_params*)malloc(sizeof(struct text_params));
		*tmp = _DATA->params;
		_DATA->_params = _DATA->params;
		dump_text_params(tmp);

		/* reallocate data for font name and text */
		if(tmp->s_font)
		{
			strcpy(tmp2 = (char*)malloc(strlen(tmp->s_font) + 1) , tmp->s_font); tmp->s_font = tmp2;
		};
		if(tmp->s_text)
		{
			strcpy(tmp2 = (char*)malloc(strlen(tmp->s_text) + 1) , tmp->s_text); tmp->s_text = tmp2;
		};

		/* release struct lock */
		dump_text_params(tmp);
		ReleaseMutex(_DATA->_lock_update);

		/* lock queue */
		WaitForSingleObject(_DATA->_async_renderer_lock, WAIT_TIMEOUT_VALUE);
		if(_DATA->_async_renderer_queue[0])
		{
			// first element busy 
			if(_DATA->_async_renderer_queue[1])
				// second element is busy too - replace 
				free_text_params(_DATA->_async_renderer_queue[1]);
			// setup element
			_DATA->_async_renderer_queue[1] = tmp;
		}
		else
			// firts element is free - setup
			_DATA->_async_renderer_queue[0] = tmp;
		ReleaseMutex(_DATA->_async_renderer_lock);
	};
};

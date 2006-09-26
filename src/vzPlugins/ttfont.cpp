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
	2006-04-22:
		*cleanup handle of asynk text loader

	2005-06-08: Code cleanup


*/

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"
#include "../vz/vzImage.h"
#include "../vz/plugin-procs.h"
#include "../templates/hash.hpp"
#include "../vz/vzTTFont.h"

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
PLUGIN_EXPORT vzPluginInfo info =
{
	"ttfont",
	1.0,
	"rc5"
};

// internal structure of plugin
typedef struct
{
	// public
	char* s_text;
	char* s_font;
	long L_center;
	long l_height;
	long h_colour;
	float f_line_space;
	float f_limit_width;
	float f_limit_height;
	long l_wrap_word;

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
	char* _s_font;
	char* _s_text;
	long _l_height;
	long _h_colour;

	HANDLE _async_text_loader;

} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
	// public
	NULL,
	NULL,
	GEOM_CENTER_CM,
	0,
	0,
	1.0f,
	-1.0f,
	-1.0f,
	0,
	
	// internal
	0,
	NULL,
	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,
	0,
	0,
	NULL,
	NULL,

	// state change detection
	NULL,
	NULL,
	0,
	0,
	INVALID_HANDLE_VALUE
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"s_text", "Text content", PLUGIN_PARAMETER_OFFSET(default_value,s_text)},
	{"s_font", "Font name", PLUGIN_PARAMETER_OFFSET(default_value,s_font)},
	{"L_center", "Center of image", PLUGIN_PARAMETER_OFFSET(default_value,L_center)},
	{"l_height", "Font height", PLUGIN_PARAMETER_OFFSET(default_value,l_height)},
	{"h_colour", "Text's colour (in hex)", PLUGIN_PARAMETER_OFFSET(default_value,h_colour)},

	{"f_line_space", "Line space multiplier", PLUGIN_PARAMETER_OFFSET(default_value,f_line_space)},
	{"f_limit_width", "Limit width of text to expected", PLUGIN_PARAMETER_OFFSET(default_value,f_limit_width)},
	{"f_limit_height", "Limit height of text", PLUGIN_PARAMETER_OFFSET(default_value,f_limit_height)},
	{"l_wrap_word", "Break words during wrapping (flag)", PLUGIN_PARAMETER_OFFSET(default_value,l_wrap_word)},
	{NULL,NULL,0}
};

PLUGIN_EXPORT void* constructor(void)
{
	// init memmory for structure
	vzPluginData* data = (vzPluginData*)malloc(sizeof(vzPluginData));

	// copy default value
	*data = default_value;

	// create mutexes
	_DATA->_lock_update = CreateMutex(NULL,FALSE,NULL);

	// init space for internal text buffer
	_DATA->_s_text_buf = (char*)malloc(0);

	// return pointer
	return data;
};

PLUGIN_EXPORT void destructor(void* data)
{
	// check if texture initialized
	if(_DATA->_texture_initialized)
		glDeleteTextures (1, &(_DATA->_texture));

	// wait and dispose handle
	if (INVALID_HANDLE_VALUE != _DATA->_async_text_loader)
		CloseHandle(_DATA->_async_text_loader);

	// try to lock struct
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

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


		// sync values
		_DATA->_s_text = _DATA->s_text;
		_DATA->_s_font = _DATA->s_font;
		_DATA->_l_height = _DATA->l_height;
		_DATA->_h_colour = _DATA->h_colour;

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
		// determine center offset 
		float co_X = 0.0f, co_Y = 0.0f, co_Z = 0.0f;

		// translate coordinates accoring to base image
		center_vector(_DATA->L_center,_DATA->_base_width,_DATA->_base_height,co_X,co_Y);

		// translate coordinate according to real image
		co_Y += _DATA->_offset_y + _DATA->_base_height - _DATA->_height;
		co_X += (-1.0f) * _DATA->_offset_x;

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
	}
	else
	{

	};
};

unsigned long WINAPI _text_loader(void* data)
{
	WaitForSingleObject(_DATA->_lock_update,INFINITE);
	char* error_log;

	vzTTFont* font = get_font(_DATA->s_font,_DATA->l_height);

	if(font)
	{
#ifdef _DEBUG
	printf(__FILE__ "::_text_loaded Rendering image (colour=%X",_DATA->h_colour);
#endif

#ifdef _DUMP_IMAGES
		WaitForSingleObject(_dump_counter_lock,INFINITE);
		long i = _dump_counter++;
		ReleaseMutex(_dump_counter_lock);
		char name1[100],name2[100],namet[100];;
		sprintf(name1,"d:/temp/ttfonts/%da.tga",i);
		sprintf(name2,"d:/temp/ttfonts/%db.tga",i);
		sprintf(namet,"d:/temp/ttfonts/%d.txt",i);

		FILE* f = fopen(namet,"wb");
		fwrite(_DATA->s_text,1,strlen(_DATA->_s_text_buf),f);
		fclose(f);
#endif
	
		// its was able to load font
		vzImage* image = font->render
		(
//			_DATA->s_text,
			_DATA->_s_text_buf,
			_DATA->h_colour,
			_DATA->f_line_space,
			_DATA->l_wrap_word,
			_DATA->f_limit_width,
			_DATA->f_limit_height
		);
#ifdef _DUMP_IMAGES
		// dump image from renderer
		vzImageSaveTGA(name1,image,NULL,0);
#endif


#ifdef _DEBUG
	printf(" width='%d' height='%d' base_x='%d' base_y='%d'\n",image->width,image->height,image->base_x,image->base_y);
#endif
		// make its 2^X
#ifdef _DEBUG
try
{
#endif
		vzImageExpand2X(image);

#ifdef _DUMP_IMAGES
		// dump image from renderer
		vzImageSaveTGA(name2,image,NULL,0);
#endif



#ifdef _DEBUG
}
catch (char* hard_error)
{
	printf("ERRROR!!!!!!!!!!!\n width='%d' height='%d' base_x='%d' base_y='%d'\n",image->width,image->height,image->base_x,image->base_y);
	exit(-1);
}
#endif

		// save image
		image->surface_type = GL_BGRA_EXT;
		_DATA->_image = image;


	}
	else
	{
#ifdef _DEBUG
	printf(__FILE__ "::_text_loaded Unable to load font\n");
#endif
	};

	// release mutex
	ReleaseMutex(_DATA->_lock_update);
	
	// and thread
	ExitThread(0);
	return 0;
};

PLUGIN_EXPORT void notify(void* data)
{
#ifdef _DEBUG
	printf(__FILE__ "::notified\n");
#endif

#ifdef _DEBUG
	printf(__FILE__ "::notify() s_text=\"%s\",%X\n",_DATA->s_text,_DATA->s_text);
#endif

	//wait for mutext free
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	// check if data changed
	if ((_DATA->s_text != _DATA->_s_text) || (_DATA->s_font != _DATA->_s_font) || (_DATA->l_height != _DATA->_l_height) || (_DATA->h_colour != _DATA->_h_colour))
	{
		// check if image not synced
		if (_DATA->_image)
		{
			vzImageFree(_DATA->_image);
			_DATA->_image = NULL;
		};

		_DATA->_s_text_buf = (char*)realloc(_DATA->_s_text_buf,strlen(_DATA->s_text) + 1);
		memcpy(_DATA->_s_text_buf,_DATA->s_text,strlen(_DATA->s_text)+1);

		// wait until previous start finish
		if (INVALID_HANDLE_VALUE != _DATA->_async_text_loader)
			CloseHandle(_DATA->_async_text_loader);
		//start thread for texture loading
		unsigned long thread;
		_DATA->_async_text_loader = CreateThread(0, 0, _text_loader, data, 0, &thread);
	}

	ReleaseMutex(_DATA->_lock_update);

};

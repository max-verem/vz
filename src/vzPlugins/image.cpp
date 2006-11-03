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
	2006-04-23:
		*cleanup handle of asynk image loader

    2005-06-08: Code cleanup

*/

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"
#include "../vz/vzImage.h"
#include "../vz/plugin-procs.h"

#include <process.h>
#include <stdio.h>

// declare name and version of plugin
PLUGIN_EXPORT vzPluginInfo info =
{
	"image",
	1.0,
	"rc8"
};

// internal structure of plugin
typedef struct
{
	char* s_filename;
	long L_center;
	char* _filename;

	HANDLE _lock_update;

	float _width;
	float _height;
	float _offset_x;
	float _offset_y;
	float _base_width;
	float _base_height;
	unsigned int _texture;
	unsigned int _texture_initialized;
	vzImage* _image;
	HANDLE _async_image_loader;
} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
	NULL,
	GEOM_CENTER_CM,
	NULL,

	NULL,

	0.0f,0.0f,
	0.0f,0.0f,
	0.0f,0.0f,
	0,
	0,
	NULL,
	INVALID_HANDLE_VALUE
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"s_filename", "Name of filename to load", PLUGIN_PARAMETER_OFFSET(default_value,s_filename)},
	{"L_center", "Center of image", PLUGIN_PARAMETER_OFFSET(default_value,L_center)},
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
	ReleaseMutex(_DATA->_lock_update);

	// return pointer
	return data;
};

PLUGIN_EXPORT void destructor(void* data)
{
	// check if texture initialized
	if(_DATA->_texture_initialized)
		glDeleteTextures (1, &(_DATA->_texture));

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


	// close mutexes
	CloseHandle(_DATA->_lock_update);

	// free data
	free(data);
};

PLUGIN_EXPORT void prerender(void* data,vzRenderSession* session)
{
	unsigned long r;

	// try to lock struct
	if((r = WaitForSingleObject(_DATA->_lock_update,0)) != WAIT_OBJECT_0)
	{
		// was unable to lock 
		// do not wait - return
		ERROR_LOG("unable to lock due to:",(r == WAIT_ABANDONED)?"WAIT_ABANDONED":"WAIT_TIMEOUT")
		return;
	};

	// struct is locked
	// check if image is submitted
	if (_DATA->_image)
	{
		// image submitted!!!
		
		// sync file name
		_DATA->_filename = _DATA->s_filename;

		unsigned int texture,prev_texture;
		
		// generate new texture id
		glGenTextures(1, &texture);

		// load texture
		glBindTexture(GL_TEXTURE_2D, texture);
/*
		gluBuild2DMipmaps
		(
			GL_TEXTURE_2D, 4, 
			_DATA->_image->width,
			_DATA->_image->height,
			GL_BGRA_EXT, //_DATA->_image->surface_type, //GL_BGRA_EXT,
			GL_UNSIGNED_BYTE, 
			_DATA->_image->surface
		);
*/
glTexImage2D
(
	GL_TEXTURE_2D,			// GLenum target,
	0,						// GLint level,
	4,						// GLint components,
	_DATA->_image->width,	// GLsizei width, 
	_DATA->_image->height,	// GLsizei height, 
	0,						// GLint border,
	GL_BGRA_EXT,			// GLenum format,
	GL_UNSIGNED_BYTE,		// GLenum type,
	_DATA->_image->surface	// const GLvoid *pixels
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

		// glTexCoord2f() takes a U and V (U,V) into our texture.  The U and V are 
		// in the range from 0 to 1.  And work like this:

	/*	  (0,1)  (1,1) 
		    _______
		   |	   |
		   |	   |
		   |	   |			Just like Cartesian coordinates :)
		    -------
		  (0,0)  (1,0)

	*/

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
/*
printf("image: '%s', width='%f',height='%f',base_width='%f',base_height='%f',"
	   "offset_x='%f',offset_y='%f',co_X='%f',co_Y='%f'\n",
	   _DATA->_filename,_DATA->_width,_DATA->_height,_DATA->_base_width,_DATA->_base_height,
	   _DATA->_offset_x,_DATA->_offset_y,co_X,co_Y);
*/
	}
	else
	{
//ERROR_LOG("NOT INIT","");
	};
};

unsigned long WINAPI _image_loader(void* data)
{
	WaitForSingleObject(_DATA->_lock_update,INFINITE);
	char* error_log;
	vzImage* image;

	// load image
	if(image = vzImageLoadTGA(_DATA->s_filename,&error_log))
	{
		// image  loaded !!!!
		_DATA->_image = vzImageExpand2X(image);
	}
	else
	{
		// unable to load file
		ERROR_LOG("Unable to load file", error_log);
	};

	// release mutex
	if(ReleaseMutex(_DATA->_lock_update) == 0)
	{
		printf("Error! Unable to release mutex: code %ld\n",GetLastError());
	};
	
	// and thread
	ExitThread(0);
	return 0;
};

PLUGIN_EXPORT void notify(void* data)
{
	// check if pointer to filenames is different?
	if(_DATA->s_filename != _DATA->_filename)
	{
		//wait for mutext free
		WaitForSingleObject(_DATA->_lock_update,INFINITE);

		// check if image not synced
		// but new arrived - lets delete them!!!
		if (_DATA->_image)
		{
			vzImageFree(_DATA->_image);
			_DATA->_image = NULL;
		};

		//start thread for texture loading
		if (INVALID_HANDLE_VALUE != _DATA->_async_image_loader)
			CloseHandle(_DATA->_async_image_loader);
		//start thread for texture loading
		_DATA->_async_image_loader = CreateThread(0, 0, _image_loader, data, 0, NULL);

		// release mutex -  let created tread work
		ReleaseMutex(_DATA->_lock_update);
	};
};

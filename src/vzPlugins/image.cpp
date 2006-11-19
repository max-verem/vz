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
	2006-11-19:
		*texture flip support
		*refactoryng async loader.

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
// public parameters
	char* s_filename;		/* file with image */
	long L_center;			/* align type */
	long l_flip_v;			/* flip vertical flag */
	long l_flip_h;			/* flip vertical flag */

// internal data
	char* _filename;
	void* _surface;
	HANDLE _lock_update;

	long _width;
	long _height;
	long _base_width;
	long _base_height;
	unsigned int _texture;
	unsigned int _texture_initialized;
	vzImage* _image;
	HANDLE _async_image_loader;
} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
// public parameters
	NULL,					// char* s_filename;		/* file with image */
	GEOM_CENTER_CM,			// long L_center;			/* align type */
	0,						// long l_flip_v;			/* flip vertical flag */
	0,						// long l_flip_h;			/* flip vertical flag */

// internal data
	NULL,					// char* _filename;
	NULL,					// void* _surface;
	INVALID_HANDLE_VALUE,	// HANDLE _lock_update;

	0,						// long _width;
	0,						// long _height;
	0,						// long _base_width;
	0,						// long _base_height;
	0,						// unsigned int _texture;
	0,						// unsigned int _texture_initialized;
	NULL,					// vzImage* _image;
	INVALID_HANDLE_VALUE	// HANDLE _async_image_loader;
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"s_filename", "Name of image filename to load", PLUGIN_PARAMETER_OFFSET(default_value,s_filename)},
	{"L_center", "Center of image", PLUGIN_PARAMETER_OFFSET(default_value,L_center)},
	{"l_flip_v", "flag to vertical flip", PLUGIN_PARAMETER_OFFSET(default_value,l_flip_v)},
	{"l_flip_h", "flag to horozontal flip", PLUGIN_PARAMETER_OFFSET(default_value,l_flip_h)},
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

	// Wait until previous thread finish
	if (INVALID_HANDLE_VALUE != _DATA->_async_image_loader)
		CloseHandle(_DATA->_async_image_loader);

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

	// lock struct
	WaitForSingleObject(_DATA->_lock_update, INFINITE);

	// check if image is submitted
	if
	(
		(_DATA->_image)
		&&
		(_DATA->_image->width)
		&&
		(_DATA->_image->height)
	)
	{
		// image submitted!!!
		if
		(
			(_DATA->_width != POT(_DATA->_image->width))
			||
			(_DATA->_height != POT(_DATA->_image->height))
		)
		{
			/* texture should be (re)initialized */

			/* delete old texture in some case */
			if(_DATA->_texture_initialized)
				glDeleteTextures (1, &(_DATA->_texture));

			/* generate new texture */
			glGenTextures(1, &_DATA->_texture);

			/* set flags */
			_DATA->_width = POT(_DATA->_image->width);
			_DATA->_height = POT(_DATA->_image->height);
			_DATA->_base_width = _DATA->_image->width;
			_DATA->_base_height = _DATA->_image->height;
			_DATA->_texture_initialized = 1;

			/* generate fake surface */
			void* fake_frame = malloc(4*_DATA->_width*_DATA->_height);
			memset(fake_frame,0,4*_DATA->_width*_DATA->_height);

			/* create texture (init texture memory) */
			glBindTexture(GL_TEXTURE_2D, _DATA->_texture);
			glTexImage2D
			(
				GL_TEXTURE_2D,			// GLenum target,
				0,						// GLint level,
				4,						// GLint components,
				_DATA->_width,			// GLsizei width, 
				_DATA->_height,			// GLsizei height, 
				0,						// GLint border,
				GL_BGRA_EXT,			// GLenum format,
				GL_UNSIGNED_BYTE,		// GLenum type,
				fake_frame				// const GLvoid *pixels
			);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

			/* free memory of fake image */
			free(fake_frame);

			/* for safe condition reset surface trigger */
			_DATA->_surface = NULL;
		};


		/* load image if it new */
		if(_DATA->_surface != _DATA->_image->surface)
		{
			/* load */
			glBindTexture(GL_TEXTURE_2D, _DATA->_texture);
			glTexSubImage2D
			(
				GL_TEXTURE_2D,									// GLenum target,
				0,												// GLint level,
				(_DATA->_width - _DATA->_image->width)/2,		// GLint xoffset,
				(_DATA->_height - _DATA->_image->height)/2,		// GLint yoffset,
				_DATA->_image->width,							// GLsizei width,
				_DATA->_image->height,							// GLsizei height,
				GL_BGRA_EXT,									// GLenum format,
				GL_UNSIGNED_BYTE,								// GLenum type,
				_DATA->_image->surface							// const GLvoid *pixels 
			);

			/* sync */
			_DATA->_surface = _DATA->_image->surface;

			/* free image */
			vzImageFree(_DATA->_image);
			_DATA->_image = NULL;
		};
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
	if
	(
		(_DATA->_texture_initialized)
	)
	{
		// determine center offset 
		float co_X = 0.0f, co_Y = 0.0f, co_Z = 0.0f;

		// translate coordinates accoring to base image
		center_vector(_DATA->L_center,_DATA->_base_width,_DATA->_base_height,co_X,co_Y);

		// translate coordinate according to real image
		co_Y -= (_DATA->_height - _DATA->_base_height)/2;
		co_X -= (_DATA->_width - _DATA->_base_width)/2;

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

		glTexCoord2f
		(
			(_DATA->l_flip_h)?1.0f:0.0f, 
			(_DATA->l_flip_v)?0.0f:1.0f
		);
		glVertex3f(co_X + 0.0f, co_Y + 0.0f, co_Z + 0.0f);

		glTexCoord2f
		(
			(_DATA->l_flip_h)?1.0f:0.0f,
			(_DATA->l_flip_v)?1.0f:0.0f
		);
		glVertex3f(co_X + 0.0f, co_Y + _DATA->_height, co_Z + 0.0f);

		glTexCoord2f
		(
			(_DATA->l_flip_h)?0.0f:1.0f,
			(_DATA->l_flip_v)?1.0f:0.0f
		);
		glVertex3f(co_X + _DATA->_width, co_Y + _DATA->_height, co_Z + 0.0f);

		glTexCoord2f
		(
			(_DATA->l_flip_h)?0.0f:1.0f,
			(_DATA->l_flip_v)?0.0f:1.0f
		);
		glVertex3f(co_X + _DATA->_width, co_Y + 0.0f, co_Z + 0.0f);

		glEnd(); // Stop drawing QUADS

		glDisable(GL_TEXTURE_2D);
	};
};

struct imageloader_desc
{
	char filename[1024];
	void* data;
};

static unsigned long WINAPI imageloader_proc(void* d)
{
	char* error_log;
	vzImage* image;

	struct imageloader_desc* desc = (struct imageloader_desc*)d;
	void* data = desc->data;

	/* notify */
	printf("image: imageloader_proc('%s')\n", desc->filename);

	// load image
	if(image = vzImageLoadTGA(desc->filename, &error_log))
	{
		/* image  loaded -  assign new image */

		/* lock */
		WaitForSingleObject(_DATA->_lock_update,INFINITE);
		
		/* free image if its exists */
		if(_DATA->_image)
			vzImageFree(_DATA->_image);

		/* assign new */
		_DATA->_image = image;

		/* reset surface flag */
		_DATA->_surface = NULL;

		/* unlock */
		ReleaseMutex(_DATA->_lock_update);
	}
	else
		// unable to load file
		ERROR_LOG("Unable to load TGA file", error_log);

	/* free data */
	free(d);

	// and thread
	ExitThread(0);
	return 0;
};

PLUGIN_EXPORT void notify(void* data)
{
	struct imageloader_desc* desc;

	//wait for mutext free
	WaitForSingleObject(_DATA->_lock_update, INFINITE);

	// check if pointer to filenames is different?
	if(_DATA->_filename != _DATA->s_filename)
	{
		// Wait until previous thread finish
		if (INVALID_HANDLE_VALUE != _DATA->_async_image_loader)
			CloseHandle(_DATA->_async_image_loader);

		/* allocate desc info */
		desc = (struct imageloader_desc*)malloc(sizeof(struct imageloader_desc));
		memset(desc, 0, sizeof(struct imageloader_desc));

		/* setup arguments */
		desc->data = data;
		strcpy(desc->filename, _DATA->s_filename);

		//start thread for texture loading
		_DATA->_async_image_loader = CreateThread(0, 0, imageloader_proc, desc, 0, NULL);

		/* sunc filename */
		_DATA->_filename = _DATA->s_filename;
	};

	// release mutex -  let created tread work
	ReleaseMutex(_DATA->_lock_update);
};

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
	2006-12-14:
		*constructor updates, added two parameters 'scene' 'parent_container'

	2006-11-20:
		*'glDrawArrays' method usage
		*free transform implementation

	2006-11-19:
		*texture flip support
		*refactoryng async loader.

	2006-04-23:
		*cleanup handle of asynk image loader

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

#include <process.h>
#include <stdio.h>

#include "free_transform.h"

//#define VERBOSE2

// declare name and version of plugin
DEFINE_PLUGIN_INFO("image");

// internal structure of plugin
typedef struct
{
// public parameters
	char* s_filename;		/* file with image */
	long L_center;			/* align type */
	long l_flip_v;			/* flip vertical flag */
	long l_flip_h;			/* flip vertical flag */
// free transform coords
	float f_x1;				/* left bottom coner */
	float f_y1;
	float f_z1;
	float f_x2;				/* left upper coner */
	float f_y2;
	float f_z2;
	float f_x3;				/* right upper coner */
	float f_y3;
	float f_z3;
	float f_x4;				/* right bottom coner */
	float f_y4;
	float f_z4;
	long  l_tr_lod;			/* number of breaks */

// internal data
	char* _filename;
	void* _surface;
	HANDLE _lock_update;
	float* _ft_vertices;
	float* _ft_texels;

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
// free transform coords
	0.0,					// float f_x1;				/* left bottom coner */
	0.0,					// float f_y1;
	0.0,					// float f_z1;
	0.0,					// float f_x2;				/* left upper coner */
	0.0,					// float f_y2;
	0.0,					// float f_z2;
	0.0,					// float f_x3;				/* right upper coner */
	0.0,					// float f_y3;
	0.0,					// float f_z3;
	0.0,					// float f_x4;				/* right bottom coner */
	0.0,					// float f_y4;
	0.0,					// float f_z4;
	30,						// long  l_tr_lod;			/* number of breaks */


// internal data
	NULL,					// char* _filename;
	NULL,					// void* _surface;
	INVALID_HANDLE_VALUE,	// HANDLE _lock_update;
	NULL,					// float* _ft_vertices;
	NULL,					// float* _ft_texels;

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

	{"f_x1", "X of left bottom corner (free transorm mode)", PLUGIN_PARAMETER_OFFSET(default_value, f_x1)},
	{"f_y1", "Y of left bottom corner (free transorm mode)", PLUGIN_PARAMETER_OFFSET(default_value, f_y1)},
	{"f_z1", "Z of left bottom corner (free transorm mode)", PLUGIN_PARAMETER_OFFSET(default_value, f_z1)},

	{"f_x2", "X of left upper corner (free transorm mode)", PLUGIN_PARAMETER_OFFSET(default_value, f_x2)},
	{"f_y2", "Y of left upper corner (free transorm mode)", PLUGIN_PARAMETER_OFFSET(default_value, f_y2)},
	{"f_z2", "Z of left upper corner (free transorm mode)", PLUGIN_PARAMETER_OFFSET(default_value, f_z2)},

	{"f_x3", "X of right upper corner (free transorm mode)", PLUGIN_PARAMETER_OFFSET(default_value, f_x3)},
	{"f_y3", "Y of right upper corner (free transorm mode)", PLUGIN_PARAMETER_OFFSET(default_value, f_y3)},
	{"f_z3", "Z of right upper corner (free transorm mode)", PLUGIN_PARAMETER_OFFSET(default_value, f_z3)},

	{"f_x4", "X of right bottom corner (free transorm mode)", PLUGIN_PARAMETER_OFFSET(default_value, f_x4)},
	{"f_y4", "Y of right bottom corner (free transorm mode)", PLUGIN_PARAMETER_OFFSET(default_value, f_y4)},
	{"f_z4", "Z of right bottom corner (free transorm mode)", PLUGIN_PARAMETER_OFFSET(default_value, f_z4)},

	{"l_tr_lod", "Level of triangulation (free transorm mode)", PLUGIN_PARAMETER_OFFSET(default_value, l_tr_lod)},

	{NULL,NULL,0}
};

PLUGIN_EXPORT void* constructor(void* scene, void* parent_container)
{
	// init memmory for structure
	vzPluginData* data = (vzPluginData*)malloc(sizeof(vzPluginData));

	// copy default value
	*data = default_value;

	// create mutexes
	_DATA->_lock_update = CreateMutex(NULL,FALSE,NULL);
	ReleaseMutex(_DATA->_lock_update);

	/* allocate space for _filename */
	_DATA->_filename = (char*)malloc(MAX_PATH);
	_DATA->_filename[0] = 0;

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

	/* free arrays coords */
	if(_DATA->_ft_vertices) free(_DATA->_ft_vertices);
	if(_DATA->_ft_texels) free(_DATA->_ft_texels);

	/* free filename data */
	free(_DATA->_filename);

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
/*
			(_DATA->_width != POT(_DATA->_image->width))
			||
			(_DATA->_height != POT(_DATA->_image->height))
*/
			(_DATA->_base_width != _DATA->_image->width)
			||
			(_DATA->_base_height != _DATA->_image->height)
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
	double p;
	int i;

	// check if texture initialized

	/* not free transform mode */
	if
	(
		(_DATA->_texture_initialized)
	)
	{
		float X1,X2,X3,X4, Y1,Y2,Y3,Y4, Z1,Z2,Z3,Z4, XC,YC,ZC;

		if (FOURCC_TO_LONG('_','F','T','_') == _DATA->L_center)
		{
			/* free transform mode */
			
			/* reset Z */
			Z1 = (Z2 = (Z3 = (Z4 = 0.0f)));

			/* calc coordiantes of image */
			calc_free_transform
			(
				/* dimentsions */
				_DATA->_width, _DATA->_height,
				_DATA->_base_width, _DATA->_base_height,

				/* source coordinates */
				_DATA->f_x1, _DATA->f_y1,
				_DATA->f_x2, _DATA->f_y2,
				_DATA->f_x3, _DATA->f_y3,
				_DATA->f_x4, _DATA->f_y4,

				/* destination coordinates */
				&X1, &Y1,
				&X2, &Y2,
				&X3, &Y3,
				&X4, &Y4
			);

#ifdef VERBOSE1
			printf
			(
				"\n"
				"1: (%7.3f,%7.3f) -> (%7.3f,%7.3f)\n"
				"2: (%7.3f,%7.3f) -> (%7.3f,%7.3f)\n"
				"3: (%7.3f,%7.3f) -> (%7.3f,%7.3f)\n"
				"4: (%7.3f,%7.3f) -> (%7.3f,%7.3f)\n",

				_DATA->f_x1, _DATA->f_y1, X1, Y1,
				_DATA->f_x2, _DATA->f_y2, X2, Y2,
				_DATA->f_x3, _DATA->f_y3, X3, Y3,
				_DATA->f_x4, _DATA->f_y4, X4, Y4
			);
#endif	/* VERBOSE1 */

		}
		else if (FOURCC_TO_LONG('_','F','C','_') == _DATA->L_center)
		{
			X1 = _DATA->f_x1;
			Y1 = _DATA->f_y1;
			Z1 = _DATA->f_z1;

			X2 = _DATA->f_x2;
			Y2 = _DATA->f_y2;
			Z2 = _DATA->f_z2;
			
			X3 = _DATA->f_x3;
			Y3 = _DATA->f_y3;
			Z3 = _DATA->f_z3;

			X4 = _DATA->f_x4;
			Y4 = _DATA->f_y4;
			Z4 = _DATA->f_z4;
		}
		else
		{
			/* normal mode */

			// determine center offset 
			float co_X = 0.0f, co_Y = 0.0f, co_Z = 0.0f;

			// translate coordinates accoring to base image
			center_vector(_DATA->L_center,_DATA->_base_width,_DATA->_base_height,co_X,co_Y);

			// translate coordinate according to real image
			co_Y -= (_DATA->_height - _DATA->_base_height)/2;
			co_X -= (_DATA->_width - _DATA->_base_width)/2;

			X1 = co_X + 0.0f;
			Y1 = co_Y + 0.0f;
			Z1 = co_Z + 0.0f;

			X2 = co_X + 0.0f;
			Y2 = co_Y + _DATA->_height;
			Z2 = co_Z + 0.0f;

			X3 = co_X + _DATA->_width;
			Y3 = co_Y + _DATA->_height;
			Z3 = co_Z + 0.0f;

			X4 = co_X + _DATA->_width;
			Y4 = co_Y + 0.0f;
			Z4 = co_Z + 0.0f;
		};


		/* begin drawing */
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _DATA->_texture);

/*
	NB!

			glTexCoord2f() takes a U and V (U,V) into our texture.  The U and V are 
			in the range from 0 to 1.  And work like this:

			(0,1)  (1,1) 
			+--------+
			|        |
			|        |
			|        |
			|        |   Just like Cartesian coordinates :)
			+--------+
			(0,0)  (1,0)

*/
		if
		(
			(FOURCC_TO_LONG('_','F','T','_') == _DATA->L_center)
			||
			(FOURCC_TO_LONG('_','F','C','_') == _DATA->L_center)
		)
		{
			ZC = 0.0f;

			/* setup colour & alpha */
			glColor4f(1.0f,1.0f,1.0f,session->f_alpha);

			/* reinit arrays */
			if(_DATA->_ft_vertices) free(_DATA->_ft_vertices);
			if(_DATA->_ft_texels) free(_DATA->_ft_texels);
			_DATA->_ft_vertices = (float*)malloc(sizeof(float) * 3 * (2*_DATA->l_tr_lod + 2));
			_DATA->_ft_texels = (float*)malloc(sizeof(float) * 2 * (2*_DATA->l_tr_lod + 2));

			/* fill array */
			for(i = 0; i<= (2*_DATA->l_tr_lod + 1); i++)
			{
				p = ((double)(i/2))/_DATA->l_tr_lod;

				if(i & 1)
				{
					/* 'right' line */

					/* setup texture coords */
					_DATA->_ft_texels[i * 2 + 0] = 1.0f;
					_DATA->_ft_texels[i * 2 + 1] = p;

					/* calc vector coord */
					calc_ft_vec_part(X3,Y3, X4,Y4, p, &XC,&YC);
				}
				else
				{
					/* left line */

					/* setup texture coords */
					_DATA->_ft_texels[i * 2 + 0] = 0.0f;
					_DATA->_ft_texels[i * 2 + 1] = p;

					/* calc vector coord */
					calc_ft_vec_part(X2,Y2, X1,Y1, p, &XC,&YC);
				};

				_DATA->_ft_vertices[i * 3 + 0] = XC;
				_DATA->_ft_vertices[i * 3 + 1] = YC;
				_DATA->_ft_vertices[i * 3 + 2] = ZC;
			};

			/* enable vertex and texture coors array */
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);

			/* init vertext and texel array pointers */
			glVertexPointer(3, GL_FLOAT, 0, _DATA->_ft_vertices);
			glTexCoordPointer(2, GL_FLOAT, 0, _DATA->_ft_texels);

			/* draw array */
			glDrawArrays(GL_TRIANGLE_STRIP, 0,  (2*_DATA->l_tr_lod + 2) );

			/* enable vertex and texture coors array */
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);
		}
		else
		{
			// Draw a quad (ie a square)
			glBegin(GL_QUADS);
			glColor4f(1.0f,1.0f,1.0f,session->f_alpha);

			/* (0,0) */
			glTexCoord2f
			(
				(_DATA->l_flip_h)?1.0f:0.0f, 
				(_DATA->l_flip_v)?0.0f:1.0f
			);
			glVertex3f(X1, Y1, Z1);

			/* (0,1) */
			glTexCoord2f
			(
				(_DATA->l_flip_h)?1.0f:0.0f,
				(_DATA->l_flip_v)?1.0f:0.0f
			);
			glVertex3f(X2, Y2, Z2);

			/* (1,1) */
			glTexCoord2f
			(
				(_DATA->l_flip_h)?0.0f:1.0f,
				(_DATA->l_flip_v)?1.0f:0.0f
			);
			glVertex3f(X3, Y3, Z3);

			/* (1,0) */
			glTexCoord2f
			(
				(_DATA->l_flip_h)?0.0f:1.0f,
				(_DATA->l_flip_v)?0.0f:1.0f
			);
			glVertex3f(X4, Y4, Z4);

			// Stop drawing QUADS
			glEnd(); // Stop drawing
		};

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

PLUGIN_EXPORT void notify(void* data, char* param_name)
{
	struct imageloader_desc* desc;

//	printf("image: notify('%s')\n", param_name);

	//wait for mutext free
	WaitForSingleObject(_DATA->_lock_update, INFINITE);

	/* check if parameter "s_filename" changed */
	if
	(
		(NULL == param_name)
		||
		(0 == strcmp(param_name, "s_filename"))
	)
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
                SetThreadPriority(_DATA->_async_image_loader , VZPLUGINS_AUX_THREAD_PRIO);

		/* store filename */
		strcpy(_DATA->_filename, _DATA->s_filename);
	};

	// release mutex -  let created tread work
	ReleaseMutex(_DATA->_lock_update);
};

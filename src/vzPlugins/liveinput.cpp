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
	2006-12-16:
		*audio mixing/support added.

	2006-12-14:
		*constructor updates, added two parameters 'scene' 'parent_container'

	2006-12-12:
		*texture updates depend on field mode, for frame mode we update texture
		one time per two filed.

	2006-12-11:
		*'twice_fields' depends on field mode, fixed.

	2006-12-06:
		*one more trick with 'f_offset_y' 'l_offset_y' - no greater picture 
		improvement.

	2006-12-05:
		*'Twice height' reincarnated (again) due to configurable behavior of
		fields duplicator in IO driver.
		*'y_offset' for future.

	2006-12-04:
		*Twice height removed. Field has the same size as frame. Duplicating
		lines should be provided in IO driver.
  
	2006-12-03:
		*Height twice in field mode.

	2006-12-01:
		*Draft version startup

*/

static char* _plugin_description = 
"Live video input source."
;

static char* _plugin_notes = 
"Depends on IO driver functionality."
;

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"
#include "../vz/plugin-procs.h"
#include "../vz/vzGlExt.h"
#include "../vz/vzOutput.h"

#include <stdio.h>
#include <windows.h>

#include "free_transform.h"


BOOL APIENTRY DllMain
(
	HANDLE hModule, 
    DWORD  ul_reason_for_call, 
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

#define PBO_SLICES  4
#define _pbo_empty_size (2048 * 2048 * 4) /* 1920x1080 should fit */
static unsigned int _pbo_empty_buf;

// declare name and version of plugin
DEFINE_PLUGIN_INFO("liveinput");

/* ------------------------------------------------------------------------- */

// internal structure of plugin
typedef struct
{
// public data
	long l_input;			/* input channel number */
	long L_center;			// centering of image
	long l_flip_v;			/* flip vertical flag */
	long l_flip_h;			/* flip vertical flag */
	long l_offset_y;		/* texture field dribling */
	float f_offset_y;		/* coords field dribling */
// audio 
	float f_audio_level;	/* level in DB of mixer, 0.0 - do not change */
	long l_audio_mute;		/* mute audio flag */
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

// internal datas
	HANDLE _lock_update;	// update struct mutex
    void* _output_context;
    int last_sys_id;
	unsigned int _texture;
	unsigned int _texture_initialized;
    long _width;
    long _height;
    long _base_width;
    long _base_height;
	float* _ft_vertices;
	float* _ft_texels;
    unsigned int _pbo[PBO_SLICES];
    unsigned int _pbo_size;
    unsigned int _pbo_slice_size;
    unsigned int _pbo_slice_height;
} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
// public data
	0,						// long l_input;			/* input channel number */
	GEOM_CENTER_CM,			// long L_center;			// centering of image
	0,						// long l_flip_v;			/* flip vertical flag */
	0,						// long l_flip_h;			/* flip vertical flag */
	0,						// long l_offset_y;			/* texture field dribling */
	0.0,					// float f_offset_y;		/* coords field dribling */
// audio 
	0.0f,					// float f_audio_level;	/* level in DB of mixer, 0.0 - do not change */
	0,						// long l_audio_mute;		/* mute audio flag */
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


// internal datas
	INVALID_HANDLE_VALUE,	// HANDLE _lock_update;	// update struct mutex
	NULL,					// void* _output_context;
    -1,                      // int last_sys_id;
	0,						// unsigned int _texture;
	0,						// unsigned int _texture_initialized;
	0,						// long _width;
	0,						// long _height;
    0,                      // long _base_width;
    0,                      // long _base_height;
	NULL,					// float* _ft_vertices;
	NULL					// float* _ft_texels;
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"l_input",			"number of input (1..4)",
						PLUGIN_PARAMETER_OFFSET(default_value,l_input)},

	{"L_center",		"centering of image",
						PLUGIN_PARAMETER_OFFSET(default_value,L_center)},

	{"l_flip_v",		"flag to vertical flip",
						PLUGIN_PARAMETER_OFFSET(default_value,l_flip_v)},

	{"l_flip_h",		"flag to horozontal flip",
						PLUGIN_PARAMETER_OFFSET(default_value,l_flip_h)},

	{"l_offset_y",		"texture field number depend dribling (-1, 0, 1)",
						PLUGIN_PARAMETER_OFFSET(default_value,l_offset_y)},

	{"f_offset_y",		"rect coordinate Y field number depend dribling [-1.0 .. 1.0]",
						PLUGIN_PARAMETER_OFFSET(default_value,l_offset_y)},

	{"f_x1",			"X of left bottom corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_x1)},
	{"f_y1",			"Y of left bottom corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_y1)},
	{"f_z1",			"Z of left bottom corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_z1)},

	{"f_x2",			"X of left upper corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_x2)},
	{"f_y2",			"Y of left upper corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_y2)},
	{"f_z2",			"Z of left upper corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_z2)},

	{"f_x3",			"X of right upper corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_x3)},
	{"f_y3",			"Y of right upper corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_y3)},
	{"f_z3",			"Z of right upper corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_z3)},

	{"f_x4",			"X of right bottom corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_x4)},
	{"f_y4",			"Y of right bottom corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_y4)},
	{"f_z4",			"Z of right bottom corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_z4)},

	{"l_tr_lod",		"Level of triangulation (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, l_tr_lod)},

	{"l_audio_mute",	"Mute audio flag",
						PLUGIN_PARAMETER_OFFSET(default_value, l_audio_mute)},

	{"f_audio_level",	"Audio mixer level, db (0.0 - no change)",
						PLUGIN_PARAMETER_OFFSET(default_value, f_audio_level)},

	{NULL,NULL,0}
};

PLUGIN_EXPORT int load(void* config)
{
    int r;
    void* p;

    /* init empty PBO object */
    glGenBuffers(1, &_pbo_empty_buf);
    r = glGetError();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, _pbo_empty_buf);
    glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, _pbo_empty_size, 0, GL_STREAM_DRAW);
    p = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
    memset(p, 0, _pbo_empty_size);
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

    return 0;
};

PLUGIN_EXPORT int unload(void* config)
{
    /* check for global empty pbo */
    glDeleteBuffers(1, &_pbo_empty_buf);

    return 0;
};

PLUGIN_EXPORT void* constructor(void* scene, void* parent_container)
{
	// init memmory for structure
	vzPluginData* data = (vzPluginData*)malloc(sizeof(vzPluginData));

	// copy default value
	*data = default_value;

    /* assign output context */
    _DATA->_output_context = vzOutputGlobalContextGet();

	// create mutexes
	_DATA->_lock_update = CreateMutex(NULL,FALSE,NULL);
	ReleaseMutex(_DATA->_lock_update);

	// return pointer
	return data;
};

PLUGIN_EXPORT int release(void* data)
{
    vzPluginData* ctx = (vzPluginData*)data;

    // try to lock struct
    WaitForSingleObject(ctx->_lock_update,INFINITE);

    // check if texture initialized
    if(ctx->_texture_initialized)
        glErrorLog(glDeleteTextures(1, &ctx->_texture););

    /* delete buffers from non opengl context */
    if(ctx->_pbo_size)
        glErrorLog(glDeleteBuffers(PBO_SLICES, ctx->_pbo););

    // unlock
    ReleaseMutex(_DATA->_lock_update);

    return 0;
};

PLUGIN_EXPORT void destructor(void* data)
{
	int i;

	// try to lock struct
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	/* free arrays coords */
	if(_DATA->_ft_vertices) free(_DATA->_ft_vertices);
	if(_DATA->_ft_texels) free(_DATA->_ft_texels);

	// unlock
	ReleaseMutex(_DATA->_lock_update);

	// close mutexes
	CloseHandle(_DATA->_lock_update);

	// free data
	free(data);
};

PLUGIN_EXPORT void prerender(void* data,vzRenderSession* session)
{
    int r, b;
    vzImage* img;
    vzPluginData* ctx = (vzPluginData*)data;

    // try to lock struct
    WaitForSingleObject(_DATA->_lock_update,INFINITE);

    /* basic data */
    if(!ctx->_output_context || !ctx->l_input)
    {
        // release mutex
        ReleaseMutex(ctx->_lock_update);
        return;
    };

    /* request image */
    r = vzOutputInputPull(ctx->_output_context, ctx->l_input - 1, (void**)&img);
    if(r || !img)
    {
        // release mutex
        ReleaseMutex(ctx->_lock_update);
        return;
    };

    /* recreare PBO if needs */
    if(ctx->_pbo_size != img->line_size * img->height)
    {
        /* delete buffer if needed */
        if(ctx->_pbo_size)
            glDeleteBuffers(PBO_SLICES, ctx->_pbo);

        /* update frame size */
        ctx->_pbo_size = img->line_size * img->height;
        ctx->_pbo_slice_height = (img->height + PBO_SLICES - 1) / PBO_SLICES;
        ctx->_pbo_slice_size = ctx->_pbo_slice_height * img->line_size;

        /* generate new buffers */
        glGenBuffers(PBO_SLICES, ctx->_pbo);

        /* setup buffers size */
        for(b = 0; b < PBO_SLICES; b++)
        {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, ctx->_pbo[b]);
            glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, ctx->_pbo_slice_size, 0, GL_STREAM_DRAW);
        };
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    };

    /* recreate texture */
    if(!ctx->_texture_initialized ||
        ctx->_base_width != img->width ||
        ctx->_base_height != img->height)
    {
        /* texture should be (re)initialized */
        if(ctx->_texture_initialized)
            glErrorLog(glDeleteTextures(1, &(ctx->_texture)););

        /* generate new texture */
        glErrorLog(glGenTextures(1, &ctx->_texture));

        /* set */
        ctx->_base_width = img->width;
        ctx->_base_height = img->height;
        ctx->_width = POT(img->width);
        ctx->_height = POT(img->height);
        ctx->_texture_initialized = 1;

        /* create texture (init texture memory) */
        glErrorLogD(glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, _pbo_empty_buf));
        glErrorLog(glBindTexture(GL_TEXTURE_2D, _DATA->_texture));
        glErrorLog(glTexImage2D
        (
            GL_TEXTURE_2D,          // GLenum target,
            0,                      // GLint level,
            GL_RGBA8,               // GLint components,
            _DATA->_width,          // GLsizei width, 
            _DATA->_height,         // GLsizei height, 
            0,                      // GLint border,
            GL_BGRA_EXT,            // GLenum format,
            GL_UNSIGNED_BYTE,       // GLenum type,
            NULL                    // const GLvoid *pixels
        ));
        glErrorLog(glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR));
        glErrorLog(glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR));
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    };

    /* load texture */
    if(img->sys_id != ctx->last_sys_id)
    {
        ctx->last_sys_id = img->sys_id;

#if 0
        glErrorLog(glBindTexture(GL_TEXTURE_2D, _DATA->_texture));  /* bind to buffer */

        /* load */
        glTexSubImage2D
        (
            GL_TEXTURE_2D,                                  // GLenum target,
            0,                                              // GLint level,
            (_DATA->_width - img->width)/2,                 // GLint xoffset,
            (_DATA->_height - img->height)/2,               // GLint yoffset,
            img->width,                                     // GLsizei width,
            img->height,                                    // GLsizei height,
            vzImagePixFmt2OGL(img->pix_fmt),                // GLenum format,
            GL_UNSIGNED_BYTE,                               // GLenum type,
            (unsigned char*)img->surface                    // const GLvoid *pixels
        );
#else
        /* draw buffers */
        for(b = 0; b < PBO_SLICES; b++)
        {
            void* pbo_ptr;

            glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, ctx->_pbo[b]);
            pbo_ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
            if(pbo_ptr)
            {
                void* src_slice_ptr = (unsigned char*)img->surface + b * ctx->_pbo_slice_size;
                unsigned int src_slice_size =
                    VZ_MIN(ctx->_pbo_size - b * ctx->_pbo_slice_size, ctx->_pbo_slice_size);

                memcpy(pbo_ptr, src_slice_ptr, src_slice_size);

                glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
            };

            glBindTexture(GL_TEXTURE_2D, _DATA->_texture);
            glTexSubImage2D
            (
                GL_TEXTURE_2D,                                  // GLenum target,
                0,                                              // GLint level,
                (_DATA->_width - img->width)/2,                 // GLint xoffset,
                (_DATA->_height - img->height)/2                // GLint yoffset,
                    + b * ctx->_pbo_slice_height,
                img->width,                                     // GLsizei width,
                ctx->_pbo_slice_height,                         // GLsizei height,
                vzImagePixFmt2OGL(img->pix_fmt),                // GLenum format,
                GL_UNSIGNED_BYTE,                               // GLenum type,
                NULL
            );
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
        };
#endif
    };

    r = vzOutputInputPullBack(ctx->_output_context, ctx->l_input - 1, (void**)&img);

    // release mutex
    ReleaseMutex(_DATA->_lock_update);
};

PLUGIN_EXPORT void postrender(void* data,vzRenderSession* session)
{
};

PLUGIN_EXPORT void render(void* data, vzRenderSession* session)
{
    int I;
    vzPluginData* ctx = (vzPluginData*)data;

    // check if texture initialized
    if(!ctx->_texture_initialized)
        return;

		/* mode depend rendering */
		if (FOURCC_TO_LONG('_','F','T','_') == _DATA->L_center)
		{
			int i;
			double p;

			/* free transform mode */
			float X1,X2,X3,X4, Y1,Y2,Y3,Y4, Z1,Z2,Z3,Z4, XC,YC,ZC;

			/* reset Z */
			ZC = (Z1 = (Z2 = (Z3 = (Z4 = 0.0f))));

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

			/* begin drawing */
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, _DATA->_texture);

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

			glDisable(GL_TEXTURE_2D);
		}
		else
		{
			/* no transform mode */
		
			// determine center offset 
			float co_X = 0.0f, co_Y = 0.0f, co_Z = 0.0f;

			/* fix co_Y */
			co_Y += _DATA->f_offset_y*session->field;

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

PLUGIN_EXPORT void notify(void* data, char* param_name)
{
	//wait for mutext free
	WaitForSingleObject(_DATA->_lock_update,INFINITE);


	// release mutex -  let created tread work
	ReleaseMutex(_DATA->_lock_update);

};

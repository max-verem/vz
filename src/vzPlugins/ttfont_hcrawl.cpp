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
	2006-09-21:
		*ttfont source as base src for plugin


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

#ifdef _DEBUG
#endif

#ifdef _DEBUG
//#define  _DUMP_IMAGES
#endif

#ifdef _DUMP_IMAGES
static long _dump_images_counter = 0;
#endif


BOOL APIENTRY DllMain
(
	HANDLE hModule, 
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


struct txt_msg_descr
{
	long id;
	long status;
	float pos;
	long width;

	long done;
	HANDLE async;
	char* buffer;
	void* parent;
};

// declare name and version of plugin
PLUGIN_EXPORT vzPluginInfo info =
{
	"ttfont_hcrawl",
	1.0,
	"rc8"
};

// internal structure of plugin
typedef struct
{
	/* PUBLIC params */
	char* s_font;
	long l_height;
	long h_colour;
	long l_box_width;
	long l_loop;
	long l_interval;
	float f_speed;

	/* PUBLIC triggers */
	char* s_trig_append;
	long l_reset;

	/* INTERNAL */
	HANDLE _lock_update;				/* handle locking data struct */
	struct txt_msg_descr** _txt_msg;	/* text of messages queue */
	long _txt_msg_len;					/* length of messages queue */
	vzTTFont* _font;					/* font used for rendering glyphs */
	unsigned int _texture;				/* texture id */
	long _texture_initialized;			/* flag indicated texture init */

	char* _s_trig_append;				/* state change detection */
	long _l_box_width;
	long _l_height;
	char* _s_font;

} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
	/* PUBLIC params */
	NULL,					/* char* s_font; */
	0,						/* long l_height; */
	0,						/* long h_colour; */
	0,						/* long l_box_width; */
	0,						/* long l_loop; */
	0,						/* long l_interval; */
	0.0f,					/* double f_speed; */

	/* PUBLIC triggers */
	NULL,					/* char* s_trig_append; */
	0,						/* long l_reset; */

	/* INTERNAL */
	INVALID_HANDLE_VALUE,	/* HANDLE _lock_update; */
	NULL,					/* struct txt_msg_descr** _txt_msg; */
	0,						/* long _txt_msg_len; */
	NULL,					/* vzTTFont* _font;	*/
	0,						/* texture id */
	0,						/* flag indicated texture init */

	NULL,					/* char* _s_trig_append; */
	0,						/* long _l_box_width */
	0,						/* long _l_height; */
	NULL					/* char* _s_font; */
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"s_font", "Font name", PLUGIN_PARAMETER_OFFSET(default_value,s_font)},
	{"l_height", "Font height", PLUGIN_PARAMETER_OFFSET(default_value,l_height)},
	{"h_colour", "Text's colour (in hex)", PLUGIN_PARAMETER_OFFSET(default_value,h_colour)},
	{"l_box_width", "Line space multiplier", PLUGIN_PARAMETER_OFFSET(default_value,l_box_width)},
	{"l_loop", "Inicated loop operation (1 - enable)", PLUGIN_PARAMETER_OFFSET(default_value,l_loop)},
	{"l_interval", "Interval (px) between text messages", PLUGIN_PARAMETER_OFFSET(default_value,l_interval)},
	{"f_speed", "Speed of moving left blocks (px/field)", PLUGIN_PARAMETER_OFFSET(default_value,f_speed)},
	{"s_trig_append", "Append string to crawl", PLUGIN_PARAMETER_OFFSET(default_value,s_trig_append)},
	{"l_reset", "Reset currently layouted objects", PLUGIN_PARAMETER_OFFSET(default_value,l_reset)},
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

	// init messages queue
	_DATA->_txt_msg = (struct txt_msg_descr**)malloc(0);
	_DATA->_txt_msg_len = 0;

	// return pointer
	return data;
};

PLUGIN_EXPORT void destructor(void* data)
{
	int i;

	// check if texture initialized
	if(_DATA->_texture_initialized)
		glDeleteTextures (1, &(_DATA->_texture));

	// try to lock struct
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	/* wait for all rendering objects and destroy*/
	for(i = 0; i<_DATA->_txt_msg_len; i++)
	{
		if(INVALID_HANDLE_VALUE != _DATA->_txt_msg[i]->async)
			CloseHandle(_DATA->_txt_msg[i]->async);
		free(_DATA->_txt_msg[i]);
	};
	free(_DATA->_txt_msg);
	_DATA->_txt_msg_len = 0;

	// unlock
	ReleaseMutex(_DATA->_lock_update);

	// close
	CloseHandle(_DATA->_lock_update);

	/* close main struct */
	free(data);
};

PLUGIN_EXPORT void prerender(void* data,vzRenderSession* session)
{
	int i, j, l;

	/* try to lock struct */
	if(WaitForSingleObject(_DATA->_lock_update,0) != WAIT_OBJECT_0)
		// was unable to lock 
		// do not wait - return
		return;

	/* defragment queue, administrative tasks */
	for(i=0;i<_DATA->_txt_msg_len; i++)
	{
		/* check if thread ends and handler not close */
		if
		(
			(_DATA->_txt_msg[i]->done)
			&&
			(INVALID_HANDLE_VALUE != _DATA->_txt_msg[i]->async)
		)
		{
			CloseHandle(_DATA->_txt_msg[i]->async);
			_DATA->_txt_msg[i]->async = INVALID_HANDLE_VALUE;
#ifdef _DEBUG
			fprintf(stderr, DEBUG_LINE_ARG  
				"detected async proc finish for obj %d\n", DEBUG_LINE_PARAM, i);
#endif /* _DEBUG */
		};

		/* set status '1' to just finished rendering objects */
		if
		(
			(_DATA->_txt_msg[i]->done)
			&&
			(_DATA->_txt_msg[i]->status == 0)
			&&
			(_DATA->_txt_msg[i]->id >= 0)
		)
		{
			_DATA->_txt_msg[i]->status = 1;
#ifdef _DEBUG
			fprintf(stderr, DEBUG_LINE_ARG  
				"obj %d been rised status to '1'\n", DEBUG_LINE_PARAM, i);
#endif /* _DEBUG */
		};


		/* check if we need to delete item from queue */
		if(_DATA->_txt_msg[i]->id < 0)
		{
#ifdef _DEBUG
			fprintf(stderr, DEBUG_LINE_ARG  
				"obj %d deleted from queue\n", DEBUG_LINE_PARAM, i);
#endif /* _DEBUG */

			struct txt_msg_descr* tmp = _DATA->_txt_msg[i];

			for(j=i+1;j<_DATA->_txt_msg_len;j++)
				_DATA->_txt_msg[j - 1] = _DATA->_txt_msg[j];
			i--;
			_DATA->_txt_msg_len--;

			free(tmp);
		};
	};

	/* resort items: the higher status - ther lefter item is */
	for(i = 0;i<(_DATA->_txt_msg_len - 1); i++)
		for(j = i + 1; j < _DATA->_txt_msg_len; j++)
			if(_DATA->_txt_msg[i]->status < _DATA->_txt_msg[j]->status)
			{
				struct txt_msg_descr* tmp = _DATA->_txt_msg[j];
				_DATA->_txt_msg[j] = _DATA->_txt_msg[i];
				_DATA->_txt_msg[i] = tmp;
			};

	/* setup coordinates */
	if (_DATA->_txt_msg_len > 0)
		if (_DATA->_txt_msg[0]->status > 0)
		{
			/* detetct starting offset */
			if (_DATA->_txt_msg[0]->status == 1)
			{
				/* first item is just rendered - start coordinates running*/
				_DATA->_txt_msg[0]->pos = (float)_DATA->l_box_width;

				/* rise status */
				_DATA->_txt_msg[0]->status = 2;

#ifdef _DEBUG
				fprintf(stderr, DEBUG_LINE_ARG  
					"new objects got pos='%f'\n", DEBUG_LINE_PARAM, _DATA->_txt_msg[0]->pos);
#endif /* _DEBUG */

			};

			/* setup coordinates for further messages */
			for(i = 1; (i<_DATA->_txt_msg_len) && (_DATA->_txt_msg[i]->status > 0); i++)
				/* calc new coordinate for new items */
				if (_DATA->_txt_msg[i]->status == 1)
				{
					/* check if previous item off screen */	
					if ((_DATA->_txt_msg[i - 1]->pos + _DATA->_txt_msg[i - 1]->width + _DATA->l_interval) >= _DATA->l_box_width)
					{
						/* prev item' tail deep out of surface */
						_DATA->_txt_msg[i]->pos = 
							_DATA->_txt_msg[i - 1]->pos
							+
							(float)_DATA->_txt_msg[i - 1]->width
							+
							(float)_DATA->l_interval;
					}
					else
						_DATA->_txt_msg[i]->pos = (float)_DATA->l_box_width;

					/* change status */
					_DATA->_txt_msg[i]->status = 2;
				};

		};

	// release mutex
	ReleaseMutex(_DATA->_lock_update);
};

PLUGIN_EXPORT void postrender(void* data,vzRenderSession* session)
{
	int i;

	/* try to lock struct */
	if(WaitForSingleObject(_DATA->_lock_update,0) != WAIT_OBJECT_0)
		// was unable to lock 
		// do not wait - return
		return;

	/* decrement coordinate of objects */
	for(i = 0; (i<_DATA->_txt_msg_len) && (_DATA->_txt_msg[i]->status > 0); i++)
		_DATA->_txt_msg[i]->pos -=  _DATA->f_speed;

	/* perform loop condition */
	if (_DATA->_txt_msg_len > 0)
		if (_DATA->_txt_msg[0]->status == 2)
			if (( (long)_DATA->_txt_msg[0]->pos + _DATA->_txt_msg[0]->width) <= 0)
			{
#ifdef _DEBUG
				fprintf(stderr, DEBUG_LINE_ARG  "object out of loop\n", DEBUG_LINE_PARAM);
#endif /* _DEBUG */

				/* first item is out of visible area */
				_DATA->_txt_msg[0]->status = 0;

				if (!(_DATA->l_loop))
				{
					/* destroy symbols */
					_DATA->_font->delete_symbols(_DATA->_txt_msg[0]->id);

					/* mark this container dead */
					_DATA->_txt_msg[0]->id = -1;
#ifdef _DEBUG
					fprintf(stderr, DEBUG_LINE_ARG  "and will be destroyed\n", DEBUG_LINE_PARAM);
#endif /* _DEBUG */

				};
			};
	
	// release mutex
	ReleaseMutex(_DATA->_lock_update);
};


PLUGIN_EXPORT void render(void* data,vzRenderSession* session)
{
	int i;

	/* try to lock struct */
	if(WaitForSingleObject(_DATA->_lock_update,0) != WAIT_OBJECT_0)
		// was unable to lock 
		// do not wait - return
		return;

	/* check if width and height setted */
	if ( (_DATA->l_height == 0) || (_DATA->l_box_width == 0) || (_DATA->_font == NULL)) 
	{
		ReleaseMutex(_DATA->_lock_update);
		return;
	};

	// --------------------------------------------------------------------
	/* create texture */
	long w = POT(_DATA->l_box_width);
	long h = POT(_DATA->l_height*2);
	if(!(_DATA->_texture_initialized))
	{
		// generate new texture id
		glGenTextures(1, &_DATA->_texture);

		// generate fake surface
		void* fake_frame = malloc(4*w*h);
		memset(fake_frame,0,4*w*h);

		// create texture (init texture memory)
		glBindTexture(GL_TEXTURE_2D, _DATA->_texture);
		glTexImage2D
		(
			GL_TEXTURE_2D,			// GLenum target,
			0,						// GLint level,
			4,						// GLint components,
			w,						// GLsizei width, 
			h,						// GLsizei height, 
			0,						// GLint border,
			GL_BGRA_EXT,			// GLenum format,
			GL_UNSIGNED_BYTE,		// GLenum type,
			fake_frame				// const GLvoid *pixels
		);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

		/* free memory of fake image */
		free(fake_frame);

		/* rise flag */
		_DATA->_texture_initialized = 1;

#ifdef _DEBUG
		fprintf(stderr, DEBUG_LINE_ARG  "generated new texture (%d x %d)\n", DEBUG_LINE_PARAM, w, h);
#endif /* _DEBUG */

	};

	// --------------------------------------------------------------------
	/* create temp image */
	vzImage* image = vzImageNew
	(
		_DATA->l_box_width, 
		_DATA->l_height*2, 
		4*_DATA->l_box_width*(_DATA->l_height*2)
	);

	/* blit texts according to plugin logic */
	for(i = 0; (i<_DATA->_txt_msg_len) && (_DATA->_txt_msg[i]->status == 2); i++)
		_DATA->_font->render_to
		(
			image, 
			_DATA->_txt_msg[i]->pos, 0, 
			_DATA->_txt_msg[i]->id,
			_DATA->h_colour
		);
	vzImageFlipVertical(image);

#ifdef _DUMP_IMAGES
	char dump_file_name[1024];
	sprintf(dump_file_name, "c:/temp/d1/%.5d.tga", _dump_images_counter++);
	vzImageSaveTGA(dump_file_name, image, NULL, 0);
#endif /* _DUMP_IMAGES */


	// --------------------------------------------------------------------
	/* update texture part */
	glBindTexture(GL_TEXTURE_2D, _DATA->_texture);
	glTexSubImage2D
	(
		GL_TEXTURE_2D,			// GLenum target,
		0,						// GLint level,
		(w - _DATA->l_box_width)/2, // GLint xoffset,
		0,						// GLint yoffset,
		_DATA->l_box_width,		// GLsizei width,
		_DATA->l_height*2,		// GLsizei height,
		GL_BGRA_EXT,			// GLenum format,
		GL_UNSIGNED_BYTE,		// GLenum type,
		image->surface			// const GLvoid *pixels 
	);

	/* free image */
	vzImageFree(image);

	// --------------------------------------------------------------------
	// begin drawing
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, _DATA->_texture);

	// Draw a quad (ie a square)
	glBegin(GL_QUADS);

	glColor4f(1.0f,1.0f,1.0f,session->f_alpha);

	/* up - left corner */
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f
	(
		(float)(-w/2),
		(float)_DATA->l_height, 
		0.0f
	);

	/* bottom - left corner */
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f
	(
		(float)(-w/2), 
		(float)(_DATA->l_height - h), 
		0.0f
	);

	/* bottom - right corner */
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f
	(
		(float)(w - w/2), 
		(float)(_DATA->l_height - h), 
		0.0f
	);

	/* up - right corner */
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f
	(
		(float)(w - w/2), 
		(float)_DATA->l_height, 
		0.0f
	);

	glEnd(); // Stop drawing QUADS

	glDisable(GL_TEXTURE_2D);
	// --------------------------------------------------------------------

	ReleaseMutex(_DATA->_lock_update);
};

unsigned long WINAPI _msg_layouter(void* param)
{
#ifdef _DEBUG
		fprintf(stderr, DEBUG_LINE_ARG
			"Started '_msg_layouter'\n", DEBUG_LINE_PARAM );
#endif /* _DEBUG */

	/* source data */
	struct txt_msg_descr* txt_msg = (struct txt_msg_descr*)param;
	void* data = txt_msg->parent;

	/* detect font used */
	vzTTFont* font = _DATA->_font;

	/* check if font is ok */
	if(font)
	{
		/* compose */
		txt_msg->id = font->compose(txt_msg->buffer);
		txt_msg->width = font->get_symbol_width(txt_msg->id);
#ifdef _DEBUG
		fprintf(stderr, DEBUG_LINE_ARG 
			"Composed: txt_msg->id='%d'txt_msg->width='%d'\n", DEBUG_LINE_PARAM, 
			txt_msg->id, txt_msg->width);
#endif /* _DEBUG */
	}
	else
	{
		/* id is fail  */
		txt_msg->id = -1;
	};

	/* mark process done and */
	txt_msg->done = 1;

	/* free buffer used */
	free(txt_msg->buffer); txt_msg->buffer = NULL;

//	DEBUG_OUT("Finished '_msg_layouter'\n");

	/* Return */
	ExitThread(0);
	return 0;
};

PLUGIN_EXPORT void notify(void* data)
{
	int i;

	//wait for mutext free
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	/* texture param changed */
	if
	(
		(_DATA->l_height != _DATA->_l_height)
		||
		(_DATA->_l_box_width != _DATA->l_box_width)
	)
	{
		/* detelete prev texture */
		if(_DATA->_texture_initialized)
		{
			glDeleteTextures (1, &(_DATA->_texture));
			_DATA->_texture_initialized = 0;
		};

		/* fix value */
		_DATA->_l_box_width = _DATA->l_box_width;
	}


	/* check if font data changed */
	if
	(
		(_DATA->l_height != _DATA->_l_height)
		|| 
		(_DATA->s_font != _DATA->_s_font) 
	)
	{
		/* reset redered queue */
		for(i = 0; i<_DATA->_txt_msg_len; i++)
		{
			/* wait for async renderer */
			if(_DATA->_txt_msg[i]->async != INVALID_HANDLE_VALUE)
				CloseHandle(_DATA->_txt_msg[i]->async);

			if(_DATA->_font)
				/* free symbols */
				_DATA->_font->delete_symbols(_DATA->_txt_msg[i]->id);

			/* free data */
			free(_DATA->_txt_msg[i]);
		};
		_DATA->_txt_msg_len = 0;

		/* sync font */
		_DATA->_s_font = _DATA->s_font;
		_DATA->_l_height = _DATA->l_height;

		/* setup new font */
		_DATA->_font = get_font(_DATA->s_font,_DATA->l_height);
	};

	/* appending message check */
	if(_DATA->_s_trig_append != _DATA->s_trig_append)
	{
#ifdef _DEBUG
		fprintf(stderr, DEBUG_LINE_ARG 
			"Detected flag '_DATA->_s_trig_append'\n", DEBUG_LINE_PARAM);
#endif /* _DEBUG */

		/* reallocate queue */
		_DATA->_txt_msg_len++;
		_DATA->_txt_msg = (struct txt_msg_descr**) realloc(_DATA->_txt_msg, _DATA->_txt_msg_len*sizeof(struct txt_msg_descr*));

		/* create new */
		struct txt_msg_descr* txt_msg = (struct txt_msg_descr*)malloc(sizeof(struct txt_msg_descr));
		memset(txt_msg, 0, sizeof(struct txt_msg_descr));
		_DATA->_txt_msg[_DATA->_txt_msg_len - 1] = txt_msg;
		
		/* copy text buffer */
		int l = strlen(_DATA->s_trig_append) + 1;
		memcpy(txt_msg->buffer = (char*)malloc(l),  _DATA->s_trig_append, l);
		txt_msg->parent = _DATA;

		/* start async thread */
		unsigned long thread;
		txt_msg->async = CreateThread(0, 0, _msg_layouter, txt_msg, 0, &thread);

		/* clear bit */
		_DATA->_s_trig_append = _DATA->s_trig_append;
	};

	/* check if reset rised */
	if(_DATA->l_reset)
	{
		/* change flags */
		for(i = 0; (i<_DATA->_txt_msg_len); i++)
			if
			(
				(_DATA->_txt_msg[i]->done > 0)
				&&
				(_DATA->_txt_msg[i]->id >= 0)
				&&
				(_DATA->_font)
			)
			{
				/* destroy symbols */
				_DATA->_font->delete_symbols(_DATA->_txt_msg[i]->id);
				_DATA->_txt_msg[i]->id = -1;
			}

		_DATA->l_reset = 0;
	};

	ReleaseMutex(_DATA->_lock_update);

};

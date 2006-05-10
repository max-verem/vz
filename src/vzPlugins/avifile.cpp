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
	2005-06-30:	Draft Version

*/

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"
#include "../vz/plugin-procs.h"

#include <stdio.h>
#include <windows.h>
#include <vfw.h>
#pragma comment(lib, "VFW32.LIB")
#pragma comment(lib, "winmm.lib")


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
			// init avi lib !
			AVIFileInit();
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


// declare name and version of plugin
PLUGIN_EXPORT vzPluginInfo info =
{
	"avifile",
	1.0,
	"rc4"
};

#define CMD_PLAY		FOURCC_TO_LONG('_','P','L','Y')
#define CMD_STOP		FOURCC_TO_LONG('_','S','T','P')
#define CMD_PAUSE		FOURCC_TO_LONG('_','P','S','E')
#define CMD_CONTINUE	FOURCC_TO_LONG('_','C','N','T')


// internal structure of plugin
typedef struct
{
// public data
	char* s_filename;	// avi file name
	long L_center;		// centering of image
	long l_width;		// _DEFined width of image
	long l_height;		// _DEFined height of image
	float f_speed;		// speed of playing
	long l_loop;		// flag indicated that loop playing is active
	float f_position;	// current frame (0.0 is first)
	long l_play;		// indicate autoplay state

// trigger events for online control
	long l_trigger_cmd;	// _PLY - start PLAying from begining
						// _STP - SToP and rewind to begin
						// _PSE - PauSE playing
						// _CNT - CoNTinue playing

// internal datas
	char* _filename;		// mirror pointer to filename
	HANDLE _lock_update;	// update struct mutex
	AVISTREAMINFO *_strhdr;	// stream definition struct
	PGETFRAME _pgf;
	PAVISTREAM _avi_stream;

	unsigned long _current_frame;	// pointer on AVI frame
	unsigned long _global_frame;	// global frame no

	unsigned int _texture;
	unsigned int _texture_generated;
	unsigned int _texture_initialized;

	long _width;
	long _height;
	long _base_width;
	long _base_height;

} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
	NULL,			// char* s_filename;	// avi file name
	GEOM_CENTER_CM,	// long L_center;		// centering of image
	0,				// long l_width;		// _DEFined width of image
	0,				// long l_height;		// _DEFined height of image
	1.0f,			// float f_speed;		// speed of playing
	0,				// long l_loop;		// flag indicated that loop playing is active
	0.0f,			// float f_position;	// current frame (0 is first)
	0,				// long l_play;		// indicate autoplay state

// trigger events for online control
	0,				// long l_trigger_cmd:	
						// _PLY - start PLAying from begining
						// _STP - SToP and rewind to begin
						// _PSE - PauSE playing
						// _CNT - CoNTinue playing

// internal datas
	NULL,			// char* _filename;		// mirror pointer to filename
	NULL,			// HANDLE _lock_update;	// update struct mutex
	NULL,			// AVISTREAMINFO *_strhdr;	// stream definition struct
	NULL,			// PGETFRAME _pgf;
	NULL,			// PAVISTREAM _avi_stream;
	0,				// unsigned long _current_frame;
	0,				// unsigned long _global_frame;	// global frame no

	0,				// unsigned int _texture;
	0,				// unsigned int _texture_generated;
	0,				// unsigned int _texture_initialized;

	0,				// long _width;
	0,				// long _height;
	0,				// long _pot_width;
	0				// long _pot_height;
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"s_filename",		"avi file name",
						PLUGIN_PARAMETER_OFFSET(default_value,s_filename)},

	{"L_center",		"centering of image",
						PLUGIN_PARAMETER_OFFSET(default_value,L_center)},

	{"f_speed",			"speed of playing",
						PLUGIN_PARAMETER_OFFSET(default_value,f_speed)},

	{"l_loop",			"flag indicated that loop playing is active",
						PLUGIN_PARAMETER_OFFSET(default_value,l_loop)},

	{"f_position",		"current frame (0.0 is first)",
						PLUGIN_PARAMETER_OFFSET(default_value,f_position)},

	{"l_play",			"indicate autoplay state",
						PLUGIN_PARAMETER_OFFSET(default_value,l_play)},

	{"l_trigger_cmd",	"trigger events for online control: _PLY - start PLAying from begining,	_STP - SToP and rewind to begin, _PSE - PauSE playing, _CNT - CoNTinue playing",
						PLUGIN_PARAMETER_OFFSET(default_value,l_trigger_cmd)},
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
	// try to lock struct
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	// check if texture initialized
	if(_DATA->_texture_generated)
		glDeleteTextures (1, &(_DATA->_texture));

	// delete avi info block
	if(_DATA->_strhdr)
		free(_DATA->_strhdr);

	if(_DATA->_pgf)
		AVIStreamGetFrameClose(_DATA->_pgf);

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


	// check if avi opened
	if(!(_DATA->_pgf))
	{
		// release mutex
		ReleaseMutex(_DATA->_lock_update);
		return;
	};

	// test if texture is generated
	if (_DATA->_texture_generated == 0)
	{
		glGenTextures(1, &_DATA->_texture);
		_DATA->_texture_generated = 1;
	};


	// calculate frame to load
	if(_DATA->l_play)
	{
		// avi in auto play state

		// fix previous global frame number at first run
		if(_DATA->_global_frame == 0xffffffff)
			_DATA->_global_frame = session->frame;
//fprintf(stderr,"AUTOPLAY\n\t_DATA->f_speed=%f\n\tsession->frame=%ld\t\nsession->field=%ld\t\n_DATA->_global_frame=%ld \n",_DATA->f_speed,session->frame,session->field,_DATA->_global_frame);
		// find delta frame from prev to current
		float delta = _DATA->f_speed * (session->frame - _DATA->_global_frame  + 0.5f*session->field);
		_DATA->f_position += delta;

//fprintf(stderr,"delta=%f\n",delta);
	};

//	_DATA->f_position = (long)(_DATA->f_position) % _DATA->_strhdr->dwLength;

	long frame = (long)_DATA->f_position;

//fprintf(stderr,"_DATA->f_position=%f, frame=%ld\n",_DATA->f_position, frame);


	// load frame
	if
	(
		(_DATA->_current_frame == 0xffffffff)
		||
		(_DATA->_current_frame != frame)
	)
	{
		void* frame_head = AVIStreamGetFrame(_DATA->_pgf, frame); 
		LPBITMAPINFOHEADER frame_info = (LPBITMAPINFOHEADER)frame_head;
		void* frame_data = ((unsigned char*)frame_head) + frame_info->biSize;
//ERROR_LOG("prerender","AVIStreamGetFrame OK");
		// test if texture initilized!	
		if(!(_DATA->_texture_initialized))
		{
			// determinate used width and height
			_DATA->_base_height = frame_info->biHeight;
			_DATA->_base_width = frame_info->biWidth;

			// calculate PowerOfTwo dimensions of image
			_DATA->_height = POT(_DATA->_base_height);
			_DATA->_width = POT(_DATA->_base_width);

			// generate fake surface
			void* fake_frame = malloc(4*_DATA->_width*_DATA->_height);
			memset(fake_frame,0,4*_DATA->_width*_DATA->_height);

			// create texture (init texture memory)
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
			// free memory of fake image
			free(fake_frame);

			// mark flag
			_DATA->_texture_initialized = 1;
		};


		// load subimage into texture
#define GL_BGR                            0x80E0
		glBindTexture(GL_TEXTURE_2D, _DATA->_texture);
		glTexSubImage2D
		(
			GL_TEXTURE_2D,			// GLenum target,
			0,						// GLint level,
			(_DATA->_width - _DATA->_base_width)/2, // GLint xoffset,
			(_DATA->_height - _DATA->_base_height)/2,	// GLint yoffset,
			_DATA->_base_width,		// GLsizei width,
			_DATA->_base_height,	// GLsizei height,
			(frame_info->biBitCount == 32)?GL_BGRA_EXT:GL_BGR,	// GLenum format,
			GL_UNSIGNED_BYTE,		// GLenum type,
			frame_data				// const GLvoid *pixels 
		);
		_DATA->_current_frame = frame;
		_DATA->_global_frame = session->frame;
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
	if((_DATA->_texture_initialized)&&(_DATA->_width != 0)&&(_DATA->_height != 0))
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
	};
};

char* _avi_err(long err)
{
	switch(err)
	{
		case AVIERR_BADFORMAT:		return "The file couldn't be read, indicating a corrupt file or an unrecognized format.";break;
		case AVIERR_MEMORY:			return "The file could not be opened because of insufficient memory.";break;
		case AVIERR_FILEREAD:		return "A disk error occurred while reading the file.";break;
		case AVIERR_FILEOPEN:		return "A disk error occurred while opening the file.";break;
		case REGDB_E_CLASSNOTREG:	return "According to the registry, the type of file specified in AVIFileOpen does not have a handler to process it.";break;
		case AVIERR_NODATA:			return "The file does not contain a stream corresponding to the values of fccType and lParam";break;
		default:					return "[unknown?]"; break;
	};
};

PLUGIN_EXPORT void notify(void* data)
{
	// check if pointer to filenames is different?
	if(_DATA->s_filename != _DATA->_filename)
	{
		//wait for mutext free
		WaitForSingleObject(_DATA->_lock_update,INFINITE);

		//  check if privious opened

		// delete avi info block
		if(_DATA->_strhdr)
			free(_DATA->_strhdr);
		_DATA->_strhdr = NULL;

		// close frame
		if(_DATA->_pgf)
			AVIStreamGetFrameClose(_DATA->_pgf);
		_DATA->_pgf = NULL;

		// sync name
		_DATA->_filename = _DATA->s_filename;

		// result of operations
		HRESULT hr;
	
		// open stream
		hr = AVIStreamOpenFromFile
		(
			&_DATA->_avi_stream,	//	PAVISTREAM * ppavi,    
			_DATA->_filename,		//	LPCTSTR szFile,        
			streamtypeVIDEO,		//	DWORD fccType,         
			0,						//	LONG lParam,           
			OF_READ,				//	UINT mode,             
			NULL					//	CLSID * pclsidHandler  
		);

fprintf(stderr,"AVIStreamOpenFromFile = %ld, _DATA->_avi_stream = %ld\n",hr,_DATA->_avi_stream);

		// check if was error for opening stream from file
		if(hr != 0)
		{
			printf("Error! 	'AVIStreamOpenFromFile': %s\n",_avi_err(hr));
			_DATA->_avi_stream = NULL;
			return;
		};

		_DATA->_strhdr = (AVISTREAMINFO*)malloc(sizeof(AVISTREAMINFO));
		memset(_DATA->_strhdr, 0, sizeof(AVISTREAMINFO));
		
		// request stream info
		hr = AVIStreamInfo
		(
			_DATA->_avi_stream,			// PAVISTREAM pavi,
			_DATA->_strhdr,				// AVISTREAMINFO * psi,
			sizeof(AVISTREAMINFO)		// LONG lSize
		);				

fprintf(stderr,"AVIStreamInfo = %ld, _strhdr = %ld\n",hr,_DATA->_strhdr);

		// check if was error happen
		if(hr != 0)
		{
			printf("Error! 	'AVIStreamInfo': %s\n",_avi_err(hr));
			free(_DATA->_strhdr);
			_DATA->_strhdr = NULL;
			_DATA->_avi_stream = NULL;
			return;
		};

		// frame open pointer
		// choose NULL for format
		_DATA->_pgf = AVIStreamGetFrameOpen
		(
			_DATA->_avi_stream,			// PAVISTREAM pavi,
			NULL						// LPBITMAPINFOHEADER lpbiWanted  
		);

		if (_DATA->_pgf == NULL)
		{
			printf("Error! 	'AVIStreamGetFrameOpen'\n",_avi_err(hr));
			free(_DATA->_strhdr);
			_DATA->_strhdr = NULL;
			_DATA->_avi_stream = NULL;
			return;
		};

		// seems avi file opens fine!

		// set flag to force 'glTexImage2D'
		_DATA->_texture_initialized = 0;

		// mark frame counter not loaded
		_DATA->_current_frame = 0xFFFFFFFF;

		
		// release mutex -  let created tread work
		ReleaseMutex(_DATA->_lock_update);
	};
};

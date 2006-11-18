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
	2006-11-18:
		*memory preloading clips
		*texture flipping flags

	2006-11-16:
		*Compleatly rewriting code with asyncronous reader.
		*'CoInitializeEx' usage to avoid COM uninitilizes answer from AVI* 
		functions (CO_E_NOTINITIALIZED returns) in multithreaded inviroment.

	2005-06-30:
		*Draft Version

*/

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"
#include "../vz/plugin-procs.h"

#include <stdio.h>
#include <windows.h>
#include <vfw.h>

#define RING_BUFFER_LENGTH 10
#define MAX_AVI_LOADERS 5

//#define VERBOSE

/*#define _WIN32_DCOM
#define _WIN32_WINNT 0x0400
#include <objbase.h>
*/
#define COINIT_MULTITHREADED 0
WINOLEAPI  CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);
#pragma comment(lib, "ole32.LIB")


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
			break;
		case DLL_THREAD_ATTACH:
			// init avi lib !
			AVIFileInit();
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
	"rc8"
};

#define CMD_PLAY		FOURCC_TO_LONG('_','P','L','Y')
#define CMD_STOP		FOURCC_TO_LONG('_','S','T','P')
#define CMD_PAUSE		FOURCC_TO_LONG('_','P','S','E')
#define CMD_CONTINUE	FOURCC_TO_LONG('_','C','N','T')

/* ------------------------------------------------------------------------- */

#define GL_BGR                            0x80E0
struct aviloader_desc
{
	int flag_ready;
	int flag_exit;
	int flag_gone;
	int flag_mem_preload;

	char filename[128];					/* file name */

	long frames_count;
	long width;
	long height;
	long bpp;

	HANDLE lock;						/* struct update lock */
	HANDLE task;						/* thread id */
	HANDLE wakeup;						/* wakeup signal */

	void** buf_data;					/* buffers */
	int* buf_frame;
	int* buf_clear;
	int* buf_fill;
	int* buf_filled;
	long cursor;
};

static char* avi_err(long err)
{
	switch(err)
	{
		case AVIERR_BADFORMAT:		return "The file couldn't be read, indicating a corrupt file or an unrecognized format.";
		case AVIERR_MEMORY:			return "The file could not be opened because of insufficient memory.";
		case AVIERR_FILEREAD:		return "A disk error occurred while reading the file.";
		case AVIERR_FILEOPEN:		return "A disk error occurred while opening the file.";
		case REGDB_E_CLASSNOTREG:	return "According to the registry, the type of file specified in AVIFileOpen does not have a handler to process it.";
		case AVIERR_NODATA:			return "The file does not contain a stream corresponding to the values of fccType and lParam";
		case CO_E_NOTINITIALIZED:	return "CO_E_NOTINITIALIZED";
		default:					return "not known error";
	};
};

static unsigned long WINAPI aviloader_proc(void* p)
{
	HRESULT hr;
	AVISTREAMINFO *strhdr;		/* AVI stream definition structs */
	PGETFRAME pgf;
	PAVISTREAM avi_stream;
	void* frame_head;
	void* frame_data;
	int frame_size = 0;
	LPBITMAPINFOHEADER frame_info;
	int i,l;

#ifdef VERBOSE
	printf("avifile: aviloader_proc started\n");
#endif /* VERBOSE */

	/* cast struct */
	struct aviloader_desc* desc = (struct aviloader_desc*)p;

	/* init AVI for this thread */
	CoInitializeEx( NULL, COINIT_MULTITHREADED );
	AVIFileInit();

	/* open avi file stream */
	if
	(
		0
		==
		(hr = AVIStreamOpenFromFile
			(
				&avi_stream,			//	PAVISTREAM * ppavi,    
				desc->filename,			//	LPCTSTR szFile,        
				streamtypeVIDEO,		//	DWORD fccType,         
				0,						//	LONG lParam,           
				OF_READ,				//	UINT mode,             
				NULL					//	CLSID * pclsidHandler  
			)
		)
	)
	{
		/* prepare struct for stream header */
		strhdr = (AVISTREAMINFO*)malloc(sizeof(AVISTREAMINFO));
		memset(strhdr, 0, sizeof(AVISTREAMINFO));

		/* request frames count */
		if((desc->frames_count = AVIStreamLength(avi_stream)) < 0)
			desc->frames_count = 0;
		
		/* request stream info */
		if
		(
			0
			==
			(hr = AVIStreamInfo
				(
					avi_stream,				// PAVISTREAM pavi,
					strhdr,					// AVISTREAMINFO * psi,
					sizeof(AVISTREAMINFO)	// LONG lSize
				)
			)
		)
		{
			if
			(
				NULL
				!=
				(pgf = AVIStreamGetFrameOpen
					(
						avi_stream,			// PAVISTREAM pavi,
						NULL				// LPBITMAPINFOHEADER lpbiWanted  
					)
				)
			)
			{
				/* test load  */
				frame_head = AVIStreamGetFrame(pgf, 0);
				frame_info = (LPBITMAPINFOHEADER)frame_head;
				frame_data = ((unsigned char*)frame_head) + frame_info->biSize;

				/* check */
				if(NULL != frame_head)
				{
					/* setup width, height, buffer type */
					desc->width = frame_info->biWidth;
					desc->height = frame_info->biHeight;
					switch(frame_info->biBitCount)
					{
						case 32: desc->bpp = GL_BGRA_EXT; break;
						case 24: desc->bpp = GL_BGR; break;
					};

					/* allocate space for buffers */
					desc->buf_data = (void**)malloc(sizeof(void*) * ((desc->flag_mem_preload)?desc->frames_count:RING_BUFFER_LENGTH));
					desc->buf_frame = (int*)malloc(sizeof(int) * ((desc->flag_mem_preload)?desc->frames_count:RING_BUFFER_LENGTH));
					desc->buf_clear = (int*)malloc(sizeof(int) * ((desc->flag_mem_preload)?desc->frames_count:RING_BUFFER_LENGTH));
					desc->buf_fill = (int*)malloc(sizeof(int) * ((desc->flag_mem_preload)?desc->frames_count:RING_BUFFER_LENGTH));
					desc->buf_filled = (int*)malloc(sizeof(int) * ((desc->flag_mem_preload)?desc->frames_count:RING_BUFFER_LENGTH));
					for(i = 0; i<((desc->flag_mem_preload)?desc->frames_count:RING_BUFFER_LENGTH); i++)
					{
						/* init buffer */
						frame_size = frame_info->biSize + 
							(frame_info->biWidth + 4)* frame_info->biHeight * (frame_info->biBitCount / 8);
						desc->buf_data[i] = malloc( frame_size );
						memset(desc->buf_data[i], 0, frame_size);
		
						/* setup start plan */
						if(i < desc->frames_count)
						{
							desc->buf_frame[i] = i;
							desc->buf_fill[i] = 1;
							desc->buf_clear[i] = 1;
						};
					};

					/* "endless" loop */
					while(!(desc->flag_exit))
					{
						/* reset jobs counter */
						l = 0;

						/* check if frames should be loaded */
						for
						(
							i = 0; 
							(i<((desc->flag_mem_preload)?desc->frames_count:RING_BUFFER_LENGTH))
							&& 
							(!(desc->flag_exit)); 
							i++
						)
							if
							(
								(1 == desc->buf_clear[i])		/* frame is clear */
								&&								/* and */
								(1 == desc->buf_fill[i])		/* should be loaded */
							)
							{
								int f = desc->buf_frame[i];
								if(f < desc->frames_count) /* frame in range */
								{
									if(NULL != (frame_head = AVIStreamGetFrame(pgf, f)))
									{
										l++;						/* increment jobs counter */
										Sleep(0);					/* allow context switch */
										frame_info = (LPBITMAPINFOHEADER)frame_head;
										memcpy
										(
											desc->buf_data[i], 
											((unsigned char*)frame_head) + frame_info->biSize, 
											frame_size - frame_info->biSize
										);							/* copy frame */
										Sleep(0);					/* allow context switch */
										/* setup flags */
										desc->buf_fill[i] = 0;
										desc->buf_clear[i] = 0;
										desc->buf_filled[i] = f + 1;
#ifdef VERBOSE
										printf("avifile: loaded frame %d into slot %d\n", f, i);
#endif /* VERBOSE */
									}
									else
									{
										/* setup flags */
										desc->buf_fill[i] = 0;
										printf("avifile: WARNING! AVIStreamGetFrame('%d') == NULL\n", desc->buf_frame[i]);
									}
								}
								else
								{
									/* setup flags */
									desc->buf_fill[i] = 0;
									printf("avifile: WARNING! desc->buf_frame[%d] >= %d\n", desc->buf_frame[i], i, desc->frames_count);
								};
							};

						/* rise flag about ready */
						desc->flag_ready = 1;

						/* no more in mem proload mode */
						if((desc->flag_mem_preload)&&(0 == desc->flag_exit))
						{
							desc->flag_exit = 2;
						};

						/* wait for signal if no jobs done */
						if( 0 == l) WaitForSingleObject(desc->wakeup, 40);
						ResetEvent(desc->wakeup);
					};

					/* wait here until read exit flag came */
					if
					(
						(desc->flag_mem_preload)
						&&
						(2 == desc->flag_exit)
					)
					{
#ifdef VERBOSE
						printf("avifile: aviloader_proc waiting for real f_exit\n");
#endif /* VERBOSE */

						desc->flag_exit = 0;
						while(!(desc->flag_exit))
						{
							WaitForSingleObject(desc->wakeup, 40);
							ResetEvent(desc->wakeup);
						};
					};

					/* free buffers */
					for(i = 0; i<((desc->flag_mem_preload)?desc->frames_count:RING_BUFFER_LENGTH); i++)
						free(desc->buf_data[i]);
					free(desc->buf_data);
					free(desc->buf_frame);
					free(desc->buf_clear);
					free(desc->buf_fill);
					free(desc->buf_filled);
				}
				else
				{
					printf("avifile: ERROR! probe AVIStreamGetFrame(0) == NULL\n");
				};

				/* close frame pointer */
				AVIStreamGetFrameClose(pgf);
			}
			else
			{
				printf("avifile: ERROR! AVIStreamGetFrameOpen() == NULL\n");
			};
		}
		else
		{
			printf("avifile: ERROR! AVIStreamInfo() == 0x%.8X [%s]\n", hr, avi_err(hr));
		};

		/* close avi stream */
		AVIStreamClose(avi_stream);

		/* free stream header info struct */
		free(strhdr);
	}
	else
	{
		printf("avifile: ERROR! AVIStreamOpenFromFile('%s') == 0x%.8X [%s]\n", desc->filename, hr, avi_err(hr));
	};

	/* setup flag of exiting */
	desc->flag_gone = 1;

#ifdef VERBOSE
	printf("avifile: aviloader_proc exiting\n");
#endif /* VERBOSE */


	/* exit thread */
	return 0;
};

static struct aviloader_desc* aviloader_init(char* filename, int mem_preload)
{
	unsigned long thread_id;

	/* allocate and clear struct */
	struct aviloader_desc* desc = (struct aviloader_desc*)malloc(sizeof(struct aviloader_desc));
	memset(desc, 0, sizeof(struct aviloader_desc));

	/* copy argument */
	strcpy(desc->filename, filename);
	desc->flag_mem_preload = mem_preload;

	/* init events */
	desc->wakeup = CreateEvent(NULL, TRUE, FALSE, NULL);
	ResetEvent(desc->wakeup);

	/* init mutexes */
	desc->lock = CreateMutex(NULL,FALSE,NULL);

	/* run thread */
	desc->task = CreateThread(0, 0, aviloader_proc, desc, 0, &thread_id);

	return desc;
};


static void aviloader_destroy(struct aviloader_desc* desc)
{
	/* check if struct is not dead */
	if(NULL == desc) return;

	/* rise flag */
	desc->flag_exit = 1;

	/* wait for thread finish */
	WaitForSingleObject(desc->task, INFINITE);

	/* close all */
	CloseHandle(desc->task);
	CloseHandle(desc->lock);
	CloseHandle(desc->wakeup);

	/* free mem */
	free(desc);
};


/* ------------------------------------------------------------------------- */


// internal structure of plugin
typedef struct
{
// public data
	char* s_filename;		// avi file name
	long L_center;			// centering of image
	long l_loop;			// flag indicated that loop playing is active
	long l_field_mode;		// field incrementator
	long l_start_frame;		// start frame
	long l_auto_play;		// indicate autoplay state
	long l_flip_v;			/* flip vertical flag */
	long l_flip_h;			/* flip vertical flag */
	long l_mem_preload;		/* preload whole clip in memmory */

// trigger events for online control
	long l_trig_play;		/* play from beginning */
	long l_trig_cont;		/* continie play from current position */
	long l_trig_stop;		/* stop/pause playing */
	long l_trig_jump;		/* jump to desired position */

// internal datas
	char* _filename;		// mirror pointer to filename
	HANDLE _lock_update;	// update struct mutex
	long _playing;			/* flags indicate PLAY state */
	long _current_frame;	// pointer on AVI frame
	long _cursor_loaded;	/* previous cursor - detect to upload texture */
	struct aviloader_desc** _loaders;
	unsigned int _texture;
	unsigned int _texture_initialized;
	long _width;
	long _height;

} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
	NULL,					// char* s_filename;		// avi file name
	GEOM_CENTER_CM,			// long L_center;			// centering of image
	0,						// long l_loop;				// flag indicated that loop playing is active
	0,						// long l_field_mode;		// field incrementator
	0,						// long l_start_frame;		// start frame
	0,						// long l_auto_play;		// indicate autoplay state
	0,						// long l_flip_v;			/* flip vertical flag */
	0,						// long l_flip_h;			/* flip vertical flag */
	0,						// long l_mem_preload;		/* preload whole clip in memmory */

// trigger events for online control
	0,						// long l_trig_play;		/* play from beginning */
	0,						// long l_trig_cont;		/* continie play from current position */
	0,						// long l_trig_stop;		/* stop/pause playing */
	0xFFFFFFF,				// long l_trig_jump;		/* jump to desired position */

// internal datas
	NULL,					// char* _filename;		// mirror pointer to filename
	INVALID_HANDLE_VALUE,	// HANDLE _lock_update;	// update struct mutex
	0,						// long _playing;			/* flags indicate PLAY state */
	0,						// long _current_frame;	// pointer on AVI frame
	0,						// long _cursor_loaded;	/* previous cursor - detect to upload texture */
	NULL,					// struct aviloader_desc** _loaders;
	0,						// unsigned int _texture;
	0,						// unsigned int _texture_initialized;
	0,						// long _width;
	0						// long _height;
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"s_filename",		"avi file name",
						PLUGIN_PARAMETER_OFFSET(default_value,s_filename)},

	{"L_center",		"centering of image",
						PLUGIN_PARAMETER_OFFSET(default_value,L_center)},

	{"l_loop",			"flag indicated that loop playing is active",
						PLUGIN_PARAMETER_OFFSET(default_value,l_loop)},

	{"l_field_mode",	"field mode frame counter",
						PLUGIN_PARAMETER_OFFSET(default_value,l_field_mode)},

	{"l_start_frame",	"start frame number",
						PLUGIN_PARAMETER_OFFSET(default_value,l_start_frame)},

	{"l_auto_play",		"indicate autoplay state",
						PLUGIN_PARAMETER_OFFSET(default_value,l_auto_play)},

	{"l_trig_play",		"start playing (from beginning) trigger",
						PLUGIN_PARAMETER_OFFSET(default_value,l_trig_play)},

	{"l_trig_cont",		"continue playing from current position",
						PLUGIN_PARAMETER_OFFSET(default_value,l_trig_cont)},

	{"l_trig_stop",		"stop/pause playing",
						PLUGIN_PARAMETER_OFFSET(default_value,l_trig_stop)},

	{"l_trig_jump",		"jump to specified frame position",
						PLUGIN_PARAMETER_OFFSET(default_value,l_trig_jump)},

	{"l_flip_v",		"flag to vertical flip",
						PLUGIN_PARAMETER_OFFSET(default_value,l_flip_v)},

	{"l_flip_h",		"flag to horozontal flip",
						PLUGIN_PARAMETER_OFFSET(default_value,l_flip_h)},

	{"l_mem_preload",	"flag to vertical flip",
						PLUGIN_PARAMETER_OFFSET(default_value,l_mem_preload)},

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

	/* create loaders array */
	_DATA->_loaders = (struct aviloader_desc**)malloc(MAX_AVI_LOADERS * sizeof(struct aviloader_desc*));
	memset(_DATA->_loaders, 0, MAX_AVI_LOADERS * sizeof(struct aviloader_desc*));

	// return pointer
	return data;
};

PLUGIN_EXPORT void destructor(void* data)
{
	int i;

	// try to lock struct
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	// check if texture initialized
	if(_DATA->_texture_initialized)
		glDeleteTextures (1, &(_DATA->_texture));

	/* destroy loaders */
	for(i = 0; i<MAX_AVI_LOADERS; i++)
		if(_DATA->_loaders[i])
		{
			aviloader_destroy(_DATA->_loaders[i]);
			_DATA->_loaders[i] = NULL;
		};
	free(_DATA->_loaders);

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
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	/* check if we need to init/reinit texture */
	if
	(
		(_DATA->_loaders[0])									/* current loader is defined */
		&&														/* and */
		(_DATA->_loaders[0]->flag_ready)						/* pipeline theoreticaly ready */
	)
	{
		/* pipeline ready - check texture */
		if
		(
			(_DATA->_width != POT(_DATA->_loaders[0]->width))
			||
			(_DATA->_height != POT(_DATA->_loaders[0]->height))
		)
		{
			/* texture should be (re)initialized */

			if(_DATA->_texture_initialized)
				glDeleteTextures (1, &(_DATA->_texture));

			/* set flags */
			_DATA->_width = POT(_DATA->_loaders[0]->width);
			_DATA->_height = POT(_DATA->_loaders[0]->height);
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
#ifdef VERBOSE
			printf("avifile: reinitialized texture %dx%d\n", _DATA->_width, _DATA->_height);
#endif /* VERBOSE */

		};

		/* load new texture , if it ready*/
		if
		(
			(_DATA->_cursor_loaded != _DATA->_loaders[0]->cursor)
			&&
			(_DATA->_loaders[0]->buf_filled[ _DATA->_loaders[0]->cursor ] )
		)
		{
			/* load */
			glBindTexture(GL_TEXTURE_2D, _DATA->_texture);
			glTexSubImage2D
			(
				GL_TEXTURE_2D,									// GLenum target,
				0,												// GLint level,
				(_DATA->_width - _DATA->_loaders[0]->width)/2,	// GLint xoffset,
				(_DATA->_height - _DATA->_loaders[0]->height)/2,// GLint yoffset,
				_DATA->_loaders[0]->width,						// GLsizei width,
				_DATA->_loaders[0]->height,						// GLsizei height,
				_DATA->_loaders[0]->bpp,						// GLenum format,
				GL_UNSIGNED_BYTE,								// GLenum type,
				_DATA->_loaders[0]->buf_data[ _DATA->_loaders[0]->cursor ]	// const GLvoid *pixels 
			);

			/* sync cursor value */
			_DATA->_cursor_loaded = _DATA->_loaders[0]->cursor;

			/* setup flag is clear for current frame */
			_DATA->_loaders[0]->buf_clear[ _DATA->_loaders[0]->cursor ] = 1;

#ifdef VERBOSE
			printf("avifile: loaded texture frame %d from slot %d\n", _DATA->_loaders[0]->buf_filled[ _DATA->_loaders[0]->cursor ], _DATA->_loaders[0]->cursor);
#endif /* VERBOSE */
		};

	};

	// release mutex
	ReleaseMutex(_DATA->_lock_update);
};

PLUGIN_EXPORT void postrender(void* data,vzRenderSession* session)
{
	int i, t, c, f;
	// try to lock struct
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	/* check mode */
	if
	(
		/* we increment number diff in frame and field mode */
		(
			(_DATA->l_field_mode)								/* if we are rendering by field */
			||													/* or */
			(
				(!(_DATA->l_field_mode))						/* if we are rendering by frame */
				&&												/* and */
				(0 == session->field)							/* current field is upper */
			)
		)
		&&														/* and */
		/* async pipeline is ready */
		(
			(_DATA->_loaders[0])								/* current loader is defined */
			&&													/* and */
			(_DATA->_loaders[0]->flag_ready)					/* pipeline theoreticaly ready */
		)
	)
	{

		if(_DATA->l_mem_preload)
		{
			/* we are in memory preload mode */

			/* increment current frame if we are in playing mode */
			if(_DATA->_playing)
				_DATA->_current_frame++;

			/* check range */
			if(_DATA->_current_frame >= _DATA->_loaders[0]->frames_count)
			{
				/* check loop mode */
				if(_DATA->l_loop)
					_DATA->_current_frame = 0;
				else
				{
					/* set to last frame */
					_DATA->_current_frame = _DATA->_loaders[0]->frames_count - 1;

					/* exit play mode */
					_DATA->_playing = 0;
				};
			};

			/* sync cursor to frame number */
			_DATA->_loaders[0]->cursor = _DATA->_current_frame;
		}
		else
		{
			/* we are in async load mode */

			/* plan frames order in pipeline */
			for(i = 0, t = 0; i<((RING_BUFFER_LENGTH*3)/4) ; i++)
			{
				/* calc cursor and frame number */
				c = (_DATA->_loaders[0]->cursor + i) % RING_BUFFER_LENGTH;
				f = (_DATA->_current_frame + i) % _DATA->_loaders[0]->frames_count;

				/* check if position planned */
				if(_DATA->_loaders[0]->buf_frame[c] != f)
				{
					t++;
					_DATA->_loaders[0]->buf_frame[c] = f;
					_DATA->_loaders[0]->buf_fill[c] = 1;
					_DATA->_loaders[0]->buf_filled[c] = 0;
					_DATA->_loaders[0]->buf_clear[c] = 1;
				};
			};
			if(t) PulseEvent(_DATA->_loaders[0]->wakeup);
		
			/* try shift cursor */
			c = (_DATA->_loaders[0]->cursor + 1 + RING_BUFFER_LENGTH) % RING_BUFFER_LENGTH;
			if
			(
				(_DATA->_cursor_loaded == _DATA->_loaders[0]->cursor) /* current frame loaded */
				&&													/* and */
				(_DATA->_playing)									/* we are in playing mode */
				&&													/* and */
				(0 != _DATA->_loaders[0]->buf_filled[c])			/* frame is loaded */
			)
			{
#ifdef VERBOSE
				printf("avifile: trying to shift cursor\n");
#endif /* VERBOSE */

				/* determinate how shift frame position and cursor */
				if((_DATA->_current_frame + 1)< _DATA->_loaders[0]->frames_count)
				{
					_DATA->_current_frame++;
					_DATA->_loaders[0]->cursor = c;
#ifdef VERBOSE
					printf("avifile: shifted to current_frame=%d, _DATA->_loaders[0]->cursor=%d\n", _DATA->_current_frame, _DATA->_loaders[0]->cursor);
#endif /* VERBOSE */
				}
				else
				{
					if(_DATA->l_loop)
					{
						/* jump to first frame */
						_DATA->_current_frame = 0;
						_DATA->_loaders[0]->cursor = c;
#ifdef VERBOSE
						printf("avifile: loop condition detected\n");
#endif /* VERBOSE */

					}
					else
					{
						/* stop playing */
						_DATA->_playing = 0;
#ifdef VERBOSE
						printf("avifile: end of file detected\n");
#endif /* VERBOSE */
					};
				};
			}
			else
			{
#ifdef VERBOSE2
				printf("avifile: no condition to shift cursor\n");
#endif /* VERBOSE */

#ifdef VERBOSE2
			if(!(0 != _DATA->_loaders[0]->buf_filled[c]))
				printf("avifile: _DATA->_loaders[0]->buf_filled[%d] == %d\n", c, _DATA->_loaders[0]->buf_filled[c]);
#endif /* VERBOSE */

			};
		};
	};

	// release mutex
	ReleaseMutex(_DATA->_lock_update);

};

PLUGIN_EXPORT void render(void* data,vzRenderSession* session)
{
	// check if texture initialized
	if
	(
		(_DATA->_texture_initialized)
		&&
		(_DATA->_loaders[0])
		&&
		(_DATA->_loaders[0]->flag_ready)
	)
	{
		// determine center offset 
		float co_X = 0.0f, co_Y = 0.0f, co_Z = 0.0f;

		// translate coordinates accoring to base image
		center_vector(_DATA->L_center, _DATA->_loaders[0]->width, _DATA->_loaders[0]->height, co_X, co_Y);

#ifdef VERBOSE2
		printf("avifile: center_vector: co_X = %f, co_Y = %f\n", co_X, co_Y);
#endif /* VERBOSE */

		// translate coordinate according to real image
		co_Y -= (_DATA->_height - _DATA->_loaders[0]->height)/2;
		co_X -= (_DATA->_width - _DATA->_loaders[0]->width)/2;

#ifdef VERBOSE2
		if(_DATA->_playing)
			printf
			(
				"avifile: translate: co_X = %f, co_Y = %f "
				"(_DATA->_loaders[0]->width=%d, _DATA->_loaders[0]->height=%d\n", 
				co_X, co_Y,
				_DATA->_loaders[0]->width, _DATA->_loaders[0]->height
			); 
#endif /* VERBOSE */

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

PLUGIN_EXPORT void notify(void* data)
{
	//wait for mutext free
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	// check if pointer to filenames is different?
	if(_DATA->s_filename != _DATA->_filename)
	{
		/* change current frame position*/
		_DATA->_current_frame = _DATA->l_start_frame;
		_DATA->_cursor_loaded = 0xFFFFFFFF;
		_DATA->_playing = _DATA->l_auto_play;

		/* create loader */
		struct aviloader_desc* loader = aviloader_init(_DATA->s_filename, _DATA->l_mem_preload);
		_DATA->_filename = _DATA->s_filename;

		/* check second slot */
		if(_DATA->_loaders[1])
		{
			aviloader_destroy(_DATA->_loaders[1]);
			_DATA->_loaders[1] = NULL;
		};

		/* shift loaders */
		_DATA->_loaders[1] = _DATA->_loaders[0];
		_DATA->_loaders[0] = loader;

		/* notify old loader to die */
		if(_DATA->_loaders[1])
			_DATA->_loaders[1]->flag_exit = 1;
	};

	/* control triggers */
	if(_DATA->l_trig_play)
	{
		_DATA->_current_frame = _DATA->l_start_frame;
		_DATA->_cursor_loaded = 0xFFFFFFFF;
		_DATA->_playing = 1;

		if(_DATA->l_mem_preload)
			_DATA->_loaders[0]->cursor = _DATA->_current_frame;

		/* reset trigger */
		_DATA->l_trig_play = 0;
	};
	if(_DATA->l_trig_cont)
	{
		_DATA->_playing = 1;

		/* reset trigger */
		_DATA->l_trig_cont = 0;
	};
	if(_DATA->l_trig_stop)
	{
		_DATA->_playing = 0;

		/* reset trigger */
		_DATA->l_trig_stop = 0;
	};
	if(0xFFFFFFFF != _DATA->l_trig_jump)
	{
		if
		(
			(_DATA->_loaders[0])
			&&
			(_DATA->_loaders[0]->flag_ready)
		)
		{
			/* loader ready */

			/* check range */
			if(_DATA->l_trig_jump < 0)
				_DATA->_current_frame = 0;
			else if (_DATA->l_trig_jump >= _DATA->_loaders[0]->frames_count)
				_DATA->_current_frame = _DATA->_loaders[0]->frames_count - 1;
			else
				_DATA->_current_frame = _DATA->l_trig_jump;

			/* sync cursor */
			if(_DATA->l_mem_preload)
				_DATA->_loaders[0]->cursor = _DATA->_current_frame;
		};
		
		/* reset trigger */
		_DATA->l_trig_jump = 0xFFFFFFFF;
	};

	// release mutex -  let created tread work
	ReleaseMutex(_DATA->_lock_update);

};

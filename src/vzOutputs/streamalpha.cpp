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
    2008-09-24:
        *logger use for message outputs

	2006-12-02:
		*New scheme of buffers config.

	2006-11-29:
		*Output/Input buffers described by the same data block

	2006-11-28:
		*Output buffers paramters aquired from output driver.

	2005-07-09:
		*Transformation function turned off. Update of framebuffer provided 
		directly into native PLANAR format. New MMX optimized function for 
		transcoding of AGBR into planar-YUV added.

	2005-07-06:
		*Added syncronization function, to sync main generating thread. 
		*Found frame flicking during high-speed update.

	2005-07-05:
		*Due to lack of 'streamlabs' SDK info about their product, primary
		source was found at: 
			http://www.semiconductors.philips.com/acrobat/other/tm1000.pdf
		that describe all operation and DMA block transfer. 
		Was developed 'VirtualFrameBuffer' 4Mb device driver that allocate
		contigous physical memory and provide mapping to userspace and return
		physical address that's is able to submit into DMA_CTL registers of
		video adapter. Processor load descreased drammaticaly and almost 
		realtime on 1GHz Celeron(tm). 
		*New function for converting ARGB exactly into Trimedia TM-1000 
		framebuffer's planar format. FrameBuffer is mapped with offset 
		YOBUF3 but could be another. Offset is stored in register.... 
		(RTFM for more details). Not sure that new copier will function
		with non-Trimedia based adapters like ... Plus or ...Component.

	2005-06-25:
		*Modified framebuffer requests operations due to TBC scheme
		modification.

    2005-06-08:
		*Code cleanup for GPL licence compatibitlity.
		*Configuring board against config submitted
		*TV System independent? All params configured agains
		vzTVSpec struct submitted.

*/

#include <windows.h>

#include "../vz/vzOutput.h"
#include "../vz/vzOutput-devel.h"
#include "../vz/vzImage.h"
#include "../vz/vzOutputInternal.h"
#include "../vz/vzMain.h"
#include "../vz/vzTVSpec.h"
#include "../vz/vzLogger.h"

#include <stdio.h>
#include <SlBoardIoctl.h>
#include <SharedDef.h>
#include <SlAlpha.h>
#pragma comment(lib, "SlAlpha.lib") 

// VirtualFrameBuffer4
#include "../../../vfb4/include/vfb4-data.h"
static vfb4_data vfb4_info;


/*
	Acceleration features functions
*/

// load and transcode n-th field into framebuffer
void load_n_transcode_field(int field, void* dst, void* src, int flip = 0);
void load_n_transcode_field_planar(int field, void* dst, void* src,int flip=0);


// MMX optimized BGRA to UYVA transcoder
void BGRA2UYVA (void* dst, void* src, int count);
void BGRA2PLANAR4444 (void* dst_yaya, void* dst_u, void* dst_v, void* src, int count);

// is is fast? NO!!! the same as standart lib function....
void inline memcpy2(void* dst,void *src, int count);

// Init VirtualFrameBuffer 4 device
int init_vfb4_info(vfb4_data*);

// own DMA acclerated function for transfering transcoded data into FrameBuffer
void transfer_buffer(unsigned long DST_PHYSICAL_ADDR, unsigned long SRC_PHYSICAL_ADDR, unsigned long SIZE , unsigned long user_reg);



// Dll Main function for initialization of virtual framebuffer
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	int i;
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			if(!(init_vfb4_info(&vfb4_info)))
				exit(0);
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


static const char* vzAlphaStreamOptionsNames[] = 
{
// only for AlphaPlus&AlphaComponent
	"MODE_BYPASS_ON",
	"MODE_BYPASS_OFF",

	"PROGRAM_VIDEOONLY",
	"PROGRAM_OVERLAY",
	"PROGRAM_OVERLAYREPLACE",
	"PROGRAM_GRAPHICSONLY",

	"PREVIEW_VIDEOONLY",
	"PREVIEW_OVERLAY",
	"PREVIEW_OVERLAYREPLACE",
	"PREVIEW_GRAPHICSONLY",

	"MEM_422",
	"MEM_4444",

	"DMUX_ON",
	"DMUX_OFF",

	"CHROMAKEY_ON",
	"CHROMAKEY_OFF",

	"MODE_PAL",
	"MODE_NTSC",

	"MULTIPLY_ALPHA_ON",
	"MULTIPLY_ALPHA_OFF",

	"SWITCH_OUPUT_ON_PREVIEW_ON",
	"SWITCH_OUPUT_ON_PREVIEW_OFF",

	"PLL_ON",
	"PLL_OFF",

	"FREERUN_PLL_ON",
	"FREERUN_PLL_OFF",

	"INPUT_ANALOG",
	"INPUT_SDI",
	"INPUT_ANALOG_SYNC",
	"INPUT_SDI_SYNC",


	NULL
};


enum vzAlphaStreamOptions
{
// only for AlphaPlus&AlphaComponent
	MODE_BYPASS_ON = 0,
	MODE_BYPASS_OFF,

	PROGRAM_VIDEOONLY,
	PROGRAM_OVERLAY,
	PROGRAM_OVERLAYREPLACE,
	PROGRAM_GRAPHICSONLY,

	PREVIEW_VIDEOONLY,
	PREVIEW_OVERLAY,
	PREVIEW_OVERLAYREPLACE,
	PREVIEW_GRAPHICSONLY,

	MEM_422,
	MEM_4444,

	DMUX_ON,
	DMUX_OFF,

	CHROMAKEY_ON,
	CHROMAKEY_OFF,

	MODE_PAL,
	MODE_NTSC,

	MULTIPLY_ALPHA_ON,
	MULTIPLY_ALPHA_OFF,

	SWITCH_OUPUT_ON_PREVIEW_ON,
	SWITCH_OUPUT_ON_PREVIEW_OFF,

	PLL_ON,
	PLL_OFF,

	FREERUN_PLL_ON,
	FREERUN_PLL_OFF,

	INPUT_ANALOG,
	INPUT_SDI,
	INPUT_ANALOG_SYNC,
	INPUT_SDI_SYNC


};

static unsigned int vzAlphaStreamOptions_[][2][2] = 
{
	// non SDI						SDI
	{ {1<<0,0xFFFFFFFF ^ (1<<0) },	{1<<7,0xFFFFFFFF ^ (1<<7) } },	//	MODE_BYPASS_ON,
	{ {0<<0,0xFFFFFFFF ^ (1<<0) },	{0<<7,0xFFFFFFFF ^ (1<<7) } },	//	MODE_BYPASS_OFF,

	{ {0<<1,0xFFFFFFFF ^ (3<<1) },	{0,0xFFFFFFFF}				},	//	PROGRAM_VIDEOONLY,
	{ {1<<1,0xFFFFFFFF ^ (3<<1) },	{0,0xFFFFFFFF}				},	//	PROGRAM_OVERLAY,
	{ {2<<1,0xFFFFFFFF ^ (3<<1) },	{0,0xFFFFFFFF}				},	//	PROGRAM_OVERLAYREPLACE,
	{ {3<<1,0xFFFFFFFF ^ (3<<1) },	{0,0xFFFFFFFF}				},	//	PROGRAM_GRAPHICSONLY,

	{ {0<<3,0xFFFFFFFF ^ (3<<3) },	{0,0xFFFFFFFF}				},	//	PREVIEW_VIDEOONLY,
	{ {1<<3,0xFFFFFFFF ^ (3<<3) },	{0,0xFFFFFFFF}				},	//	PREVIEW_OVERLAY,
	{ {2<<3,0xFFFFFFFF ^ (3<<3) },	{0,0xFFFFFFFF}				},	//	PREVIEW_OVERLAYREPLACE,
	{ {3<<3,0xFFFFFFFF ^ (3<<3) },	{0,0xFFFFFFFF}				},	//	PREVIEW_GRAPHICSONLY,

	{ {1<<6,0xFFFFFFFF ^ (1<<6) },	{1<<6,0xFFFFFFFF ^ (1<<6) } },	//	MEM_422,
	{ {0<<6,0xFFFFFFFF ^ (1<<6) },	{0<<6,0xFFFFFFFF ^ (1<<6) }	},	//	MEM_4444,

	{ {1<<8,0xFFFFFFFF ^ (1<<8) },	{0,0xFFFFFFFF}				},	//	DMUX_ON,
	{ {0<<8,0xFFFFFFFF ^ (1<<8) },	{0,0xFFFFFFFF}				},	//	DMUX_OFF,

	{ {1<<9,0xFFFFFFFF ^ (1<<9) },	{1<<15,0xFFFFFFFF ^(1<<15)}	},	//	CHROMAKEY_ON,
	{ {0<<9,0xFFFFFFFF ^ (1<<9) },	{0<<15,0xFFFFFFFF ^(1<<15)}	},	//	CHROMAKEY_OFF

	{ {0<<12,0xFFFFFFFF ^(1<<12)},	{0<<12,0xFFFFFFFF ^(1<<12)}	},	//	MODE_PAL,
	{ {1<<12,0xFFFFFFFF ^(1<<12)},	{1<<12,0xFFFFFFFF ^(1<<12)}	},	//	MODE_NTSC

	{ {0,0xFFFFFFFF},				{0<<0,0xFFFFFFFF ^ (1<<0) }	},	//	MULTIPLY_ALPHA_ON,
	{ {0,0xFFFFFFFF},				{1<<0,0xFFFFFFFF ^ (1<<0) }	},	//	MULTIPLY_ALPHA_OFF,

	{ {0,0xFFFFFFFF},				{1<<3,0xFFFFFFFF ^ (1<<3) } },	//	SWITCH_OUPUT_ON_PREVIEW_ON,
	{ {0,0xFFFFFFFF},				{0<<3,0xFFFFFFFF ^ (1<<3) } },	//	SWITCH_OUPUT_ON_PREVIEW_OFF,

	{ {0,0xFFFFFFFF},				{1<<9,0xFFFFFFFF ^ (1<<8) }	},	//	PLL_ON,
	{ {0,0xFFFFFFFF},				{0<<9,0xFFFFFFFF ^ (1<<8) }	},	//	PLL_OFF

	{ {0,0xFFFFFFFF},				{1<<9,0xFFFFFFFF ^ (1<<9) }	},	//	FREERUN_PLL_ON,
	{ {0,0xFFFFFFFF},				{0<<9,0xFFFFFFFF ^ (1<<9) }	},	//	FREERUN_PLL_OFF


	{ {0,0xFFFFFFFF},				{0<<1,0xFFFFFFFF ^ (3<<1) }	},	//	INPUT_ANALOG,
	{ {0,0xFFFFFFFF},				{1<<1,0xFFFFFFFF ^ (3<<1) }	},	//	INPUT_SDI,
	{ {0,0xFFFFFFFF},				{2<<1,0xFFFFFFFF ^ (3<<1) }	},	//	INPUT_ANALOG_SYNC,
	{ {0,0xFFFFFFFF},				{3<<1,0xFFFFFFFF ^ (3<<1) }	},	//	INPUT_SDI_SYNC,

};

#define ALPHA_SET_CONTROL(CONTROL,PARAM,SDI) CONTROL&=vzAlphaStreamOptions_[PARAM][SDI][1];CONTROL|=vzAlphaStreamOptions_[PARAM][SDI][0];


// titles
static char *_titles[4]=
{
	"AlphaTM",
	"AlphaPlus v1",
	"AlphaPlus v2",
	"AlphaComponent"
};

static void* _config = NULL;
static vzTVSpec* _tv = NULL;

#define ALPHA_BOARD _handle,_id

static unsigned char _boards[4];		// array of found boards
static int _handle;					// used id - handle
static unsigned long _id;				// id
static unsigned long _count;			// count of boards
static REGISTRY_INFO _registry[4];
static HANDLE _fields_event[3];

// smart operations....
static MEMORY_ADDRESS _addr_reg;
static MEMORY_ADDRESS _addr_mem;
static unsigned long* _reg;


// init
VZOUTPUTS_EXPORT long vzOutput_FindBoard(char** error_log = NULL)
{
	// 1 find boards
	_count = Alpha_GetBoardCount(_boards,_handle);
	if (_count == 0)
	{
		if (error_log) *error_log = "No boards found";
		return 0;
	};
	return _count;
};

// init
VZOUTPUTS_EXPORT void vzOutput_SetConfig(void* config)
{
	_config = config;
};


VZOUTPUTS_EXPORT long vzOutput_SelectBoard(unsigned long id,char** error_log = NULL)
{
	if (id>=_count)
	{
		if (error_log) *error_log = "Board number is over";
		return 0;
	}

	// set id
	_id = id;

	logger_printf(0, "streamalpha[%s]", _titles[_boards[_id]]);

	if (error_log) *error_log = _titles[_boards[_id]];

	return _id;
};

static frames_counter_proc _fc = NULL;

VZOUTPUTS_EXPORT long vzOutput_SetSync(frames_counter_proc fc)
{
	return (long)(_fc =  fc);
};

VZOUTPUTS_EXPORT long vzOutput_InitBoard(void* tv)
{
	// assign tv value 
	_tv = (vzTVSpec*)tv;

	// map registers (???)
	Alpha_GetMapRegs(ALPHA_BOARD,&_addr_reg);
	_reg = (unsigned long*)_addr_reg.m_UserAddress;
	Alpha_GetMapMemory(ALPHA_BOARD,&_addr_mem);

	// configure board!
	if(_boards[_id] != ALPHA_TM)
	{
		// Component Board!
		char* v;

		// interrupts enable
		// seem to enable bits for all interrupts (field#0 and field#1)
		unsigned long interrupts_enable = (1+2)<<16;
		// m_TMReg[m_ActiveBoard][PINT_ST] = int_en;
		_reg[PINT_ST] = interrupts_enable;
	}
	else
	{
		// SDI Board!
		// ???? 0x40 - Enable PCI Interrupts
		// ???? 0x80 - Enable Transform
		// ???? 0x100 - Enable Grabber
		_reg[SPDO_BASE1>>2] |= 0x40; 

		// disable tranform - use direct planar framebuffer
		_reg[SPDO_BASE1>>2] &= ~0x80;
	};

	// set parameters
	char* config_option;
	unsigned int settings = 0;

	logger_printf(0, "streamalpha: general settings");
	for(int i=0;vzAlphaStreamOptionsNames[i];i++)
		if(vzConfigParam(_config,"streamalpha",(char*)vzAlphaStreamOptionsNames[i]))
		{
			logger_printf(0, "streamalpha: |- '%s'",vzAlphaStreamOptionsNames[i]);
			ALPHA_SET_CONTROL(settings,i,(_boards[_id] == ALPHA_TM)?1:0);
		};

	Alpha_SetVCONTROL(ALPHA_BOARD,settings);

	// set Phase
	if(config_option = vzConfigParam(_config,"streamalpha","ColorSubcarrierPhase"))
	{
		unsigned char phase = (unsigned char)atol(config_option);
		Alpha_SetColorSubcarrierPhase(ALPHA_BOARD,phase);
		logger_printf(0, "streamalpha: |- 'ColorSubcarrierPhase'='%d",(unsigned int)phase);
	};

	// set "the offset of source video signal according to sync. signal"
	if(config_option = vzConfigParam(_config,"streamalpha","SDEL"))
	{
		Alpha_SetSDEL(ALPHA_BOARD,atol(config_option));
		logger_printf(0, "streamalpha: |- 'SDEL'='%ld",atol(config_option));
	};

	// set "(the offset of the alpha-channel (Key) according to the source signal (Fill))"
	if(config_option = vzConfigParam(_config,"streamalpha","ADEL"))
	{
		Alpha_SetADEL(ALPHA_BOARD,atol(config_option));
		logger_printf(0, "streamalpha: |- 'ADEL'='%ld",atol(config_option));
	};

	// clear picture
	TMMEMORY_STRUCT tm_memory;
	memset(&tm_memory,0,sizeof(TMMEMORY_STRUCT));
	tm_memory.dwHeight=_tv->TV_FRAME_HEIGHT;
	tm_memory.dwWidth=_tv->TV_FRAME_WIDTH;
	tm_memory.Flags=FIELD_01;
	Alpha_ClearMemory(ALPHA_BOARD,&tm_memory);

	return _count;
};

static HANDLE loop_thread;
static int notify_to_stop_loop = 0;


static unsigned long WINAPI output_loop(void* obj)
{
	// framebuffer pointer
	void **input_buffers, *image_buffer = NULL;
	void *output_a_buffer, **input_a_buffers;

	vzOutput* tbc = (vzOutput*)obj;

	logger_printf(0, "streamalpha: Output loop started!");

	// define framebuffers queue
	unsigned long vfb_in = vfb4_info.user;
	unsigned long vfb_out = vfb4_info.physical_low;

	unsigned long alpha_in[2];
		alpha_in[0] = (_addr_mem.PhysAddr + YOBUF4 );
		alpha_in[1] = (_addr_mem.PhysAddr + YOBUF4 + 887040);


	// endless loop
	while(0 == notify_to_stop_loop)
	{
// -----------------------------------------------------------------------------------------

		// first, request new framebuffer
		tbc->lock_io_bufs(&image_buffer, &input_buffers, &output_a_buffer, &input_a_buffers);

// -----------------------------------------------------------------------------------------

		// transcode field #0
		if (image_buffer)
			// transcode field!
			load_n_transcode_field_planar
			(
				0,
				(void*)vfb_in,
				(void*)image_buffer,
				1
			);

		// wait for field #0
		WaitForSingleObject(_fields_event[0], INFINITE);	ResetEvent(_fields_event[0]);

		// sync signal !
		_fc();

		// DMA transfer field #0
		transfer_buffer(alpha_in[0], vfb_out, 872640, (unsigned long)_addr_reg.m_UserAddress);
		

// -----------------------------------------------------------------------------------------

		// transcode field #0
		if (image_buffer)
			// transcode field!
			load_n_transcode_field_planar
			(
				1,
				(void*)vfb_in,
				(void*)image_buffer,
				1
			);

		// unlock used buffer
		tbc->unlock_io_bufs(&image_buffer, &input_buffers, &output_a_buffer, &input_a_buffers);

		// wait for field #1
		WaitForSingleObject(_fields_event[1], INFINITE);ResetEvent(_fields_event[1]);

		// DMA transfer field #0
		transfer_buffer(alpha_in[1], vfb_out, 872640, (unsigned long)_addr_reg.m_UserAddress);

// -----------------------------------------------------------------------------------------

	};

	// event to stop reached !!!
	ExitThread(0);

	return 0;
};

VZOUTPUTS_EXPORT void vzOutput_StartOutputLoop(void* obj)
{
	notify_to_stop_loop = 0;

	// events
	if(!(_fields_event[0] = CreateEvent(NULL, TRUE, FALSE, NULL)))//field0 VSync
		logger_printf(1, "streamalpha: Unable to Create Event #0");
	if(!(_fields_event[1] = CreateEvent(NULL, TRUE, FALSE, NULL)))//field1 VSync
		logger_printf(1, "streamalpha: Unable to Create Event #1");
	if(!(_fields_event[2] = CreateEvent(NULL, TRUE, FALSE, NULL)))
		logger_printf(1, "streamalpha: Unable to Create Event #2");

	Alpha_SetNotificationEvent(ALPHA_BOARD,_fields_event);

	loop_thread = CreateThread
	(
		NULL,
		1024,
		output_loop,
		obj, //&_thread_data, // params
		0,
		NULL
	);

	SetThreadPriority(loop_thread , THREAD_PRIORITY_HIGHEST);
};


VZOUTPUTS_EXPORT void vzOutput_StopOutputLoop()
{
	// notify to stop thread
	notify_to_stop_loop = 1;

	// wait thread for stopped state
	WaitForSingleObject(loop_thread, INFINITE);

	// close everithing
	CloseHandle(loop_thread);
};


VZOUTPUTS_EXPORT void vzOutput_GetBuffersInfo(struct vzOutputBuffers* b)
{
	b->output.offset = 0;
	b->output.size = 4*_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT;
	b->output.gold = ((b->output.size + DMA_PAGE_SIZE)/DMA_PAGE_SIZE)*DMA_PAGE_SIZE;
};

VZOUTPUTS_EXPORT void vzOutput_AssignBuffers(struct vzOutputBuffers* b)
{

};

/* ------------------------------------------------------------------------ */

static void BGRA2UYVA (void* dst, void* src, int count)
{
	if ( (!(dst)) || (!(src)) || (!(count)) )
		return;
/*

Note: Alpha value is scaled up to TV value [0,255] -> [16,235]

We use equantion (output should be devided on 256 ):

	Y =		0		+76R	+150G	+29B	+0A
	U =		32768	-43R	-85G	+127B	+0A
	V =		32768	+127R	-106G	-21B	+0A
	a =		4096	+0R		+0G		+0G		+229A

Resort in DST mem organization,
in memory view is	[Ui,Yi,Vi,Ai]
than loaded in register: (AiViYiUi) (each value is 8-bits)

	a =		4096	+0R		+0G		+0G		+229A
	V =		32768	+127R	-106G	-21B	+0A
	Y =		0		+76R	+150G	+29B	+0A
	U =		32768	-43R	-85G	+127B	+0A

Resort operands agains:

	a =		4096	+229A	+0B		+0G		+0R		
	V =		32768	+127R	+0A		-21B	-106G	
	Y =		0		+150G	+76R	+0A		+29B	
	U =		32768	+127B	-85G	-43R	+0A

As you see you can use iteration with shifting source 
register and multiplying on contant vectors

constants
	l1 = (4096,32768,0,32768)
	l2 = (229,127,150,127)
	l3 = (0,0,76,-85)
	l4 = (0,-21,0,-43)
	l5 = (0,-106,29,0)

for loading into 64 bit register data should reversed

*/

/* 
	Constants
	static short _l1[] = {	32768,	0,		32768,	4096	};
	static short _l2[] = {	127,	150,	127,	229		};
	static short _l3[] = {	-85,	76,		0,		0		};
	static short _l4[] = {	-43,	0,		-21,	0		};
	static short _l5[] = {	0,		29,		-106,	0		};
*/

	__asm
	{
		jmp		begin
ALIGN 16
__l1:
	__asm _emit 0x00
	__asm _emit 0x80
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0x80
	__asm _emit 0x00
	__asm _emit 0x10
__l2:
	__asm _emit 0x7F
	__asm _emit 0x00
	__asm _emit 0x96
	__asm _emit 0x00
	__asm _emit 0x7F
	__asm _emit 0x00
	__asm _emit 0xE5
	__asm _emit 0x00
__l3:
	__asm _emit 0xAB
	__asm _emit 0xFF
	__asm _emit 0x4C
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0x00
__l4:
	__asm _emit 0xD5
	__asm _emit 0xFF
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0xEB
	__asm _emit 0xFF
	__asm _emit 0x00
	__asm _emit 0x00
__l5:
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0x1D
	__asm _emit 0x00
	__asm _emit 0x96
	__asm _emit 0xFF
	__asm _emit 0x00
	__asm _emit 0x00

begin:
		// loading source surface pointer
		mov		esi, dword ptr[src];

		// loading dst surface pointer
		mov		edi, dword ptr[dst];

		// counter
		mov		ecx, dword ptr[count];

		// clear registers and set constants
		movq		MM2, qword ptr[__l1];

		movq		MM3, qword ptr[__l2];
		movq		MM4, qword ptr[__l3];
		movq		MM5, qword ptr[__l4];
		movq		MM6, qword ptr[__l5];

pixel:
		// prefetch to cache
////		PREFETCHNTA [esi];
////		PREFETCHNTA [edi];

		pxor		MM0,MM0		//	clear register

		// STEP #0: load 1 pixels
		movd		MM1, qword ptr[esi]

		// STEP #1: unpack pixel
		punpcklbw	MM1,MM0		//  unpacking

		// STEP #2:	load summ into accum vec #1
		movq		MM0, MM2

		// STEP #3: calc and add vec #2
		movq		MM7, MM1	// load pixel into addr
		pshufw		MM1, MM1,	0x39
		pmullw		MM7, MM3	// multipy
		paddw		MM0, MM7	// add to accum
	
		// STEP #4: calc and add vec #3
		movq		MM7, MM1	// load pixel into addr
		pshufw		MM1, MM1,	0x39
		pmullw		MM7, MM4	// multipy
		paddw		MM0, MM7	// add to accum

		// STEP #5: calc and add vec #4
		movq		MM7, MM1	// load pixel into addr
		pshufw		MM1, MM1,	0x39
		pmullw		MM7, MM5	// multipy
		paddw		MM0, MM7	// add to accum

		// STEP #6: calc and add vec #5
		movq		MM7, MM1	// load pixel into addr
		pshufw		MM1, MM1,	0x39
		pmullw		MM7, MM6	// multipy
		paddw		MM0, MM7	// add to accum

		// STEP #7: pack pixel
		pxor		MM1,MM1		// clear MM1
		psrlw		MM0,8		// divide into 256 :-))
		PACKUSWB	MM0,MM1


		// STEP #8 store pixel
		movd		qword ptr[edi], MM0

		add			edi , 4
		add			esi , 4

		loop pixel

		emms
	}
};

// trascode and update expected fields
static void load_n_transcode_field(int field, void* dst, void* src, int flip)
{
//	return;
#define PIXEL_SIZE 4

	if ( (!(dst)) || (!(src)) )
		return;


	long step;

	unsigned char* _dst = (unsigned char*)dst;
	unsigned char* _src = (unsigned char*)src;

	// reposition source (flip and field order) and set step for source
	if(flip)
	{
		_src += _tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT*PIXEL_SIZE - (1+field)*_tv->TV_FRAME_WIDTH*PIXEL_SIZE;
		step = -1;
	}
	else
	{
		step = 1;
		_src += field*PIXEL_SIZE*_tv->TV_FRAME_WIDTH;
	};

	for(int row=0;row<(_tv->TV_FRAME_HEIGHT/2);row++,_src+= step*2*_tv->TV_FRAME_WIDTH*PIXEL_SIZE , _dst+=_tv->TV_FRAME_WIDTH*PIXEL_SIZE)
		BGRA2UYVA(_dst, _src, _tv->TV_FRAME_WIDTH);
};


void inline memcpy2(void* dst,void *src, int count)
{
	__asm
	{
		// loading source surface pointer
		mov		esi, dword ptr[src];

		// loading dst surface pointer
		mov		edi, dword ptr[dst];

		// counter
		mov		ecx, count;

		// divide ecx
		sar		ecx,6	// devide on 2


		// loop data copy
cp:
		movq		MM0, qword ptr[esi]
		movq		MM1, qword ptr[esi+8]
		movq		MM2, qword ptr[esi+16]
		movq		MM3, qword ptr[esi+24]
		movq		MM4, qword ptr[esi+32]
		movq		MM5, qword ptr[esi+40]
		movq		MM6, qword ptr[esi+48]
		movq		MM7, qword ptr[esi+56]
		add			esi,64
		movq		qword ptr[edi],MM0
		movq		qword ptr[edi+8],MM1
		movq		qword ptr[edi+16],MM2
		movq		qword ptr[edi+24],MM3
		movq		qword ptr[edi+32],MM4
		movq		qword ptr[edi+40],MM5
		movq		qword ptr[edi+48],MM6
		movq		qword ptr[edi+56],MM7
		add			edi,64
		loop		cp;
		emms
	}

	return;

};

static void transfer_buffer(unsigned long DST_PHYSICAL_ADDR, unsigned long SRC_PHYSICAL_ADDR, unsigned long SIZE , unsigned long user_reg)
{
#define __MMIO(offset)  ( ((unsigned long*)user_reg)[(offset) >> 2])
#define _DMA_READ 1

	// see pages 10-13 (p.138) of "TM-1000 Preliminary Data Book"

	// The steps involved in a DMA transfer are:

	// 1. Wait until BIU_STATUS dma_cycle.Busy is de-asserted 
	while ( __MMIO(BIU_STATUS)& (1<<4) ) Sleep(0);

	// 1a. disable interrupt generation after DMA finished
	__MMIO(BIU_CTL) &= (0xFFFFFFFF ^ (1<<5) ^ (1<<4) ); // ^ (1<<5));

	// 2. Write to SRC_ADR and DEST_ADR as described above.
	__MMIO(SRC_ADR) = SRC_PHYSICAL_ADDR;
	__MMIO(DEST_ADR) = DST_PHYSICAL_ADDR;

	// 3. Write to DMA_CTL to start the dma transaction.This action sets dma_cycle.Busy.
	__MMIO(DMA_CTL) = SIZE | (_DMA_READ << 26) | (1 << 27);

	// 4. Wait (polling or interrupt based) until dma_cycle.Done is asserted by the hardware.
//	Sleep(10);
	while (!( __MMIO(BIU_STATUS) & (1<<5) ))  Sleep(1);

	// 5. Clear dma_cycle.Done by writing a ‘1’ to it.
	__MMIO(BIU_STATUS) |= (1<<5);
};

static int init_vfb4_info(vfb4_data *vfb4_info)
{
	unsigned long ret;
	HANDLE vdf_dev;
	
	// clear values in struct
	memset(vfb4_info,0,sizeof(vfb4_data));

	// open device
	if (!(vdf_dev = CreateFile(VFB4_API_DEV, GENERIC_READ | GENERIC_WRITE | FILE_FLAG_NO_BUFFERING, 0, NULL, OPEN_EXISTING, 0, NULL)))
	{
		logger_printf(1, "streamalpha: Error opening '%s'. Please see vfb4.txt", VFB4_API_DEV);
		return 0;
	};

	// read data
	if(!ReadFile(vdf_dev, vfb4_info, sizeof(vfb4_data), &ret, NULL))
	{
		logger_printf(1, "streamalpha: Error reading '%s'. Please see vfb4.txt", VFB4_API_DEV, GetLastError());
		return 0;
	};

	return 1;
};

static void BGRA2PLANAR4444 (void* dst_yaya, void* dst_u, void* dst_v, void* src, int count)
{
/*

Note: Alpha value is scaled up to TV value [0,255] -> [16,235]

We use equantion (output should be devided on 256 ):

	Y =		0		+76R	+150G	+29B	+0A
	U =		32768	-43R	-85G	+127B	+0A
	V =		32768	+127R	-106G	-21B	+0A
	a =		4096	+0R		+0G		+0G		+229A

Resort in DST mem organization,
in memory view is	[Ui,Yi,Vi,Ai]
than loaded in register: (AiViYiUi) (each value is 8-bits)

	a =		4096	+0R		+0G		+0B		+229A
	Y =		0		+76R	+150G	+29B	+0A
	V =		32768	+127R	-106G	-21B	+0A
	U =		32768	-43R	-85G	+127B	+0A

Resort operands agains:

	a =		4096	+229A	+0B		+0G		+0R
	Y =		0		+76R	+0A		+29B	+150G			
	V =		32768	-106G	+127R	+0A		-21B	
	U =		32768	+127B	-85G	-43R	+0A


As you see you can use iteration with shifting source 
register and multiplying on contant vectors

constants

	l1 = 4096,0,32768,32768)
	l2 = 229,76,-106,127)
	l3 = 0,0,127,-85)
	l4 = 0,29,0,-43)
	l5 = 0,150,-21,0)
	

for loading into 64 bit register data should reversed

*/

/* 
	Constants

	static short __l1[] = {	32768,	32768,	0,		4096	};
	static short __l2[] = {	127,	-106,	76,		229		};
	static short __l3[] = {	-85,	127,	0,		0		};
	static short __l4[] = {	-43,	0,		29,		0		};
	static short __l5[] = {	0,		-21,	150,	0		};
*/

	__asm
	{
		jmp		begin
ALIGN 16

__l1: 
	__asm _emit 0x00 
	__asm _emit 0x80 
	__asm _emit 0x00 
	__asm _emit 0x80 
	__asm _emit 0x00 
	__asm _emit 0x00 
	__asm _emit 0x00 
	__asm _emit 0x10
__l2:
	__asm _emit 0x7F 
	__asm _emit 0x00 
	__asm _emit 0x96 
	__asm _emit 0xFF 
	__asm _emit 0x4C 
	__asm _emit 0x00 
	__asm _emit 0xE5 
	__asm _emit 0x00
__l3:
	__asm _emit 0xAB 
	__asm _emit 0xFF 
	__asm _emit 0x7F 
	__asm _emit 0x00 
	__asm _emit 0x00 
	__asm _emit 0x00 
	__asm _emit 0x00 
	__asm _emit 0x00
__l4:
	__asm _emit 0xD5 
	__asm _emit 0xFF 
	__asm _emit 0x00 
	__asm _emit 0x00 
	__asm _emit 0x1D 
	__asm _emit 0x00 
	__asm _emit 0x00 
	__asm _emit 0x00
__l5:
	__asm _emit 0x00
	__asm _emit 0x00 
	__asm _emit 0xEB
	__asm _emit 0xFF 
	__asm _emit 0x96 
	__asm _emit 0x00 
	__asm _emit 0x00 
	__asm _emit 0x00

__mask1:
	__asm _emit 0xFF
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0x00

begin:
		// loading source surface pointer
		mov		esi, dword ptr[src];
		add		esi, 16

		// loading dst surface pointer
		mov		edi, dword ptr[dst_yaya];
		mov		ebx, dword ptr[dst_u];
		mov		eax, dword ptr[dst_v];

		// counter
		mov		ecx, dword ptr[count];

pixel:
		// save pixel counter
		push	ecx

		// processing is 4 pixel range
		mov		ecx, 4

		// clear 
		pxor	MM2, MM2	// yaya reg
		pxor	MM3, MM3	// u
		pxor	MM4, MM4	// v

		// MM5 used as accum2


		// prefetch to cache
		PREFETCHNTA [esi];
		PREFETCHNTA [edi];

subpixel:


		pxor		MM0,MM0		//	clear register

		// STEP #0: load 1 pixels
		sub			esi, 4
		mov			edx, dword ptr[esi]
		movd		MM1, edx

		// STEP #1: unpack pixel
		punpcklbw	MM1,MM0		//  unpacking

		// STEP #2:	load summ into accum vec #1
		movq		MM0, qword ptr[__l1]

		// STEP #3: calc and add vec #2
		movq		MM7, MM1	// load pixel into addr
		pshufw		MM1, MM1,	0x39
		pmullw		MM7, qword ptr[__l2]	// multipy
		paddw		MM0, MM7	// add to accum
	
		// STEP #4: calc and add vec #3
		movq		MM7, MM1	// load pixel into addr
		pshufw		MM1, MM1,	0x39
		pmullw		MM7, qword ptr[__l3]	// multipy
		paddw		MM0, MM7	// add to accum

		// STEP #5: calc and add vec #4
		movq		MM7, MM1	// load pixel into addr
		pshufw		MM1, MM1,	0x39
		pmullw		MM7, qword ptr[__l4]	// multipy
		paddw		MM0, MM7	// add to accum

		// STEP #6: calc and add vec #5
		movq		MM7, MM1	// load pixel into addr
		pshufw		MM1, MM1,	0x39
		pmullw		MM7, qword ptr[__l5]	// multipy
		paddw		MM0, MM7	// add to accum

		// STEP #7: pack pixel
		pxor		MM1,MM1		// clear MM1
		psrlw		MM0,8		// divide into 256 :-))
		PACKUSWB	MM0,MM1

// transoding into 3 registers

		// step #8a (U)
		movq		MM5,MM0		// load pixel into MM5
		psrlq		MM0,8		// shift MM0
		pand		MM5,qword ptr[__mask1]
		psllq		MM3,8
		por			MM3,MM5

//skipU:

		// step #8b (V)
		movq		MM5,MM0		// load pixel into MM5
		psrlq		MM0,8		// shift MM0
		pand		MM5,qword ptr[__mask1]
		psllq		MM4,8
		por			MM4,MM5
//skipV:

		// step #8c (Y)
		movq		MM5,MM0		// load pixel into MM5
		psllq		MM2,16
		por			MM2,MM5

		// loop to subpixels
		dec			ecx
		test		ecx,ecx
		jnz			subpixel

		// STEP #9 store pixel
		movq		qword ptr[edi], MM2		// store AYAYAYAY

		movd		edx, MM3				// store UU
		mov			dword ptr[ebx], edx

		movd		edx, MM4				// store VV
		mov			dword ptr[eax], edx

		add			edi, 8
		add			ebx, 4
		add			eax, 4

		// restore pixel counter
		pop			ecx

		add			esi, 32
		// check if all pixel proceeded
		sub			ecx,4
		test		ecx,ecx
		jnz			pixel

		emms
	}
};

// trascode and update expected fields
static void load_n_transcode_field_planar(int field, void* dst, void* src,int flip)
{
#define SRC_PIXEL_SIZE 4
#define Y_PIXEL_SIZE 2
#define U_PIXEL_SIZE 1
#define V_PIXEL_SIZE 1


	if ( (!(dst)) || (!(src)) )
		return;


	long step;

	unsigned char* _dst_Y = (unsigned char*)dst;
	unsigned char* _dst_U = (unsigned char*)dst + 443520;
	unsigned char* _dst_V = (unsigned char*)dst + 665280;

	unsigned char* _src = (unsigned char*)src;

	// reposition source (flip and field order) and set step for source
	if(flip)
	{
		_src += (_tv->TV_FRAME_HEIGHT -  1 - field ) * SRC_PIXEL_SIZE * _tv->TV_FRAME_WIDTH;
		step = -2;
	}
	else
	{
		step = 2;
		_src += field*SRC_PIXEL_SIZE*_tv->TV_FRAME_WIDTH;
	};

	for
	(
		int row=0;
		row<(_tv->TV_FRAME_HEIGHT/2);
			row++,
			_src += step*_tv->TV_FRAME_WIDTH*SRC_PIXEL_SIZE ,
			_dst_Y += _tv->TV_FRAME_WIDTH*Y_PIXEL_SIZE,
			_dst_U += _tv->TV_FRAME_WIDTH*U_PIXEL_SIZE,
			_dst_V += _tv->TV_FRAME_WIDTH*V_PIXEL_SIZE
	)
		BGRA2PLANAR4444 (_dst_Y , _dst_U, _dst_V, _src, _tv->TV_FRAME_WIDTH);

};

/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2007 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1.tv, 2007.

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
    2007-02-19:
		* new version start

*/

#include <stdio.h>
#include <process.h>
#include <windows.h>

#include "vzMain.h"
#include "vzImage.h"
#include "vzTVSpec.h"
#include "vz_cmd/vz_cmd.h"

extern void* scene;	// scene loaded
extern HANDLE scene_lock;
extern void* functions;
extern vzTVSpec tv;

extern int main_stage;
extern char screenshot_file[1024];
extern void* config;

#define SERIAL_BUF_SIZE 65536

unsigned long WINAPI serserver(void* _config)
{
	char* temp, *serial_port_name;
	long buf_size_max = SERIAL_BUF_SIZE;
	HANDLE serial_port_handle;

	// check if server is enabled
	if(!vzConfigParam(_config,"serserver","enable"))
	{
		printf("serserver: disabled\n");
		ExitThread(0);
	};

	// check for params
	serial_port_name = vzConfigParam(_config,"serserver","serial_port_name");
	if(temp = vzConfigParam(_config,"serserver","bufsize"))
		buf_size_max = atol(temp);

    // open port
    serial_port_handle = CreateFile
    (
    	serial_port_name,
    	GENERIC_READ | GENERIC_WRITE,
    	0,
    	NULL,
    	OPEN_EXISTING,
    	FILE_ATTRIBUTE_NORMAL,
		NULL
    );

    // check port opens
    if (INVALID_HANDLE_VALUE == serial_port_handle)
    {
    	printf("serserver: ERROR! Unable to open port '%s' [err: %d]\n",serial_port_name, GetLastError());
    	ExitThread(0);
    };

    // configure
    DCB lpdcb;
    memset(&lpdcb, 0, sizeof(DCB));
    lpdcb.BaudRate = (UINT) CBR_115200;		// 115200 b/s
											// a. 1 start bit ( space )
    lpdcb.ByteSize = (BYTE) 8;				// b. 8 data bits
    lpdcb.Parity = (BYTE) ODDPARITY;		// c. 1 parity bit (odd)
    lpdcb.StopBits = (BYTE) ONESTOPBIT;		// d. 1 stop bit (mark)
    lpdcb.fParity = 1;
    lpdcb.fBinary = 1;
    if (!(SetCommState(serial_port_handle, &lpdcb)))
    {
	  	printf("serserver: ERROR! Unable to configure '%s' [err: %d]\n",serial_port_name, GetLastError());
    	ExitThread(0);
    };

    // instance an object of COMMTIMEOUTS.
    COMMTIMEOUTS comTimeOut; 
    comTimeOut.ReadIntervalTimeout = 8;
    comTimeOut.ReadTotalTimeoutMultiplier = 1;
    comTimeOut.ReadTotalTimeoutConstant = 250;
    comTimeOut.WriteTotalTimeoutMultiplier = 3;
    comTimeOut.WriteTotalTimeoutConstant = 250;
    if (!(SetCommTimeouts(serial_port_handle,&comTimeOut)))
    {
    	printf("serserver: ERROR: Unable to setup timeouts\n");
    	ExitThread(0);
    };

	/* allocate space for buffer */
	void* buf;
	unsigned char* buf_ptr;
	buf = malloc(buf_size_max);
	buf_ptr = (unsigned char*)buf;

	/* "endless" loop */
	int nak = 0;
	for(;;)
	{
		HRESULT h;
		unsigned long r_bytes;

		/* write error/ack status */
		if(nak)
		{
			h = WriteFile
			(
				serial_port_handle, 
				&nak, 
				1, 
				&r_bytes, 
				NULL
			);

			nak = 0;
		};

		/* read block */
		h = ReadFile
		(
			serial_port_handle, 
			buf_ptr, 
			buf_size_max - (buf_ptr - (unsigned char*)buf), 
			&r_bytes, 
			NULL
		);

		/* timeout */
		if(0 == r_bytes) continue;

		/* try to parse buffer */
		int p_bytes = (int)r_bytes + (buf_ptr - (unsigned char*)buf);
		int r = vz_serial_cmd_probe(buf, &p_bytes);

		/* detect incorrect or partial command */
		if
		(
			(-VZ_EINVAL == r)
			||
			(-VZ_EFAIL == r)
		)
		{
			/* invalid buffer - reset */
			buf_ptr = (unsigned char*)buf;
			nak = 0x05;
			continue;
		};

		if(-VZ_EBUSY == r)
		{
			/* partial command */
			buf_ptr += r_bytes;
			continue;
		};

		/* here command is good */
		buf_ptr = (unsigned char*)buf;
		int commands_count = vz_serial_cmd_count(buf);
		for(int i = 0; i<commands_count; i++)
		{
			char* filename;
			int cmd = vz_serial_cmd_id(buf, i);
			char* cmd_name = vz_cmd_get_name(cmd);
			printf("serserver: %s", cmd_name);

			switch(cmd)
			{
				case VZ_CMD_PING:
					printf("\n");
					break;

				case VZ_CMD_LOAD_SCENE:

					/* map variable */
					vz_serial_cmd_parseNmap(buf, i, &filename);

					/* notify */
					printf("('%s')\n", filename);

					// lock scene
					WaitForSingleObject(scene_lock,INFINITE);

					/* delete curent scene */
					if(scene) vzMainSceneFree(scene);

					/* create new scene */
					scene = vzMainSceneNew(functions, config, &tv);

					/* check if it loaded ok */
					if (!vzMainSceneLoad(scene, filename))
					{
						vzMainSceneFree(scene);
						scene = NULL;
					};

					// unlock scene
					ReleaseMutex(scene_lock);
					break;

				case VZ_CMD_SCREENSHOT:

					/* map variable */
					vz_serial_cmd_parseNmap(buf, i, &filename);
					
					/* notify */
					printf("('%s')\n", filename);

					// lock scene
					WaitForSingleObject(scene_lock, INFINITE);

					// copy filename
					strcpy(screenshot_file, filename);

					// unlock scene
					ReleaseMutex(scene_lock);
					break;

				default:
					if(scene)
						vzMainSceneCommand(scene, cmd, i, buf);
					break;

			};
		};
		nak = 0x04;
	};
};

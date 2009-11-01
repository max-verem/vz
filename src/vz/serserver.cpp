/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2009 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1.tv, 2009.

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
	2009-01-24:
		*block command code split: serserver_cmd is now standalone
		*externs removed to main.h, tcpserver common code now provided 
		from main.c

    2008-09-24:
        *logger use for message outputs

	2007-11-18:
		*VS2005 migrations patches

    2007-02-19:
		* new version start

*/
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <process.h>
#include <windows.h>

#include "vzMain.h"
#include "vzImage.h"
#include "vzTVSpec.h"
#include "../vzCmd/vz_cmd.h"
#include "vzLogger.h"

#include "main.h"

#define SERIAL_BUF_SIZE 65536

static HANDLE serial_port_handle;

void serserver_kill(void)
{
	CloseHandle(serial_port_handle);
};

int serserver_cmd(void* buf, int *p_bytes)
{
	int r, i, commands_count, cmd;
	char *filename, *cmd_name;

	/* probe */
	r = vz_serial_cmd_probe(buf, p_bytes);

	/* check if probe success */
	if(r < 0)
		return r;

	/* detect command count in block */
	commands_count = vz_serial_cmd_count(buf);
	for(i = 0; i<commands_count; i++)
	{
		cmd = vz_serial_cmd_id(buf, i);
		cmd_name = vz_cmd_get_name(cmd);
		logger_printf(0, "serserver_cmd: %s (%d)", cmd_name, i);

		switch(cmd)
		{
			case VZ_CMD_PING:
				break;

			case VZ_CMD_LOAD_SCENE:

				/* map variable */
				vz_serial_cmd_parseNmap(buf, i, &filename);

				/* do it */
				CMD_loadscene(filename, NULL);
				break;

			case VZ_CMD_SCREENSHOT:

				/* map variable */
				vz_serial_cmd_parseNmap(buf, i, &filename);

				/* do it */
				CMD_screenshot(filename, NULL);
				break;

			default:
				if(scene)
					vzMainSceneCommand(scene, cmd, i, buf);
				break;

		};
	};

	return commands_count;
};

unsigned long WINAPI serserver(void* _config)
{
	char* temp, *serial_port_name;
	long buf_size_max = SERIAL_BUF_SIZE;
	

	// check if server is enabled
	if(!vzConfigParam(_config,"serserver","enable"))
	{
		logger_printf(0, "serserver: disabled");
		ExitThread(0);
	}

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
    	logger_printf(1, "serserver: Unable to open port '%s' [err: %d]",serial_port_name, GetLastError());
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
	  	logger_printf(1, "serserver: Unable to configure '%s' [err: %d]",serial_port_name, GetLastError());
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
    	logger_printf(1, "serserver: Unable to setup timeouts");
    	ExitThread(0);
    };

	/* allocate space for buffer */
	void* buf;
	unsigned char* buf_ptr;
	buf = malloc(buf_size_max);
	buf_ptr = (unsigned char*)buf;

	/* "endless" loop */
	int nak = 0;
	logger_printf(0, "serserver: started on port %s", serial_port_name);
	for(;0 == f_exit;)
	{
		HRESULT h;
		DWORD r_bytes;

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
			(DWORD)(buf_size_max - (buf_ptr - (unsigned char*)buf)), 
			&r_bytes, 
			NULL
		);

		/* timeout */
		if(0 == r_bytes) continue;

		/* try to parse buffer */
		int p_bytes = (int)r_bytes + (int)(buf_ptr - (unsigned char*)buf);
		int r = serserver_cmd(buf, &p_bytes);

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
		nak = 0x04;
	};

	return 0;
};

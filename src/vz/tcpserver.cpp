/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2009 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2009.

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
		*code reorganization
		*winsock initialization moved to main.c

    2008-09-24:
        *logger use for message outputs

	2007-11-18:
		*VS2005 migrations patches

	2007-03-10:
		*exiting from threads fixed

    2005-06-08:
		* Code cleanup.
		* Added configuration throw the global config. Parameters TCPSERVER_PORT
		and TCPSERVER_BUFSIZE no longer static and hard compiled variables.

*/
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <winsock2.h>
#include <process.h>

#include "vzVersion.h"
#include "vzMain.h"
#include "vzImage.h"
#include "vzTVSpec.h"
#include "vzLogger.h"

#include "main.h"

#define MAX_CLIENTS 32

// use winsock lib
#pragma comment(lib, "ws2_32.lib") 

// define buffer size and port
static long TCPSERVER_PORT = 8001;
static long TCPSERVER_BUFSIZE = 4096;

typedef enum LITERALS
{
	LITERAL_NOT_FOUND = 0,
	TAG_QUIT,
	TAG_SCENE,
	TAG_RENDERMAN_LOAD,
	TAG_RENDERMAN_SCREENSHOT
};

static char* literals[] = 
{
	"",
	"quit",
	"scene.",
	"renderman.load(",
	"renderman.screenshot("
};

static LITERALS FIND_FROM_LITERAL(char* &src,LITERALS literal)
{
	if
	(
		0 == strncmp
		(
			src,
			literals[literal],
			strlen(literals[literal])
		)
	)
	{
		src += strlen(literals[literal]);
		return literal;
	}
	else
		return LITERAL_NOT_FOUND;
};

static void FIND_TO_TERM(char* &src,char* &buf, char* term)
{
	char* temp;

	if(buf)
	{
		free(buf);
		buf = NULL;
	};

	if(temp = strstr(src,term))
	{
		strncpy
		(
			(char*)memset
			(
				buf = (char*)malloc(temp - src + 1),
				0,
				temp - src + 1
			),
			src,
			temp - src
		);
		src = temp + strlen(term);
	};
};

static char
	*shell_greeting_template = "%s (vz-%s) [tcpserver]\n",
	*shell_in = "\r\nvz in$> ",
	*shell_out = "vz out$> ",
	*shell_bye = "Bye!\n\r",
	*shell_error_overrun = "Error: Message too long",
	*shell_error_scene = "Error! Scene not loaded!",
	*shell_error_prenth = "Error! No final parenthesis found",
	*shell_ok_scene = "OK!Scene",
	*shell_ok_load = "OK!Load",
	*shell_ok_screenshot = "OK!Screenshot",
	*shell_error_recog = "Error! Command not recogized";



static int tcpserver_client_exec(char* buffer, char** error_log)
{
    int idx, cnt;
	*error_log = shell_error_recog;

	logger_printf(0, "recieved: '%s'",buffer);

	// clear escape characters
	for(char* p = buffer; (*p) ; p++)
	{
		// set flag
		char replace = 0;

		// check for escape character
		if('\\' == *p)
			switch(*(p+1))
			{
				case 'n': replace = 0x0A; break;
				case 'r': replace = 0x0D; break;
				case 't': replace = 0x08; break;
				case '\\': replace = '\\'; break;
			};

		// shift
		if(0 != replace)
		{
			*p = replace;
			for(char* t=(p + 1);(*t);t++) *t = *(t+1);
		};

	};

	// process command here
	if(FIND_FROM_LITERAL(buffer, TAG_QUIT))
	{
		logger_printf(0, "tcpserver: CMD quit recieved");
		*error_log = shell_bye;
		return 1;
	}
	else if (FIND_FROM_LITERAL(buffer,TAG_SCENE))
	{
		// send command tp
		logger_printf(0, "tcpserver: CMD scene \"%s\" recieved",buffer);
		*error_log = shell_ok_scene; 

        for(idx = 0, cnt = 0; idx < VZ_MAX_LAYERS; idx++)
        {
            if(!layers[idx]) continue;
            vzMainSceneCommand(layers[idx], buffer, error_log);
            cnt++;
        };
        if(!cnt)
            *error_log = shell_error_scene;
	}
	else if (FIND_FROM_LITERAL(buffer, TAG_RENDERMAN_LOAD))
	{
		char* buf = NULL;
		FIND_TO_TERM(buffer, buf, ")");
		if(buf)
		{
			// buf contains name of file
			// where scene name is stored
			logger_printf(0, "tcpserver: CMD load \"%s\" recieved",buf);
			*error_log = shell_ok_load;
            CMD_layer_load(buf, 0);
		}
		else
			*error_log = shell_error_prenth;
		
		if(buf) free(buf);
	}
	else if (FIND_FROM_LITERAL(buffer, TAG_RENDERMAN_SCREENSHOT))
	{
		char* buf = NULL;
		FIND_TO_TERM(buffer, buf, ")");
		if(buf)
		{
			// buf contains name of file
			// where screenshot stored
			logger_printf(0, "tcpserver: CMD screenshot \"%s\" recieved",buf);
			*error_log = shell_ok_screenshot;
			CMD_screenshot(buf,error_log);
		}
		else
			*error_log = shell_error_prenth;
		
		if(buf) free(buf);
	};

	return 0;
};

static unsigned long WINAPI tcpserver_client(void* _socket)
{
	SOCKET socket = (SOCKET)_socket;

	char
		hello_message[512],
		*in_buffer = (char*)malloc(2*TCPSERVER_BUFSIZE),
		*in_buffer_cmd = NULL,
		*error_log, 
		*in_buffer_head = in_buffer,
		*in_buffer_tail = in_buffer;


	/* send greeting */
	sprintf(hello_message, shell_greeting_template, VZ_TITLE, VZ_VERSION);
	send(socket, hello_message, (int)strlen(hello_message), 0);

	/* dialog loop */
	for(int r_s_in, r_rec_len = 0; r_rec_len >= 0; )
	{
		/* send prompt */
		r_s_in = send(socket, shell_in, (int)strlen(shell_in), 0);

		/* read data from socket buffer */
		for(r_rec_len = 0, r_rec_len = 0; r_rec_len >=0 ; )
		{
			/* check if error happend */
			if(r_rec_len >= 0)
			{
				/* move buffer tail */
				in_buffer_tail += r_rec_len;

				/* check if command found */
				for(; (in_buffer_head < in_buffer_tail) && (NULL == in_buffer_cmd) ; )
				{
					switch(*in_buffer_head)
					{
						case '\n':
							/* set termination symbol */
							*in_buffer_head = 0;

							/* allocate buffer for command and copy */
							in_buffer_cmd = (char*)malloc(in_buffer_head - in_buffer + 1);
							memcpy(in_buffer_cmd, in_buffer, in_buffer_head - in_buffer + 1);

							/* defragment buffer */
							memmove(in_buffer, in_buffer_head + 1, TCPSERVER_BUFSIZE);
							
							/* fix pointers */
							in_buffer_tail -= in_buffer_head - in_buffer + 1;
							in_buffer_head = in_buffer;
							break;

						case '\r':
							/* ignore character */
							memmove(in_buffer_head, in_buffer_head + 1, TCPSERVER_BUFSIZE);
							in_buffer_tail -= 1;
							break;

						default:
							in_buffer_head++;
							break;
					}		
				};
			};

			if(NULL != in_buffer_cmd)
				break;

			/* read data from socket */
			if((r_rec_len = recv(socket, in_buffer_tail, TCPSERVER_BUFSIZE, 0)) <= 0)
			{
				r_rec_len = -1;

				break;
			};
		};

		if(NULL != in_buffer_cmd)
		{
			/* ignore error read if command found */
			r_rec_len = 0;

			/* shell out */
			send(socket, shell_out, (int)strlen(shell_out), 0);

			/* execute command */
			if(0 != tcpserver_client_exec(in_buffer_cmd, &error_log))
				r_rec_len = -1;
			
			/* free buffer here */
			free(in_buffer_cmd);
			in_buffer_cmd = NULL;

			/* shell reply */
			send(socket, error_log, (int)strlen(error_log), 0);
		};
	};

	logger_printf(0, "tcpserver: closing connection.");

	free(in_buffer);
	closesocket(socket);
	return 0;
};


static struct
{
	HANDLE thread;
	SOCKET socket;
} 
srv_clients[MAX_CLIENTS];

static SOCKET socket_listen = INVALID_SOCKET;

void tcpserver_kill()
{
	shutdown(socket_listen, SD_BOTH);
	closesocket(socket_listen);
};

unsigned long WINAPI tcpserver(void* _config)
{
	char* temp;
	int j;
	unsigned long r;

	// cleanup handles array
	for(j=0;j<MAX_CLIENTS;j++) srv_clients[j].thread = INVALID_HANDLE_VALUE;

	// check if server is enabled
	if(!vzConfigParam(_config,"tcpserver","enable"))
	{
		logger_printf(0, "tcpserver: disabled");
		ExitThread(0);
	};

	// check for params
	if(temp = vzConfigParam(_config,"tcpserver","port"))
		TCPSERVER_PORT = atol(temp);
	if(temp = vzConfigParam(_config,"tcpserver","bufsize"))
		TCPSERVER_BUFSIZE = atol(temp);

	// create socket
	if (INVALID_SOCKET == (socket_listen = socket(AF_INET, SOCK_STREAM, 0)))
	{
		logger_printf(1, "tcpserver: socket() failed");
		ExitThread(0);
		return 0;
	};

	// socket addr struct
	sockaddr_in s_local;
	s_local.sin_family = AF_INET;
	s_local.sin_addr.s_addr = htonl(INADDR_ANY);;	// any interface
	s_local.sin_port = htons((unsigned short)TCPSERVER_PORT);		// 
    
	// try to bind addr struct to socket
	if (bind(socket_listen,(sockaddr*)&s_local, sizeof(sockaddr_in)) == SOCKET_ERROR) 
	{
		logger_printf(1, "tcpserver: bind() failed");
		ExitThread(0);
		return 0;
	};

	// listen socket
	listen(socket_listen, SOMAXCONN);

	// endless loop for server listener
	for(;0 == f_exit;)
	{
		sockaddr_in s_remote;
		int s_remote_size = sizeof(s_remote);

		// listen for incoming connection
		SOCKET socket_incoming = accept(socket_listen, (sockaddr*)&s_remote,&s_remote_size);

		// check if socket is valid
		if(socket_incoming == INVALID_SOCKET)
		{
			logger_printf(1, "tcpserver: accept() failed");
		}
		else
		{
			// notify system about incoming connection
			logger_printf(1, "tcpserver: accepted connection from %s:%ld",inet_ntoa(s_remote.sin_addr),s_remote.sin_port);

			// create thread for client operation
			for(j=0;j<MAX_CLIENTS;j++)
			{
				// cleanup hanldes
				if(srv_clients[j].thread != INVALID_HANDLE_VALUE)
					if(GetExitCodeThread(srv_clients[j].thread, &r))
						if(r != STILL_ACTIVE)
						{
							CloseHandle(srv_clients[j].thread);
							srv_clients[j].thread = INVALID_HANDLE_VALUE;
						};

				if(srv_clients[j].thread == INVALID_HANDLE_VALUE)
				{
					srv_clients[j].socket = socket_incoming;
					srv_clients[j].thread = CreateThread(0, 0, tcpserver_client, (void*)srv_clients[j].socket, 0, NULL);
					j = MAX_CLIENTS + 1;
				};
			}
			
			// all slots are busy
			if(j == MAX_CLIENTS)
				closesocket(socket_incoming);
		};
	};

	/* try to close all threads */
	for(j=0;j<MAX_CLIENTS;j++)
		if(srv_clients[j].thread != INVALID_HANDLE_VALUE)
		{
			/* force shutdown socket */
			shutdown(srv_clients[j].socket, SD_BOTH);
			closesocket(srv_clients[j].socket);

			/* wait for thread ends */
			WaitForSingleObject(srv_clients[j].thread, INFINITE);
			CloseHandle(srv_clients[j].thread);
		};

	closesocket(socket_listen);

	return 0;
};


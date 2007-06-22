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
	2007-03-10:
		*exiting from threads fixed

    2005-06-08:
		* Code cleanup.
		* Added configuration throw the global config. Parameters TCPSERVER_PORT
		and TCPSERVER_BUFSIZE no longer static and hard compiled variables.

*/

#include <stdio.h>
#include <winsock2.h>
#include <process.h>

#include "vzVersion.h"
#include "vzMain.h"
#include "vzImage.h"
#include "vzTVSpec.h"

extern int f_exit;
extern void* scene;	// scene loaded
extern HANDLE scene_lock;
extern void* functions;
extern int main_stage;
extern char screenshot_file[1024];
extern void* config;
extern vzTVSpec tv;

#define MAX_CLIENTS 32

static void CMD_screenshot(char* filename,char** error_log)
{
	// lock scene
	WaitForSingleObject(scene_lock,INFINITE);

	// copy filename
	strcpy(screenshot_file,filename);

	// unlock scene
	ReleaseMutex(scene_lock);
};

static void CMD_loadscene(char* filename,char** error_log)
{
	// lock scene
	if (WaitForSingleObject(scene_lock,INFINITE) != WAIT_OBJECT_0)
	{
		*error_log = "Error! Unable to lock scene handle";
		return;
	};

	if(scene)
		vzMainSceneFree(scene);

	scene = vzMainSceneNew(functions,config,&tv);

	if (!vzMainSceneLoad(scene, filename))
	{
		*error_log = "Error! Unable to load scene";
		vzMainSceneFree(scene);
		scene = NULL;
	};

	// unlock scene
	ReleaseMutex(scene_lock);
};



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
	*error_log = shell_error_recog;

	printf("recieved: '%s'\n",buffer);

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
		printf("tcpserver: CMD quit recieved\n");
		*error_log = shell_bye;
		return 1;
	}
	else if (FIND_FROM_LITERAL(buffer,TAG_SCENE))
	{
		// send command tp
		printf("tcpserver: CMD scene \"%s\" recieved\n",buffer);
		*error_log = shell_ok_scene; 
					
		// lock scene
//		WaitForSingleObject(scene_lock,INFINITE);
		if(scene)
			vzMainSceneCommand(scene, buffer,error_log);
		else
			*error_log = shell_error_scene;
//		ReleaseMutex(scene);
	}
	else if (FIND_FROM_LITERAL(buffer, TAG_RENDERMAN_LOAD))
	{
		char* buf = NULL;
		FIND_TO_TERM(buffer, buf, ")");
		if(buf)
		{
			// buf contains name of file
			// where scene name is stored
			printf("tcpserver: CMD load \"%s\" recieved\n",buf);
			*error_log = shell_ok_load;
			CMD_loadscene(buf,error_log);
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
			printf("tcpserver: CMD screenshot \"%s\" recieved\n",buf);
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
	send(socket,hello_message,strlen(hello_message),0);

	/* dialog loop */
	for(int r_s_in, r_rec_len = 0; r_rec_len >= 0; )
	{
		/* send prompt */
		r_s_in = send(socket,shell_in,strlen(shell_in),0);

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
			send(socket,shell_out,strlen(shell_out),0);

			/* execute command */
			if(0 != tcpserver_client_exec(in_buffer_cmd, &error_log))
				r_rec_len = -1;
			
			/* free buffer here */
			free(in_buffer_cmd);
			in_buffer_cmd = NULL;

			/* shell reply */
			send(socket,error_log,strlen(error_log),0);
		};
	};

	printf("tcpserver: closing connection.\n");

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
		printf("tcpserver: disabled\n");
		ExitThread(0);
	};

	// check for params
	if(temp = vzConfigParam(_config,"tcpserver","port"))
		TCPSERVER_PORT = atol(temp);
	if(temp = vzConfigParam(_config,"tcpserver","bufsize"))
		TCPSERVER_BUFSIZE = atol(temp);

	// init winsock
#define WS_VER_MAJOR 2
#define WS_VER_MINOR 2
	WSADATA wsaData;
	if ( WSAStartup( ((unsigned long)WS_VER_MAJOR) | (((unsigned long)WS_VER_MINOR)<<8), &wsaData ) != 0 )
	{
		printf("tcpserver: WSAStartup() failed\n");
		ExitThread(0);
		return 0;
	};

	// create socket
	if (INVALID_SOCKET == (socket_listen = socket(AF_INET, SOCK_STREAM, 0)))
	{
		printf("tcpserver: socket() failed\n");
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
		printf("tcpserver: bind() failed\n");
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
			printf("tcpserver: accept() failed\n");
		}
		else
		{
			// notify system about incoming connection
			printf("tcpserver: accepted connection from %s:%ld\n",inet_ntoa(s_remote.sin_addr),s_remote.sin_port);

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


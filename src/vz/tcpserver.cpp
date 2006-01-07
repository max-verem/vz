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
    2005-06-08:
		* Code cleanup.
		* Added configuration throw the global config. Parameters TCPSERVER_PORT
		and TCPSERVER_BUFSIZE no longer static and hard compiled variables.

*/

#include <stdio.h>
#include <winsock2.h>
#include <process.h>

#include "vzMain.h"
#include "vzImage.h"
#include "vzTVSpec.h"

extern void* scene;	// scene loaded
extern HANDLE scene_lock;
extern void* functions;
extern int main_stage;
extern char screenshot_file[1024];
extern void* config;
extern vzTVSpec tv;

extern const char* VZ_TITLE;
extern float VZ_VERSION_NUMBER;
extern const char* VZ_VERSION_SUFFIX;


void CMD_screenshot(char* filename,char* &error_log)
{
	// lock scene
	WaitForSingleObject(scene_lock,INFINITE);

	// copy filename
	strcpy(screenshot_file,filename);

	// unlock scene
	ReleaseMutex(scene_lock);
};

void CMD_loadscene(char* filename,char* &error_log)
{
	// lock scene
	if (WaitForSingleObject(scene_lock,INFINITE) != WAIT_OBJECT_0)
	{
		error_log = "Error! Unable to lock scene handle";
		return;
	};

	if(scene)
		vzMainSceneFree(scene);

	scene = vzMainSceneNew(functions,config,&tv);

	if (!vzMainSceneLoad(scene, filename))
	{
		error_log = "Error! Unable to load scene";
		vzMainSceneFree(scene);
		scene = NULL;
	};

	// unlock scene
	ReleaseMutex(scene_lock);
};



// use winsock lib
#pragma comment(lib, "ws2_32.lib") 

// define buffer size and port
long TCPSERVER_PORT = 8001;
long TCPSERVER_BUFSIZE = 4096;

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

LITERALS FIND_FROM_LITERAL(char* &src,LITERALS literal)
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

void FIND_TO_TERM(char* &src,char* &buf, char* term)
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




unsigned long WINAPI tcpserver_client(void* _socket)
{
	SOCKET socket = (SOCKET)_socket;

	char hello_message[512];
	sprintf(hello_message, "%s (vz-%.2f-%s) [tcpserver]\n",VZ_TITLE, VZ_VERSION_NUMBER,VZ_VERSION_SUFFIX);
	char* shell_message = "\r\nvz in$> ";
	char* shell_out = "vz out$> ";
	char* error_OVERRUN = "Error: Message too long";
	char* bye_message = "Bye!\n\r";
	char* in_buffer = (char*)malloc(TCPSERVER_BUFSIZE);
	long in_size;
	long exit_flag;

	send(socket,hello_message,strlen(hello_message),0);

	do
	{
		// exit flag - if all happents - reset flag
		exit_flag = 1;

		// send prompt
		if( send(socket,shell_message,strlen(shell_message),0) > 0 )
		{
			char* in_buffer_temp = in_buffer;
			int got_message = 0;
			// read data from client

			while((!(got_message)) && (in_buffer_temp - in_buffer < TCPSERVER_BUFSIZE) && (recv(socket,in_buffer_temp++,1,0)>0))
			{
//				printf("'%ld'\n",(unsigned long)(*(in_buffer_temp - 1)));

				// check if buffer contains terminal symbol
				if ((in_buffer_temp - in_buffer) > 1)
				{
					if
					(
						(*(in_buffer_temp - 1) == 10)
						&&
						(*(in_buffer_temp - 2) == 13)
					)
					{
						// set flag
						got_message = 1;

						// clean tail
						*(in_buffer_temp - 2) = 0;
						*(in_buffer_temp - 1) = 0;
					};
				};
			};

			if (got_message)
			{
				// process command
				in_size = in_buffer_temp - in_buffer - 2;
				if( send(socket,shell_out,strlen(shell_out),0) > 0 )
				{
					// reset flag
					exit_flag = 0;

					// clear escape characters
					for(char* p = in_buffer; (*p) ; p++)
					{
						// set flag
						char replace = 0;

						// check for escape character
						if((*p == '\\')&&(*(p+1) == 'n'))
							replace = 0x0A;
						if((*p == '\\')&&(*(p+1) == 't'))
							replace = 0x08;
						if((*p == '\\')&&(*(p+1) == 'r'))
							replace = 0x0D;
						if((*p == '\\')&&(*(p+1) == 't'))
							replace = 0x09;
						if((*p == '\\')&&(*(p+1) == '\\'))
							replace = '\\';

						// shift
						if(replace)
						{
							*p = replace;
							for(char* t=(p + 1);(*t);t++)
								*t = *(t+1);
						};

					};

					printf("recieved: '%s'\n",in_buffer);

					// process command here
					in_buffer_temp = in_buffer;
					char* error_log = "Error! Command not recogized";

					if(FIND_FROM_LITERAL(in_buffer_temp,TAG_QUIT))
					{
						printf("tcpserver: CMD quit recieved\n");
						error_log = bye_message;
						exit_flag = 1;
					}
					else if (FIND_FROM_LITERAL(in_buffer_temp,TAG_SCENE))
					{
						// send command tp
						printf("tcpserver: CMD scene \"%s\" recieved\n",in_buffer_temp);
						error_log = "OK!Scene";
					
						// lock scene
//						WaitForSingleObject(scene_lock,INFINITE);
						if(scene)
							vzMainSceneCommand(scene, in_buffer_temp,&error_log);
						else
							error_log = "Error! Scene not loaded!";
//						ReleaseMutex(scene);
					}
					else if (FIND_FROM_LITERAL(in_buffer_temp,TAG_RENDERMAN_LOAD))
					{
						char* buf = NULL;
						FIND_TO_TERM(in_buffer_temp,buf, ")");
						if(buf)
						{
							// buf contains name of file
							// where scene name is stored
							printf("tcpserver: CMD load \"%s\" recieved\n",buf);
							error_log = "OK!Load";
							CMD_loadscene(buf,error_log);
						}
						else
							error_log = "Error! No final parenthesis found";
						if(buf) free(buf);
					}
					else if (FIND_FROM_LITERAL(in_buffer_temp,TAG_RENDERMAN_SCREENSHOT))
					{
						char* buf = NULL;
						FIND_TO_TERM(in_buffer_temp,buf, ")");
						if(buf)
						{
							// buf contains name of file
							// where screenshot stored
							printf("tcpserver: CMD screenshot \"%s\" recieved\n",buf);
							error_log = "OK!Screenshot";

							CMD_screenshot(buf,error_log);
						}
						else
							error_log = "Error! No final parenthesis found";
						if(buf) free(buf);
					};

					if(send(socket,error_log,strlen(error_log),0)<=0)
						exit_flag = 1;

				};
			}
			else
			{
				// check if message is too long
				if (in_buffer_temp - in_buffer >= TCPSERVER_BUFSIZE)
				{
					// notify user about long message
					if( send(socket,shell_out,strlen(shell_out),0) > 0 )
						if( send(socket,error_OVERRUN,strlen(error_OVERRUN),0) > 0 )
							// reset exit flag
							exit_flag = 0;
				};
			};
				
		};
	}
	while(!(exit_flag));

	printf("tcpserver: closing connection.\n");

	free(in_buffer);
	closesocket(socket);
	ExitThread(0);
	return 0;
};


unsigned long WINAPI tcpserver(void* _config)
{
	char* temp;

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
	SOCKET socket_listen = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_listen == INVALID_SOCKET)
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
	for(;;)
	{
		sockaddr_in s_remote;
		int s_remote_size = sizeof(s_remote);

		// listen for incoming connection
		SOCKET socket_incoming = accept(socket_listen, (sockaddr*)&s_remote,&s_remote_size);

		// check if socket is valid
		if(socket_incoming == INVALID_SOCKET)
		{
			printf("tcpserver: accept() failed\n");
			ExitThread(0);
			return 0;
		};

		// notify system about incoming connection
		printf("tcpserver: accepted connection from %s:%ld\n",inet_ntoa(s_remote.sin_addr),s_remote.sin_port);

		// create thread for client operation
		unsigned long thread;
		CreateThread(0, 0, tcpserver_client, (void*)socket_incoming, 0, &thread);
	};

};


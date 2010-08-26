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
		*start
*/
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <winsock2.h>
#include <Ws2tcpip.h>

#include "vzVersion.h"
#include "vzMain.h"
#include "vzImage.h"
#include "vzTVSpec.h"
#include "vzLogger.h"

#include "main.h"
#include "serserver.h"

#include "../vzCmd/vz_cmd_send.h"

#define MAX_CLIENTS 32

// use winsock lib
#pragma comment(lib, "ws2_32.lib") 

// define buffer size and port
static long UDPSERVER_PORT = VZ_CMD_SEND_UDP_PORT;
static long UDPSERVER_BUFSIZE = VZ_CMD_SEND_UDP_BUF;

static SOCKET socket_listen = INVALID_SOCKET;

void udpserver_kill()
{
	shutdown(socket_listen, SD_BOTH);
	closesocket(socket_listen);
};

unsigned long WINAPI udpserver(void* _config)
{
	char* temp;
	void* buf;
	int r, e;
	int l;
	sockaddr_in s_local;
	sockaddr_in s_remote;
    struct ip_mreq imreq;

	/* check if server is enabled */
	if(!vzConfigParam(_config, "udpserver", "enable"))
	{
		logger_printf(0, "udpserver: disabled");
		ExitThread(0);
	}

	/* check for params */
	if(temp = vzConfigParam(_config, "udpserver", "port"))
		UDPSERVER_PORT = atol(temp);
	if(temp = vzConfigParam(_config, "udpserver", "bufsize"))
		UDPSERVER_BUFSIZE = atol(temp);

	/* create socket */
	if (INVALID_SOCKET == (socket_listen = socket(AF_INET, SOCK_DGRAM, 0)))
	{
		logger_printf(1, "udpserver: socket() failed");
		ExitThread(0);
		return 0;
	};

	/* socket addr struct */
	s_local.sin_family = AF_INET;
	s_local.sin_addr.s_addr = htonl(INADDR_ANY);				// any interface
	s_local.sin_port = htons((unsigned short)UDPSERVER_PORT);	// port defined
    
	/* try to bind addr struct to socket */
	if (SOCKET_ERROR == bind(socket_listen,(sockaddr*)&s_local, sizeof(sockaddr_in))) 
	{
		logger_printf(1, "udpserver: bind() failed");
err:
		closesocket(socket_listen);
		ExitThread(0);
		return 0;
	};

    if(temp = vzConfigParam(_config, "udpserver", "multicast"))
    {
        memset(&imreq, 0, sizeof(struct ip_mreq));
        imreq.imr_multiaddr.s_addr = inet_addr(temp);
        imreq.imr_interface.s_addr = INADDR_ANY; // use DEFAULT interface

        r = setsockopt(socket_listen, IPPROTO_IP, IP_ADD_MEMBERSHIP,
              (const char *)&imreq, sizeof(struct ip_mreq));
        if(SOCKET_ERROR == r)
        {
            logger_printf(1, "udpserver: setsockopt(IPPROTO_IP, IP_ADD_MEMBERSHIP, '%s') failed, r=%d",
                temp, r);
            goto err;
        };
    };

	/* initialize buffer */
	buf = malloc(UDPSERVER_BUFSIZE);

	/* endless loop */
	logger_printf(0, "udpserver: started on port %d, buffer %d bytes",
		UDPSERVER_PORT, UDPSERVER_BUFSIZE);
	for(e = 0; (0 == f_exit) && (0 == e); )
	{
		/* wait for packet */
		l = sizeof(sockaddr_in);
		r = recvfrom(socket_listen, (char*)buf, UDPSERVER_BUFSIZE, 0, 
			(sockaddr*)&s_remote, &l);

		/* check if error happens */
		if(SOCKET_ERROR == r)
			e = WSAGetLastError();
		else
		{
			/* try to execute command */
			l = r;
			r = serserver_cmd(buf, &l);

			/* output error */
			if(r < 0)
				logger_printf(1, "udpserver: serserver_cmd failed with %d for client %s:%ld", 
					r, inet_ntoa(s_remote.sin_addr),s_remote.sin_port);
		};
	};

	/* free buffer */
	free(buf);

	/* check if exit flag not rized */
	if(0 == f_exit)
		logger_printf(0, "udpserver: recvfrom failed with %d, exiting...", e);

	/* close socket */
	closesocket(socket_listen);

	return 0;
};


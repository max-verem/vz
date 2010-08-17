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
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#ifdef __linux__
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SNPRINTF snprintf
#else /* __linux__ */
#define WS_VER_MAJOR 2
#define WS_VER_MINOR 2
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#define SNPRINTF _snprintf
#endif /* __linux__ */

#include "vz_cmd.h"
#include "vz_cmd_send.h"

#define MAX_CMD_NAME 128

int vz_cmd_send(struct vz_cmd_send_target* dst, ...)
{
    int r;
    va_list ap;
    va_start(ap, dst);
    r = vz_cmd_send_va(dst, ap);
    va_end(ap);
    return r;
};

static int vz_cmd_send_udp(struct vz_cmd_send_target* dst, void* buf, int len)
{
	char* tmp;
	char host[128];
	int port, r, l;
	struct sockaddr_in addr;
	struct hostent *host_ip;
#ifndef __linux__
	SOCKET
#else
	int
#endif
		 s;

	/* check for hostname:port */
	tmp = strchr(dst->name, ':');
	if(NULL == tmp)
	{
		strncpy(host, dst->name, sizeof(host));
		port = VZ_CMD_SEND_UDP_PORT;
	}
	else
	{
		memcpy(host, dst->name, tmp - dst->name);
		host[tmp - dst->name] = 0;
		port = atol(tmp + 1);
	};

	/* resolv hostname */
    host_ip = gethostbyname(host);
    if(NULL == host_ip)
#ifndef __linux__
        return -WSAGetLastError();
#else /* __linux__ */
        return -errno;                    
#endif /* __linux__ */

    /* create communication socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);
#ifndef __linux__
    if(INVALID_SOCKET == s)
        return -WSAGetLastError();
#else /* __linux__ */
    if(-1 == s)
        return -errno;                    
#endif /* __linux__ */

	/* prepare address */
    addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)(host_ip->h_addr_list[0])));
	addr.sin_port = htons((unsigned short)port);

	/* send datagram */
	l = sizeof(struct sockaddr_in);
	r = sendto
	(
		s,							/* Socket to send result */
		buf,						/* The datagram buffer */
		len,						/* The datagram lngth */
		0,							/* Flags: no options */
		(struct sockaddr *)&addr,	/* addr */
		l							/* Server address length */
	);

#ifndef __linux__
    if(INVALID_SOCKET == r)
        r = -WSAGetLastError();
#else /* __linux__ */
    if(-1 == r)
        r = -errno;                    
#endif /* __linux__ */
	else
		r = 0;

	/* close socket */
#ifndef __linux__
    closesocket(s);
#else /*! __linux__ */
    close(s);
#endif /* __linux__ */

	return r;
};

int vz_cmd_send_va(struct vz_cmd_send_target* dst, va_list ap)
{
	void* buf;
	int r, l = VZ_CMD_SEND_UDP_BUF;

	if(VZ_CMD_SEND_UDP != dst->transport)
		return -EINVAL;

	/* compose buffer */
	buf = malloc(l);

	/* create a command */
	r = vz_serial_cmd_create_va(buf, &l, ap);

	if(0 != r)
		r = -EINVAL;
	else
		r = vz_cmd_send_udp(dst, buf, l);

	/* free buffer */
	free(buf);

	return r;
};

int vz_cmd_send_init()
{
#ifndef __linux__
	WSADATA wsaData;

	if ( WSAStartup( ((unsigned long)WS_VER_MAJOR) | (((unsigned long)WS_VER_MINOR)<<8), &wsaData ) != 0 )
		return -WSAGetLastError();
#else /* !__linux__ */

#endif /* !__linux__ */

	return 0;
};

int vz_cmd_send_release()
{
	return 0;
};

static int cmdlist_from_strlist(char** argv, int argc, void ***cmds, unsigned int **cmds_args, char* error)
{
	int e = 0, i;
	int cmd_id, idx_cmd = 0, idx_arg = 0, cmd_cnt = 0;
	char cmd_name[MAX_CMD_NAME];

	/* allocate lists */
	*cmds = (void**)malloc((1 + argc) * sizeof(void*));
	*cmds_args = (unsigned int*)malloc((1 + argc) * sizeof(unsigned int));

	while((0 == e) && (0 != argc))
	{
		/* compose name of command */
		SNPRINTF(cmd_name, MAX_CMD_NAME, "VZ_CMD_%s", *argv);

		/* lookup for command */
		cmd_id = vz_cmd_lookup_by_name(cmd_name);

		/* check */
		if(cmd_id)
		{
			cmd_cnt++;
			(*cmds)[idx_cmd++] = (void*)cmd_id;
			argc--; argv++;

			/* process command arguments */
		    switch(cmd_id)
			{
				case VZ_CMD_LOAD_SCENE:
				case VZ_CMD_SCREENSHOT:
				case VZ_CMD_CONTINUE_DIRECTOR:
				case VZ_CMD_STOP_DIRECTOR:

					/* name parameter */
					if(argc)
						(*cmds)[idx_cmd++] = (void*)*argv;
					else
						e = -2;
					argc--; argv++;
					break;

                case VZ_CMD_LAYER_UNLOAD:

                    /* integer argument */
                    if(argc)
                    {
                        (*cmds_args)[idx_arg] = (unsigned int)atol(*argv);
                        (*cmds)[idx_cmd++] = (void*)&((*cmds_args)[idx_arg++]);
                    }
                    else
                        e = -2;
                    argc--; argv++;
                    break;

				case VZ_CMD_START_DIRECTOR:
				case VZ_CMD_RESET_DIRECTOR:
				case VZ_CMD_CONTAINER_VISIBLE:
                case VZ_CMD_LAYER_LOAD:

					/* name paramer */
					if(argc)
						(*cmds)[idx_cmd++] = (void*)*argv;
					else
						e = -2;
					argc--; argv++;

					/* integer argument */
					if(argc)
					{
						(*cmds_args)[idx_arg] = (unsigned int)atol(*argv);
						(*cmds)[idx_cmd++] = (void*)&((*cmds_args)[idx_arg++]);
					}
					else
						e = -2;
					argc--; argv++;
					break;

				case VZ_CMD_SET:
					for(i = 0; (i < 3) && (0 == e); i++)
					{
						/* name paramer */
						if(argc)
							(*cmds)[idx_cmd++] = (void*)*argv;
						else
							e = -2;
						argc--; argv++;
					};
					break;
			};

			/* error notify for required arg */
			if(-2 == e)
				if(NULL != error)
					sprintf(error, "unknown command [%s]", cmd_name);
		}
		else
		{
			e = -1;
			if(NULL != error)
				sprintf(error, "unknown command [%s]", cmd_name);
		};
	};

	/* setup terminator */
	(*cmds)[idx_cmd++] = NULL;

	return (0 == e)?cmd_cnt:e;
};

int vz_cmd_send_strlist_udp(char* host, char** argv, int argc, char* error)
{
	int r;
	struct vz_cmd_send_target dst;
	void **cmds = NULL;
    unsigned int *cmds_args = NULL;

	/* init target struct */
	dst.transport = VZ_CMD_SEND_UDP;
	dst.name = host;

	/* transform symbols */
	r = cmdlist_from_strlist(argv, argc , &cmds, &cmds_args, error);

	/* send */
	if(r > 0)
		r = vz_cmd_send_va(&dst, (va_list)cmds);

	/* free lists */
	if(NULL != cmds) free(cmds);
	if(NULL != cmds_args) free(cmds_args);

	return r;
};

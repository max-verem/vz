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
#ifndef VZ_CMD_SEND_H
#define VZ_CMD_SEND_H

#ifdef __linux__
#define VZCMDSEND_API
#define VZCMDSEND_API_STDCALL
#else
#define VZCMDSEND_API_STDCALL __stdcall
#ifdef VZCMDSEND_EXPORTS
#define VZCMDSEND_API __declspec(dllexport)
#else /* !VZCMD_EXPORTS */
#define VZCMDSEND_API __declspec(dllimport)
#pragma comment(lib, "vzCmdSend.lib") 
#endif /* VZCMD_EXPORTS */

#endif /* __linux__ */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define VZ_CMD_SEND_UDP			1
#define VZ_CMD_SEND_TCP			2
#define VZ_CMD_SEND_SERIAL		3

#define VZ_CMD_SEND_UDP_PORT	8002
#define VZ_CMD_SEND_UDP_BUF		65536
#define VZ_CMD_SEND_TCP_PORT	8001

struct vz_cmd_send_target
{
	int transport;
	char* name;
};

VZCMDSEND_API int vz_cmd_send_init();
VZCMDSEND_API int vz_cmd_send_release();
VZCMDSEND_API int vz_cmd_send(struct vz_cmd_send_target* dst, ...);
VZCMDSEND_API int vz_cmd_send_va(struct vz_cmd_send_target* dst, va_list ap);
VZCMDSEND_API int VZCMDSEND_API_STDCALL vz_cmd_send_strlist_udp(char* host, char** argv, int argc, char* error);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* VZ_CMD_SEND_H */

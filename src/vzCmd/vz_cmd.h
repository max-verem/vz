/*
    vz_cmd
    (VZ (ViZualizator) control protocol commands creator/parser)

    Copyright (C) 2007 Maksym Veremeyenko.
    This file is part of Airforce project (tv broadcasting/production
    automation support)

    vz_cmd is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    vz_cmd is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vz_cmd; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef VZ_CMD_H
#define VZ_CMD_H

#ifdef __linux__
#else

#ifdef VZCMD_EXPORTS
#define VZCMD_API __declspec(dllexport)
#else /* !VZCMD_EXPORTS */
#define VZCMD_API __declspec(dllimport)
#pragma comment(lib, "vzCmd.lib") 
#endif /* VZCMD_EXPORTS */

#endif /* __linux__ */

#include <stdarg.h>

enum vz_serial_cmd_enum
{
    VZ_CMD_LOAD_SCENE = 1,
    VZ_CMD_START_DIRECTOR,
    VZ_CMD_RESET_DIRECTOR,
    VZ_CMD_CONTINUE_DIRECTOR,
    VZ_CMD_STOP_DIRECTOR,
    VZ_CMD_SET,
    VZ_CMD_PING,
	VZ_CMD_CONTAINER_VISIBLE,
	VZ_CMD_SCREENSHOT
};

#define VZ_SERIAL_CMD_STH			0x02

#define VZ_EINVAL				1
#define VZ_EBUSY				2
#define VZ_EFAIL				3

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern VZCMD_API int vz_cmd_lookup_by_name(char* name);
extern VZCMD_API char* vz_cmd_get_name(int id);

extern VZCMD_API int vz_serial_cmd_count(void* _buf);
extern VZCMD_API int vz_serial_cmd_id(void* _buf, int index);
extern VZCMD_API int vz_serial_cmd_parseNcopy(void* _buf, int index, ...);
extern VZCMD_API int vz_serial_cmd_parseNmap(void* _buf, int index, ...);
extern VZCMD_API int vz_serial_cmd_probe(void* _buf, int* _len);
extern VZCMD_API int vz_serial_cmd_create_va(void* _buf, int* _len, va_list ap);
extern VZCMD_API int vz_serial_cmd_create(void* _buf, int* _len, ...);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* VZ_CMD_H */


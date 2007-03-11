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

#ifndef VZ_CMD_PRIVATE_H
#define VZ_CMD_PRIVATE_H

struct vz_cmd_desc
{
    int id;
    char name[128];
    int args_count;
    int* args_length;
};

void* vz_cmd_lookup_by_id(int id);

#endif  /* VZ_CMD_PRIVATE_H */

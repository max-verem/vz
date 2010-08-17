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


#include <string.h>
#include "vz_cmd.h"
#include "vz_cmd-private.h"

static int 
    alLOAD_SCENE[] 			= {-1},
    alSTART_DIRECTOR[] 		= {-1, 4},
    alRESET_DIRECTOR[]		= {-1, 4},
    alCONTINUE_DIRECTOR[]	= {-1},
    alSTOP_DIRECTOR[]		= {-1},
    alSET[]					= {-1, -1, -1},
    alPING[]				= {0},
	alCONTAINER_VISIBLE[]	= {-1, 4},
    alSCREENSHOT[]          = {-1},
    alLAYER_LOAD[]          = {-1, 4},
    alLAYER_UNLOAD[]        = {4}
;

#define REG_CMD(ID, ARGS, LENS)						\
{													\
    ID,												\
    #ID,											\
    ARGS,											\
    LENS											\
}

static struct vz_cmd_desc commands[] = 
{
    REG_CMD(VZ_CMD_LOAD_SCENE,			1, alLOAD_SCENE),
    REG_CMD(VZ_CMD_START_DIRECTOR,		2, alSTART_DIRECTOR),
    REG_CMD(VZ_CMD_RESET_DIRECTOR,		2, alRESET_DIRECTOR),
    REG_CMD(VZ_CMD_CONTINUE_DIRECTOR,	1, alCONTINUE_DIRECTOR),
    REG_CMD(VZ_CMD_STOP_DIRECTOR,		1, alSTOP_DIRECTOR),
    REG_CMD(VZ_CMD_SET,					3, alSET),
    REG_CMD(VZ_CMD_PING,				0, alPING),
	REG_CMD(VZ_CMD_CONTAINER_VISIBLE,	2, alCONTAINER_VISIBLE),
	REG_CMD(VZ_CMD_SCREENSHOT,			1, alSCREENSHOT),
    REG_CMD(VZ_CMD_LAYER_LOAD,          2, alLAYER_LOAD),
    REG_CMD(VZ_CMD_LAYER_UNLOAD,        1, alLAYER_UNLOAD),

    REG_CMD(0, 0, NULL)
};


int vz_cmd_lookup_by_name(char* name)
{
    int i;
    
    for(i = 0; commands[i].id; i++)
	if(0 == strcmp(name, commands[i].name))
	    return commands[i].id;
	    
    return 0;
};

void* vz_cmd_lookup_by_id(int id)
{
    int i;
    
    for(i = 0; commands[i].id; i++)
	if(id == commands[i].id)
	    return &commands[i];
	    
    return NULL;
};

char* vz_cmd_get_name(int id)
{
    int i;
    
    for(i = 0; commands[i].id; i++)
	if(id == commands[i].id)
	    return commands[i].name;
	    
    return NULL;
};


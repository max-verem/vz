/*
    vz_cmd
    (VZ (ViZualizator) control protocol commands creator/parser)

    Copyright (C) 2009 Maksym Veremeyenko.
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

static unsigned char vz_serial_cmd_cs(unsigned char* datas, int len)
{
    unsigned char cs = 0;
    int i;

    for(i=0; i< len ;i++) cs += datas[i];
    cs ^= 0xFF;
    cs += 1;

    return cs;
}

static int vz_serial_cmd_create_single(void* _buf, int* _len, int _id, va_list *ap)
{
    int i;
    unsigned char *args_ptr, *arg;
    unsigned char *buf_start = (unsigned char*)_buf;
    int *args_len = (int*)(buf_start + 1);
    
    /* lookup cmd */
    struct vz_cmd_desc* desc = (struct vz_cmd_desc*)vz_cmd_lookup_by_id(_id);

    /* reset len */
    *_len = 0;
    
    /* setup / map fields */
    *buf_start = (unsigned char)desc->id;
    *_len = 1 + desc->args_count*4;
    args_ptr = buf_start + (*_len);
    
    /* pop args */
    for(i = 0; i< desc->args_count; i++)
    {
	/* pop arg */
	arg = va_arg(*ap, unsigned char*);

	/* arg len */
	args_len[i]
	    = 
	    (-1 == desc->args_length[i])
	    ?
	    (strlen((char*)arg) + 1)
	    :
	    desc->args_length[i];
	
	/* copy data */
	memcpy(args_ptr, arg, args_len[i]);
	
	/* shift ptr */
	args_ptr += args_len[i];
	*_len = (*_len) + args_len[i];
    };
    
    
    return 0;    
};

int vz_serial_cmd_count(void* _buf)
{
    unsigned char *buf = (unsigned char*)_buf;
    return (unsigned int)buf[1];
};

int vz_serial_cmd_id(void* _buf, int index)
{
    unsigned char *buf = (unsigned char*)_buf;
    unsigned char *buf_cmd = buf + 6;
    int i, c = (unsigned int)buf[1];
    
    /* check index range */
    if(c <= index) return -VZ_EINVAL;
    
    /* skip forward */
    for(i = 0; (i< index) ; i++) buf_cmd += *((int*)buf_cmd) + 4;

    /* return command */
    return buf_cmd[4];
};

int vz_serial_cmd_parseNcopy(void* _buf, int index, ...)
{
    return -VZ_EINVAL;
};

int vz_serial_cmd_parseNmap(void* _buf, int index, ...)
{
    va_list ap;
    struct vz_cmd_desc* desc;
    unsigned char** arg;
    unsigned char* arg_buf;
    unsigned char *buf = (unsigned char*)_buf;
    unsigned char *buf_cmd = buf + 6;
    int i, c = (unsigned int)buf[1];
    
    /* check index range */
    if(c <= index) return -VZ_EINVAL;
    
    /* skip forward */
    for(i = 0; (i< index) ; i++) buf_cmd += *((int*)buf_cmd) + 4;

    /* cmd len */
    c = *((int*)buf_cmd);
    buf_cmd += 4;
    
    /* find command descriptin */
    desc = (struct vz_cmd_desc*)vz_cmd_lookup_by_id( *buf_cmd );
    
    /* check if defined */
    if(NULL == desc) return -VZ_EINVAL;
    
    /* goto first arg */
    buf_cmd += 1;
    arg_buf = buf_cmd + 4*desc->args_count;

    /* map */
    va_start(ap, index);
    for(i = 0; i<desc->args_count ; i++)
    {
	/* get var ptr */
	arg = va_arg(ap, unsigned char**);
	
	/* map */
	*arg = arg_buf;
	
	/* goto to next */
	arg_buf += ((int*)buf_cmd)[i];
    };
    va_end(ap);
    
    /* return success */
    return 0;
};

int vz_serial_cmd_probe(void* _buf, int* _len)
{
    int l;
    unsigned char *buf = (unsigned char*)_buf;

    /* check len #1 */
    if(0 == *_len) return -VZ_EINVAL;
    
    /* check STH */
    if(VZ_SERIAL_CMD_STH != *buf) return -VZ_EINVAL;
    
    /* check len #1 - unsure if fail */
    if(*_len < 6) return -VZ_EBUSY;
    
    /* retieve length */
    l = *((int*)( buf + 2 ));

    /* partial */
    if((l + 7) > *_len) return -VZ_EBUSY;

    /* check control sum */
    if
    (
	buf[l + 6] 
	!=
	vz_serial_cmd_cs(buf + 1, l + 5)
    )
	 return -VZ_EFAIL;
    
    /* set appropriate length */
    *_len = l + 7;

    /* success */    
    return 0;    
};

int vz_serial_cmd_create_va(void* _buf, int* _len, va_list ap)
{
    int id, l, r = 0;
    struct vz_cmd_desc* desc;
    unsigned char *buf = (unsigned char*)_buf;
    unsigned char* cmds_count = (unsigned char*)(buf + 1);
    int* cmds_length = (int*)(buf + 2);
    unsigned char *buf_data = buf + 6;

    /* reset vals / setup */
    *buf = VZ_SERIAL_CMD_STH;
    *cmds_count = 0;
    *cmds_length = 0;
    
    /* retrieve args */    
    do
    {
		/* pop command id */
		id = va_arg(ap, int);

		/* lookup cmd */
		desc = (struct vz_cmd_desc*)vz_cmd_lookup_by_id(id);

		/* check if id define */
		if(NULL != desc)
		{
			/* put command into packet */
			vz_serial_cmd_create_single(buf_data + 4, (int*)buf_data, id, &ap);

		    /* calc offset and lengths */
			l = *((int*)buf_data) + 4;
			*cmds_length = (*cmds_length) +  l;
			*cmds_count = (*cmds_count) + 1;
			buf_data += l;
		}
		else
		{
			if(0 != id)
				r = -VZ_EINVAL;
		}
    }
    while(NULL != desc);
    
    /* fix buffer length */
    *_len = 7 + *cmds_length;

    /* control sum */
    buf[*_len - 1] = vz_serial_cmd_cs(buf + 1, *_len - 2);
    
    return r;
};

int vz_serial_cmd_create(void* _buf, int* _len, ...)
{
    int r;
    va_list ap;
    va_start(ap, _len);
    r = vz_serial_cmd_create_va(_buf, _len, ap);
    va_end(ap);
    return r;
};

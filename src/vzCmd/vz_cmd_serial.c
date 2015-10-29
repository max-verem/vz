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
//#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "vz_cmd.h"
#include "vz_cmd-private.h"

static unsigned char vz_serial_cmd_cs(uint8_t* datas, int len)
{
    uint8_t cs = 0;
    int i;

    for(i=0; i< len ;i++) cs += datas[i];
    cs ^= 0xFF;
    cs += 1;

    return cs;
}

static int vz_serial_cmd_create_single2(uint8_t* dst, int len_limit, struct vz_cmd_desc* desc, void*** pargs)
{
    int len, l, i;
    void** args = *pargs;
    uint8_t* dst_args_data;
    uint32_t* dst_args_len;

    /* set command */
    *dst = (uint8_t)desc->id;

    /* setup pointers */
    len = 1 + desc->args_count * sizeof(uint32_t);
    dst_args_data = dst + len;
    dst_args_len  = (uint32_t*)(dst + 1);

    /* load args into list */
    for(i = 0; i< desc->args_count; i++)
    {
        /* arg len */
        l =
            (-1 == desc->args_length[i]) ?
            ((uint32_t)strlen((char*)(*args)) + 1) :
            (uint32_t)desc->args_length[i];

        /* check if reached end of buffer */
        if(l + len >= len_limit)
            return -VZ_ENOMEM;

        /* copy data */
        memcpy(dst_args_data, *args, l);

        /* shift ptrs */
        dst_args_data += l;
        len += l;
        dst_args_len[i] = l;
        args++;
    };

    *pargs = args;

    return len;
};

static int vz_serial_cmd_create_single(uint8_t* _buf, uint32_t* _len, int _id, va_list *ap)
{
    int i;
    uint8_t *args_ptr, *arg;
    uint8_t *buf_start = (uint8_t*)_buf;
    uint32_t *args_len = (uint32_t*)(buf_start + 1);

    /* lookup cmd */
    struct vz_cmd_desc* desc = (struct vz_cmd_desc*)vz_cmd_lookup_by_id(_id);

    /* reset len */
    *_len = 0;

    /* setup / map fields */
    *buf_start = (uint8_t)desc->id;
    *_len = 1 + desc->args_count * sizeof(uint32_t);
    args_ptr = buf_start + (*_len);

    /* pop args */
    for(i = 0; i< desc->args_count; i++)
    {
        /* pop arg */
        arg = (uint8_t*)va_arg(*ap, void*);

        /* arg len */
        args_len[i] = 
            (-1 == desc->args_length[i]) ?
            ((uint32_t)strlen((char*)arg) + 1) :
            (uint32_t)desc->args_length[i];

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
    uint8_t *buf = (uint8_t*)_buf;
    return (unsigned int)buf[1];
};

int vz_serial_cmd_id(void* _buf, int index)
{
    uint8_t *buf = (uint8_t*)_buf;
    uint8_t *buf_cmd = buf + 6;
    int i, c = (unsigned int)buf[1];

    /* check index range */
    if(c <= index) return -VZ_EINVAL;

    /* skip forward */
    for(i = 0; (i< index) ; i++) buf_cmd += *((uint32_t*)buf_cmd) + sizeof(uint32_t);

    /* return command */
    return (unsigned int)buf_cmd[4];
};

int vz_serial_cmd_parseNcopy(void* _buf, int index, ...)
{
    return -VZ_EINVAL;
};

int vz_serial_cmd_parseNmap(void* _buf, int index, ...)
{
    va_list ap;
    struct vz_cmd_desc* desc;
    void** arg;
    uint8_t* arg_buf;
    uint8_t *buf = (uint8_t*)_buf;
    uint8_t *buf_cmd = buf + 6;
    int i;
    uint32_t c = (unsigned int)buf[1];

    /* check index range */
    if(c <= (unsigned)index) return -VZ_EINVAL;

    /* skip forward */
    for(i = 0; (i < index) ; i++) buf_cmd += *((uint32_t*)buf_cmd) + sizeof(uint32_t);

    /* cmd len */
    c = *((uint32_t*)buf_cmd);
    buf_cmd += sizeof(uint32_t);

    /* find command descriptin */
    desc = (struct vz_cmd_desc*)vz_cmd_lookup_by_id( *buf_cmd );

    /* check if defined */
    if(NULL == desc) return -VZ_EINVAL;

    /* goto first arg */
    buf_cmd += 1;
    arg_buf = buf_cmd + sizeof(uint32_t) * desc->args_count;

    /* map */
    va_start(ap, index);
    for(i = 0; i < desc->args_count ; i++)
    {
        /* get var ptr */
        arg = (unsigned char**)va_arg(ap, void*);

        /* map */
        *arg = arg_buf;

        /* goto to next */
        arg_buf += ((uint32_t*)buf_cmd)[i];
    };
    va_end(ap);

    /* return success */
    return 0;
};

int vz_serial_cmd_probe(void* _buf, int* _len)
{
    uint32_t l;
    uint8_t *buf = (uint8_t*)_buf;

    /* check len #1 */
    if(0 == *_len) return -VZ_EINVAL;

    /* check STH */
    if(VZ_SERIAL_CMD_STH != *buf) return -VZ_EINVAL;

    /* check len #1 - unsure if fail */
    if(*_len < 6) return -VZ_EBUSY;

    /* retieve length */
    l = *((uint32_t*)( buf + 2 ));

    /* partial */
    if((l + 7) > (unsigned)*_len) return -VZ_EBUSY;

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
    uint8_t *buf = (uint8_t*)_buf;
    uint8_t *cmds_count = (uint8_t*)(buf + 1);
    uint32_t *cmds_length = (uint32_t*)(buf + 2);
    uint8_t *buf_data = buf + 6;

    /* reset vals / setup */
    *buf = VZ_SERIAL_CMD_STH;
    *cmds_count = 0;
    *cmds_length = 0;

    /* retrieve args */
    do
    {
        /* pop command id */
        id = (int)va_arg(ap, void*);

        /* lookup cmd */
        desc = (struct vz_cmd_desc*)vz_cmd_lookup_by_id(id);

        /* check if id define */
        if(NULL != desc)
        {
            /* put command into packet */
            vz_serial_cmd_create_single(buf_data + sizeof(uint32_t), (uint32_t*)buf_data, id, &ap);

            /* calc offset and lengths */
            l = *((uint32_t*)buf_data) + sizeof(uint32_t);
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

int vz_serial_cmd_create_list(void* dst, int len_limit, void** args)
{
    int cmd_cnt = 0, cmd_len = 0;
    uint8_t *head = (uint8_t*)dst + 6;

    while(*args)
    {
        int r, id = (int32_t)(*args);
        struct vz_cmd_desc* desc = (struct vz_cmd_desc*)vz_cmd_lookup_by_id(id);

        args++;

        if(!desc)
            return -VZ_EINVAL;

        r = vz_serial_cmd_create_single2(head + sizeof(uint32_t), len_limit - cmd_len, desc, &args);
//fprintf(stderr, "%s:%d: r=%.8X\n", __FUNCTION__, __LINE__, r);
        if(r > 0)
        {
            *((uint32_t*)head) = r;
            r += sizeof(uint32_t);
            cmd_cnt++;
            cmd_len += r;
            head += r;
//fprintf(stderr, "%s:%d: r=%.8X cmd_len=%.8X\n", __FUNCTION__, __LINE__, r, cmd_len);
        };

    };

    /* setup */
    *((uint8_t*)dst + 0) = VZ_SERIAL_CMD_STH;           // STH
    *((uint8_t*)dst + 1) = cmd_cnt;                     // CMD_CNT
    *((uint32_t*)((uint8_t*)dst + 2)) = cmd_len;        // CMD_LEN
    *((uint8_t*)dst + cmd_len + 6) =                    // CS
        vz_serial_cmd_cs((uint8_t*)dst + 1, cmd_len + 5);

    return cmd_len + 7; // STH(1) + CMD_CNT(1) + CMD_LEN(4) + CS(1)
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

#ifndef vzOutputBuffers_h
#define vzOutputBuffers_h

#define VZOUTPUT_MAX_BUFS 4

typedef struct vzOutputBuffers_desc
{
    long flags;

    HANDLE lock;

    unsigned long pos_driver;                       /* buffer id for io driver */
    unsigned long pos_render;                       /* buffer id for rendering  */

    unsigned long pos_driver_jump;                  /* indicate needs to jump forward */
    unsigned long pos_render_jump;                  /* indicate needs to jump forward */

    unsigned long pos_driver_dropped;
    unsigned long pos_render_dropped;

    unsigned long cnt_render;

    struct
    {
        int id;
        void* data;
        unsigned int num;
        HANDLE lock;
    } buffers[VZOUTPUT_MAX_BUFS];

    long gold;
    long size;
    long offset;
} vzOutputBuffers_t;

#endif /* vzOutputBuffers_h */

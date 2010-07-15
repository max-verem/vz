#ifndef vzImageLoadBMP_h
#define vzImageLoadBMP_h

#include "vzImageLoad.h"

static int vzImageLoadBMP(vzImage** pimg, char* filename)
{
    FILE* f;

    if(!pimg) return -1;

    /* open file */
    f = fopen(filename, "rb");
    if(!f) return -1;

    /* get file size */
    fseek(f, 0, SEEK_END);
    int bmp_file_size_real = ftell(f);
    fseek(f, 0, SEEK_SET);

    /* check for minimal length */
    if(bmp_file_size_real < 40)
    {
        fclose(f);
        return -2;
    };

    int bmp_identifier  = fio_get_le16(f); /* the characters identifying the bitmap. the following entries are possible */

    /* check file signature */
    if(0x00004d42 != bmp_identifier) /* BM */
    {
        fclose(f);
        return -2;
    };

    int bmp_file_size   = fio_get_le32(f); /* complete file size in bytes. */

    /* check if file is not broken */
    if(bmp_file_size != bmp_file_size_real)
    {
        fclose(f);
        return -3;
    };

                          fio_get_le32(f); /* reserved 1 dword */
    int bmp_data_offset = fio_get_le32(f); /* offset from beginning of file to the beginning of the bitmap data. */
    int bmp_header_size = fio_get_le32(f); /* length of the bitmap info header used to describe the bitmap colors, compression, the following sizes are possible */
    int bmp_width       = fio_get_le32(f); /* horizontal width of bitmap in pixels. */
    int bmp_height      = fio_get_le32(f); /* vertical height of bitmap in pixels. */
    int bmp_planes      = fio_get_le16(f); /* number of planes in this bitmap. */
    int bmp_bpp         = fio_get_le16(f); /* bits per pixel used to store palette entry information. this also identifies in an indirect way the number of possible colors */

    /* check for supported color depth */
    int pix_fmt;
    switch(bmp_bpp)
    {
        case 32: pix_fmt = VZIMAGE_PIXFMT_XBGR; break;
        case 24: pix_fmt = VZIMAGE_PIXFMT_BGR;  break;
        case  8: pix_fmt = VZIMAGE_PIXFMT_GRAY; break;
        default:
        {
            fclose(f);
            return -4;
        };
    };

    int bmp_compression = fio_get_le32(f); /* compression specifications. the following values are possible */

    /* we support onlu uncompressed */
    if
    (
        0 != bmp_compression /* 0 - none (also identified by bi_rgb) */ && 
        3 != bmp_compression /* 3 - bitfields (also identified by bi_bitfields) */
    )
    {
        fclose(f);
        return -3;
    };

    int bmp_data_size   = fio_get_le32(f);
    int bmp_hresolution = fio_get_le32(f); /* horizontal resolution expressed in pixel per meter. */
    int bmp_vresolution = fio_get_le32(f); /* vertical resolution expressed in pixels per meter. */
    int bmp_colors      = fio_get_le32(f); /* number of colors used by this bitmap. for a 8-bit / pixel bitmap this will be 100h or 256. */
    int bmp_imp_colors  = fio_get_le32(f); /* number of important colors. this number will be equal to the number of colors when every color is important. */

    /* we do not support pallete */
    if(bmp_colors)
    {
        fclose(f);
        return -3;
    };

    /* create image */
    vzImage* img;
    int r = vzImageCreate(&img, bmp_width, bmp_height, pix_fmt);
    if(r)
    {
        fclose(f);
        return r;
    };

    /* load data */
    fseek(f, bmp_data_offset, SEEK_SET);
    for(int i = 0, j; i < bmp_height; i++)
    {
        j = bmp_height - 1 - i;
        fio_get_buffer(f, img->lines_ptr[j], img->line_size);
    };

    fclose(f);

    *pimg = img;

    return 0;
};

#endif /* vzImageLoadBMP_h */

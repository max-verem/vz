#ifndef vzImageLoadTGA_h
#define vzImageLoadTGA_h

#include "vzImageLoad.h"

static int vzImageLoadTGA(vzImage** pimg, char* filename)
{
    FILE* f;

    if(!pimg) return -1;

    /* open file */
    f = fopen(filename, "rb");
    if(!f) return -1;

    /* get file size */
    fseek(f, 0, SEEK_END);
    int tga_file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    /* check for minimal length */
    if(tga_file_size < 18)
    {
        fclose(f);
        return -2;
    };

    /* "ID Length - Field 1 (1 byte)" */
    int tga_id_length = fio_get_byte(f);

    /* "Color Map Type - Field 2 (1 byte)" */
    int tga_color_map_type = fio_get_byte(f);

    /* "Image Type - Field 3 (1 byte)" */
    int tga_image_type = fio_get_byte(f);

    /* check type supported */
    if
    (
        (2  != tga_image_type) &&   /* uncompressed, true-color image */
        (3  != tga_image_type) &&   /* uncompressed, black-and-white image */
        (10 != tga_image_type) &&   /* run-length encoded, true-color image */
        (11 != tga_image_type)      /*run-length encoded, black-and-white Image */
    )
    {
        fclose(f);
        return -2;
    };

    /* we supports only "no color-map data is included with this image" */
    if(tga_color_map_type)
    {
        fclose(f);
        return -3;
    };

    /* skip "Color Map Specification - Field 4 (5 bytes)" */
    fio_skip(f, 5);

    /* read "Image Specification - Field 5 (10 bytes)" */
    int tga_x_origin = fio_get_le16(f);
    int tga_y_origin = fio_get_le16(f);
    int tga_image_width = fio_get_le16(f);
    int tga_image_height = fio_get_le16(f);
    int tga_pixel_depth = fio_get_byte(f);
    int tga_image_descriptor = fio_get_byte(f);

    /* check for supported color depth */
    int pix_fmt;
    switch(tga_pixel_depth)
    {
        case 32: pix_fmt = VZIMAGE_PIXFMT_BGRA; break;
        case 24: pix_fmt = VZIMAGE_PIXFMT_BGR;  break;
        case  8: pix_fmt = VZIMAGE_PIXFMT_GRAY; break;
        default:
        {
            fclose(f);
            return -3;
        };
    };

    /* check if image id is not outside file */
    if(ftell(f) + tga_id_length > tga_file_size)
    {
        fclose(f);
        return -2;
    };

    /* skip image id */
    fio_skip(f, tga_id_length);

    /* check if image data is not outside file */
    if((ftell(f) + tga_image_width * tga_image_height * (tga_pixel_depth >> 3) > tga_file_size)
        && 2 == tga_image_type && 3 == tga_image_type)
    {
        fclose(f);
        return -2;
    };

    /* create image */
    vzImage* img;
    int r = vzImageCreate(&img, tga_image_width, tga_image_height, pix_fmt);
    if(r)
    {
        fclose(f);
        return r;
    };

    /* load data */
    if(2 == tga_image_type || 3 == tga_image_type)
    {
        for(int j, i = 0; i < tga_image_height; i++)
        {
            /* bit 5 is for top-t-obottom ordering: bottom(0) top(1) */
            if(tga_image_descriptor & (1 << 5))
                j = i;
            else
                j = tga_image_height - 1 - i;

            fio_get_buffer(f, img->lines_ptr[j], tga_image_width * img->bpp);
        };
    }
    else if(10 == tga_image_type || 11 == tga_image_type)
    {
        int r_cnt;
        int j, jc, i = 0;
        int linesize = tga_image_width * img->bpp;
        unsigned char *buf_dst;

        /* bit 5 is for top-t-obottom ordering: bottom(0) top(1) */
        if(tga_image_descriptor & (1 << 5))
        {
            j = 0;
            jc = 1;
        }
        else
        {
            j = tga_image_height - 1;
            jc = -1;
        };

        while(!feof(f))
        {
            /* get Repetition Count field */
            r_cnt = fio_get_byte(f);

            buf_dst = (unsigned char*)img->lines_ptr[j] + i;

            if(0x80 & r_cnt)
            {
                /* packet is a Run-length Packet */
                fio_get_buffer(f, buf_dst, img->bpp);
                for(r_cnt = r_cnt & 0x7F, i += img->bpp; r_cnt > 0; r_cnt--, i += img->bpp)
                    memcpy((unsigned char*)img->lines_ptr[j] + i, buf_dst, img->bpp);
            }
            else
            {
                /* packet is a Raw Packet */
                r_cnt &= 0x7F; r_cnt += 1; r_cnt *= img->bpp;
                fio_get_buffer(f, buf_dst, r_cnt);
                i += r_cnt;
            };

            /* check to switch to new scanline */
            if(i >= linesize)
            {
                i = 0;
                j += jc;

                /* check for pic end */
                if(j < 0 || j >= tga_image_height)
                    break;
            };
        };
    };

    /* close file */
    fclose(f);

    *pimg = img;

    return 0;
};

#endif /* vzImageLoadTGA_h */

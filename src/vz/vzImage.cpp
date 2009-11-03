/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2005 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2005.

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
	2005-06-08: Code cleanup

*/
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vzImage.h"
#include "vzGlExt.h"

typedef struct {
   char  idlength;
   char  colourmaptype;
   char  datatypecode;
   short int colourmaporigin;
   short int colourmaplength;
   char  colourmapdepth;
   short int x_origin;
   short int y_origin;
   short width;
   short height;
   char  bitsperpixel;
   char  imagedescriptor;
} TGA_HEADER;

//#define ERROR_LOG(MSG,LOG) printf("\n" __FILE__ ":%d: " MSG " '%s'\n",__LINE__,LOG);

static void vzImageDeinterleave(vzImage* image, int factor)
{
/*
	0 - 00 = non-interleaved.
	1 - 01 = two-way (even/odd) interleaving.
	2 - 10 = four way interleaving.
	3 - 11 = reserved.
*/
	if
	(
		(1 == factor)
		||
		(2 == factor)
	)
	{
		unsigned char* new_surface = (unsigned char*)malloc(4*image->height*image->width);
		unsigned char* old_surface = (unsigned char*)image->surface;
		long i,j;

		if(1 == factor)
		{
			/* two-way (even/odd) interleaving */
			for(i = 0; i<image->height; i++)
			{
				/* calc index */
				j = (i>>1) + (i&1)?(image->height>>1):0;

				/* copy */
				memcpy
				(
					new_surface + 4*i*image->width,
					old_surface + 4*j*image->width,
					4*image->width
				);
			};
		}
		else
		{
			/* four way interleaving. */
		};

		/* swap surfaces */
		free(old_surface);
		image->surface = new_surface;
	};
};

VZIMAGE_API int vzImageFlipVertical(vzImage* img)
{
	int i, j, h;
    void* buf;

    if(!img) return -1;

    /* allocate buffer for lines swap */
    buf = malloc(img->line_size);
    if(!buf) return -2;

    for(i = 0, h = img->height / 2; i < h; i++)
    {
        j = img->height - 1 - i;

        memcpy(buf, img->lines_ptr[i], img->line_size);
        memcpy(img->lines_ptr[i], img->lines_ptr[j], img->line_size);
        memcpy(img->lines_ptr[j], buf, img->line_size);
    };

    /* change base */
	img->base_y = img->height - img->base_y - img->base_height;

    /* release line buffer */
    free(buf);

    return 0;
};

VZIMAGE_API vzImage* vzImageNewFromVB(long width, long height)
{
    int r;
    vzImage* temp_image;

	// create buffer
    r = vzImageCreate(&temp_image, width, height, VZIMAGE_PIXFMT_BGRA);

    if(r) return NULL;

	//read gl pixels to buffer - ready to use
	glReadPixels(0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, temp_image->surface);

	return temp_image;
};

VZIMAGE_API int vzImageSaveTGA(char* filename, vzImage* vzimage, char** error_log, int flipped)
{
	TGA_HEADER header = 
	{
		0,				//	char  idlength;
		0,				//	char  colourmaptype;
		2,				//	char  datatypecode;
		0,				//	short int colourmaporigin;
		0,				//	short int colourmaplength;
		0,				//	char  colourmapdepth;
		0,				//	short int x_origin;
		0,				//	short int y_origin;
		(short)vzimage->width,	//	short width;
		(short)vzimage->height,	//	short height;
		32,				//	char  bitsperpixel;
		(char)((flipped)?0:(1<<5)) //char  imagedescriptor;
	};

#define ERR2(MSG) if(error_log) *error_log = __FILE__ "::vzImageSaveTGA: " MSG;return NULL;

	// open file
	FILE* image = fopen(filename,"wb");

	// if unable to -> FAIL
	if (!image)
	{
		ERR2("Unable to open file")
	};

#define TGA_HEADER_WRITER(FIELD,NAME) \
	if (1 != fwrite(&FIELD,sizeof(FIELD),1,image)) \
	{ \
		fclose(image); \
		ERR2("Error writing field '" NAME "'") \
		return NULL; \
	}; 

	// writing header

	// ID Length - Field 1 (1 byte):
	// This field identifies the number of bytes contained in Field 6, the Image ID Field. The maximum number of characters is 255. A value of zero indicates that no Image ID field is included with the image.
	TGA_HEADER_WRITER(header.idlength,"idlength")

	// Color Map Type - Field 2 (1 byte):
	// This field indicates the type of color map (if any) included with the image. There are currently 2 defined values for this field:
	// 0 - indicates that no color-map data is included with this image.
	// 1 - indicates that a color-map is included with this image.
	TGA_HEADER_WRITER(header.colourmaptype,"colourmaptype")

	// Image Type - Field 3 (1 byte):
	// The TGA File Format can be used to store Pseudo-Color, True-Color and Direct-Color images of various pixel depths. Truevision has currently defined seven image types:
	TGA_HEADER_WRITER(header.datatypecode,"datatypecode")

	// Color Map Specification - Field 4 (5 bytes):
	TGA_HEADER_WRITER(header.colourmaporigin,"colourmaporigin")
	TGA_HEADER_WRITER(header.colourmaplength,"colourmaplength")
	TGA_HEADER_WRITER(header.colourmapdepth,"colourmapdepth")

	// Image Specification - Field 5 (10 bytes):
	// Field 5.1 (2 bytes) - X-origin of Image:
	// These bytes specify the absolute horizontal coordinate for the lower left corner of the image as it is positioned on a display device having an origin at the lower left of the screen (e.g., the TARGA series). 
	TGA_HEADER_WRITER(header.x_origin,"x_origin")
	// Field 5.2 (2 bytes) - Y-origin of Image:
	// These bytes specify the absolute vertical coordinate for the lower left corner of the image as it is positioned on a display device having an origin at the lower left of the screen (e.g., the TARGA series). 
	TGA_HEADER_WRITER(header.y_origin,"y_origin")

	// Field 5.3 (2 bytes) - Image Width:
	// This field specifies the width of the image in pixels.
	TGA_HEADER_WRITER(header.width,"width")

	// Field 5.4 (2 bytes) - Image Height:
	// This field specifies the height of the image in pixels.
	TGA_HEADER_WRITER(header.height,"height")

	// Field 5.5 (1 byte) - Pixel Depth:
	// This field indicates the number of bits per pixel. This number includes the Attribute or Alpha channel bits. Common values are 8, 16, 24 and 32 but other pixel depths could be used.
	TGA_HEADER_WRITER(header.bitsperpixel,"bitsperpixel")

	// Field 5.6 (1 byte) - Image Descriptor:
/*
|        |        |  Bits 3-0 - number of attribute bits associated with each  |
|        |        |             pixel.                                         |
|        |        |  Bit 4    - reserved.  Must be set to 0.                   |
|        |        |  Bit 5    - screen origin bit.                             |
|        |        |             0 = Origin in lower left-hand corner.          |
|        |        |             1 = Origin in upper left-hand corner.          |
|        |        |             Must be 0 for Truevision images.               |
|        |        |  Bits 7-6 - Data storage interleaving flag.                |
|        |        |             00 = non-interleaved.                          |
|        |        |             01 = two-way (even/odd) interleaving.          |
|        |        |             10 = four way interleaving.                    |
|        |        |             11 = reserved.                                 |
*/
	TGA_HEADER_WRITER(header.imagedescriptor,"imagedescriptor")

	// storing image
	if
		(
			1
			!= 
			fwrite
			(
				vzimage->surface,
				(header.bitsperpixel >> 3) * header.width * header.height,
				1,
				image
			)
		)
	{
		fclose(image);
		ERR2("Unable to write image data");
	};

	// close file handle
	fclose(image);

	return 1;
};

static int vzImagePixFmt2Bpp(int pix_fmt)
{
    int bpp = 0;

    switch(pix_fmt)
    {
        case VZIMAGE_PIXFMT_BGR:
        case VZIMAGE_PIXFMT_RGB:
            bpp = 3;
            break;;
        case VZIMAGE_PIXFMT_BGRA:
        case VZIMAGE_PIXFMT_BGRX:
        case VZIMAGE_PIXFMT_XBGR:
        case VZIMAGE_PIXFMT_RGBA:
        case VZIMAGE_PIXFMT_RGBX:
            bpp = 4;
            break;
        case VZIMAGE_PIXFMT_GRAY:
            bpp = 1;
            break;
    };

    return bpp;
};

static long long img_sys_id_cnt = 1;
VZIMAGE_API int vzImageCreate(vzImage** pimg, int width, int height, long pix_fmt)
{
    vzImage* img;

    if(!pimg) return -2;

    /* reset pointer */
    *pimg = NULL;

    /* allocate space for image */
    img = (vzImage*)malloc(sizeof(struct vzImageDesc));
    if(!img) return -1;
    memset(img, 0, sizeof(struct vzImageDesc));

    /* setup fields */
    img->sys_id = ++img_sys_id_cnt;
    img->width = img->base_width = width;
    img->height = img->base_height = height;
    img->pix_fmt = pix_fmt;
    img->bpp = vzImagePixFmt2Bpp(pix_fmt);

    /* allocate */
    img->line_size = ((width * img->bpp + (VZIMAGE_ALIGN_LINE >> 1)) /
        VZIMAGE_ALIGN_LINE) * VZIMAGE_ALIGN_LINE;
    img->surface = malloc(img->line_size * img->height);
    if(!img->surface)
    {
        free(img);
        return -1;
    };
    memset(img->surface, 0, img->line_size * img->height);

    /* create a lines_ptr */
    img->lines_ptr = (void**)malloc(img->height * sizeof(void*));
    if(!img->lines_ptr)
    {
        free(img->surface);
        free(img);
        return -1;
    };
    for(int i = 0; i < img->height; i++)
        img->lines_ptr[i] = (unsigned char*)img->surface + i * img->line_size;

    /* setup pointer */
    *pimg = img;

    return 0;
};

VZIMAGE_API int vzImageRelease(vzImage** pimg)
{
    vzImage* img;

    if(!pimg) return -2;

    img = *pimg;
    *pimg = NULL;

    if(!img) return -1;

    if(img->surface) free(img->surface);
    if(img->lines_ptr) free(img->lines_ptr);
    free(img);

    return 0;
};

#include "vzImageLoad.h"

VZIMAGE_API int vzImageLoad(vzImage** pimg, char* filename, long pix_fmt)
{
    int k, l, i, r;
    static const struct
    {
        char ext[8];
        int (*loader)(vzImage** pimg, char* filename);
    } exts_map[] = 
    {
        {".tga",    vzImageLoadTGA},
        {".png",    vzImageLoadPNG},
        {".jpg",    vzImageLoadJPEG},
        {".jpeg",   vzImageLoadJPEG},
        {".bmp",    vzImageLoadBMP},
        {".dib",    vzImageLoadBMP},
        {"", NULL}
    };

    /* try to detect extension */
    l = (int)strlen(filename);
    for(i = 0; exts_map[i].loader ; i++)
    {
        k = (int)strlen(exts_map[i].ext);

        if((l > k) && (0 == _stricmp(filename + l - k, exts_map[i].ext)))
        {
            /* try to load */
            r = exts_map[i].loader(pimg, filename);

            /* check error code */
            if(r) return r;

            /* require convertion ? */
            if(pix_fmt == (*pimg)->pix_fmt)
                return 0;

            /* convert */
            r = vzImagePixFmtConv(pimg, pix_fmt);

            /* free image if not success convertation */
            if(r) vzImageRelease(pimg);

            /* return result */
            return r;
        };
    };

    /* not supported */
    return -1;
};

#if defined(WIN32) && !defined(GL_BGR)
#define GL_BGR 0x80E0
#endif

VZIMAGE_API int vzImagePixFmt2OGL(int pix_fmt)
{
    int type = 0;

    switch(pix_fmt)
    {
        case VZIMAGE_PIXFMT_BGR:    type = GL_BGR;          break;
        case VZIMAGE_PIXFMT_RGB:    type = GL_RGB;          break;
        case VZIMAGE_PIXFMT_BGRA:   type = GL_BGRA_EXT;     break;
        case VZIMAGE_PIXFMT_RGBA:   type = GL_RGBA;         break;
        case VZIMAGE_PIXFMT_GRAY:   type = GL_LUMINANCE;    break;
    };

    return type;
};

#include "vzImagePixFmtConv.h"

VZIMAGE_API int vzImagePixFmtConv(vzImage** pimg, int pix_fmt)
{
    int i, j, r;
    int inplace = 0;
    vzImage *src_img, *dst_img;
    static const struct
    {
        int src_pix_fmt;
        int dst_pix_fmt;
        void (*conv)(unsigned char* src, unsigned char* dst, int count);
    } pix_fmt_convs[] =
    {
        /* VZIMAGE_PIXFMT_BGR */
        {VZIMAGE_PIXFMT_BGR,    VZIMAGE_PIXFMT_BGRA,    line_conv_BGR_to_BGRA},
        {VZIMAGE_PIXFMT_BGR,    VZIMAGE_PIXFMT_RGB,     line_conv_BGR_to_RGB},
        {VZIMAGE_PIXFMT_BGR,    VZIMAGE_PIXFMT_GRAY,    0},
        {VZIMAGE_PIXFMT_BGR,    VZIMAGE_PIXFMT_RGBA,    line_conv_BGR_to_RGBA},

        /* VZIMAGE_PIXFMT_BGRA */
        {VZIMAGE_PIXFMT_BGRA,   VZIMAGE_PIXFMT_BGR,     line_conv_BGRA_to_BGR},
        {VZIMAGE_PIXFMT_BGRA,   VZIMAGE_PIXFMT_RGB,     line_conv_BGRA_to_RGB},
        {VZIMAGE_PIXFMT_BGRA,   VZIMAGE_PIXFMT_GRAY,    0},
        {VZIMAGE_PIXFMT_BGRA,   VZIMAGE_PIXFMT_RGBA,    line_conv_BGRA_to_RGBA},

        /* VZIMAGE_PIXFMT_RGB */
        {VZIMAGE_PIXFMT_RGB,    VZIMAGE_PIXFMT_BGR,     line_conv_BGR_to_RGB},
        {VZIMAGE_PIXFMT_RGB,    VZIMAGE_PIXFMT_BGRA,    line_conv_BGR_to_RGBA},
        {VZIMAGE_PIXFMT_RGB,    VZIMAGE_PIXFMT_GRAY,    0},
        {VZIMAGE_PIXFMT_RGB,    VZIMAGE_PIXFMT_RGBA,    line_conv_BGR_to_BGRA},

        /* VZIMAGE_PIXFMT_GRAY */
        {VZIMAGE_PIXFMT_GRAY,   VZIMAGE_PIXFMT_BGR,     line_conv_GRAY_to_RGB},
        {VZIMAGE_PIXFMT_GRAY,   VZIMAGE_PIXFMT_BGRA,    line_conv_GRAY_to_RGBA},
        {VZIMAGE_PIXFMT_GRAY,   VZIMAGE_PIXFMT_RGB,     line_conv_GRAY_to_RGB},
        {VZIMAGE_PIXFMT_GRAY,   VZIMAGE_PIXFMT_RGBA,    line_conv_GRAY_to_RGBA},

        /* VZIMAGE_PIXFMT_RGBA */
        {VZIMAGE_PIXFMT_RGBA,   VZIMAGE_PIXFMT_BGR,     line_conv_BGRA_to_RGB},
        {VZIMAGE_PIXFMT_RGBA,   VZIMAGE_PIXFMT_BGRA,    line_conv_BGRA_to_RGBA},
        {VZIMAGE_PIXFMT_RGBA,   VZIMAGE_PIXFMT_RGB,     line_conv_BGRA_to_BGR},
        {VZIMAGE_PIXFMT_RGBA,   VZIMAGE_PIXFMT_GRAY,    0},

        /* VZIMAGE_PIXFMT_RGBX */
        {VZIMAGE_PIXFMT_RGBX,   VZIMAGE_PIXFMT_BGRA,    line_conv_RGBX_to_BGRA},
        {VZIMAGE_PIXFMT_BGRX,   VZIMAGE_PIXFMT_BGRA,    line_conv_BGRX_to_BGRA},
        {VZIMAGE_PIXFMT_XBGR,   VZIMAGE_PIXFMT_BGRA,    line_conv_XBGR_to_BGRA},

        {0, 0, NULL}
    };

    if(!pimg)
        return -2;

    src_img = *pimg;

    if(!src_img)
        return -2;

    /* lookup for proper pixel convertor */
    for(i = 0; pix_fmt_convs[i].src_pix_fmt; i++)
        if
        (
            (pix_fmt_convs[i].src_pix_fmt == src_img->pix_fmt)
            &&
            (pix_fmt_convs[i].dst_pix_fmt == pix_fmt)
        )
        {
            break;
        };

    /* check if possible to convert */
    if(!pix_fmt_convs[i].conv)
        return -3;

    if(vzImagePixFmt2Bpp(pix_fmt) == vzImagePixFmt2Bpp(src_img->pix_fmt))
        inplace = 1;

    /* allocate image if required */
    if(inplace)
    {
        dst_img = src_img;
        dst_img->pix_fmt = pix_fmt;
    }
    else
    {
        /* allocate new image */
        r = vzImageCreate(&dst_img, src_img->width, src_img->height, pix_fmt);

        /* check */
        if(r)
            return -3;
    };

    /* convert lines */
    for(j = 0; j < src_img->height; j++)
        pix_fmt_convs[i].conv
        (
            (unsigned char*)src_img->lines_ptr[j],
            (unsigned char*)dst_img->lines_ptr[j],
            src_img->width
        );

    /* free old image in inplace not supported */
    if(!inplace)
    {
        *pimg = dst_img;
        vzImageRelease(&src_img);
    };

    return 0;
};

inline int POT(int v)
{
	int i;
	for(i = 1; (i != 0x80000000) && (i < v); i <<= 1);
	return i;
};

VZIMAGE_API int vzImageExpandPOT(vzImage** pimg)
{
    int r, i, x, y;
    vzImage *src_img, *dst_img;

    if(!pimg) return -1;

    src_img = (*pimg);

    if(!src_img) return -1;

    r = vzImageCreate(&dst_img, POT(src_img->width), POT(src_img->height), src_img->pix_fmt);

    if(r) return -2;

    /* find offsets */
    x = (dst_img->width - src_img->width) / 2;
    y = (dst_img->height - src_img->height) / 2;

    /* copy block */
    for(i = 0; i < src_img->height; i++)
        memcpy
        (
            (unsigned char*)dst_img->lines_ptr[i + y] + x * dst_img->bpp,
            src_img->lines_ptr[i],
            src_img->line_size
        );

    /* set base info */
    dst_img->base_width = src_img->base_width;
    dst_img->base_height = src_img->base_height;
    dst_img->base_x = src_img->base_x + x;
    dst_img->base_y = src_img->base_y + y;

    /* set new image */
    *pimg = dst_img;

    /* free old image */
    vzImageRelease(&src_img);

    return 0;
};

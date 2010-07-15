#ifndef vzImageLoadPNG_h
#define vzImageLoadPNG_h

#include <png.h>

#ifdef _DEBUG
	#pragma comment(lib, "libpngd.lib")
	#pragma comment(lib, "zlibd.lib")
#else
	#pragma comment(lib, "libpng.lib")
	#pragma comment(lib, "zlib.lib")
#endif

static int vzImageLoadPNG(vzImage** pimg, char* filename)
{
    FILE *fp;
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 w, h;
    int bd, ct;
    int pix_fmt;

    if(!pimg) return -1;

    /* openfile file */
    fp = fopen(filename, "rb");
    if (!fp) return -1;

    /* Allocate the png read struct */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) return -1;

    /* Allocate the png info struct */
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return -1;
    };

    /* for proper error handling */
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return -1;
    };

    png_init_io(png_ptr, fp);

    /* Read the info section of the png file */
    png_read_info(png_ptr, info_ptr);

    /* Extract info */
    png_get_IHDR(png_ptr, info_ptr, &w, &h, &bd, &ct, NULL, NULL, NULL);

    /* Convert palette color to true color */
    if (PNG_COLOR_TYPE_PALETTE == ct)
        png_set_palette_to_rgb(png_ptr);

    /* Convert low bit colors to 8 bit colors */
    if (bd < 8)
    {
        if (PNG_COLOR_TYPE_GRAY == ct || PNG_COLOR_TYPE_GRAY_ALPHA == ct)
            png_set_gray_1_2_4_to_8(png_ptr);
        else
            png_set_packing(png_ptr);
    }

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    /* Convert high bit colors to 8 bit colors */
    if (16 == bd)
        png_set_strip_16(png_ptr);

    /* Convert gray color to true color */
    if (PNG_COLOR_TYPE_GRAY == ct || PNG_COLOR_TYPE_GRAY_ALPHA == ct)
        png_set_gray_to_rgb(png_ptr);

    /* Update the changes */
    png_read_update_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &w, &h, &bd, &ct, NULL, NULL, NULL);

    /* init image */
    switch(ct)
    {
        case PNG_COLOR_TYPE_GRAY:       pix_fmt = VZIMAGE_PIXFMT_GRAY;   break;
        case PNG_COLOR_TYPE_RGB:        pix_fmt = VZIMAGE_PIXFMT_RGB;    break;
        case PNG_COLOR_TYPE_RGB_ALPHA:  pix_fmt = VZIMAGE_PIXFMT_RGBA;   break;
//        case PNG_COLOR_TYPE_GRAY_ALPHA:
        default:
            png_destroy_read_struct(&png_ptr, NULL, NULL);
            fclose(fp);
            return -2;
            break;
    };

    vzImage* img;
    int r = vzImageCreate(&img, w, h, pix_fmt);
    if(r)
    {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return -3;
    };

    /* Read data */
    png_read_image(png_ptr, (unsigned char**)img->lines_ptr);

    /* Clean up memory */
    png_read_end(png_ptr, NULL);
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
    fclose(fp);

    *pimg = img;

    return 0;
};

#endif /* vzImageLoadPNG_h */

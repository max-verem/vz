#define LIBPNG
#define LIBJPEG

#define fio_get_buffer(f, buf, len) fread(buf, len, 1, f)
#define fio_skip(f, bytes) fseek(f, bytes, SEEK_CUR)
#define fio_seek(f, bytes) fseek(f, bytes, SEEK_SET)
#define fio_get_be16(f) (fio_get_byte(f) << 8 | fio_get_byte(f))
#define fio_get_be32(f) (fio_get_byte(f) << 24 | fio_get_byte(f) << 16 | fio_get_byte(f) << 8 | fio_get_byte(f))
#define fio_get_le16(f) (fio_get_byte(f) << 0 | fio_get_byte(f) << 8)
#define fio_get_le32(f) (fio_get_byte(f) << 0 | fio_get_byte(f) << 8 | fio_get_byte(f) << 16 | fio_get_byte(f) << 24)

inline unsigned int fio_get_byte(FILE* f)
{
    unsigned int b = 0;
    fio_get_buffer(f, &b, 1);
    return b;
};

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

    /* we supports only "Uncompressed, True-color Image" 
        or "Uncompressed, Black-and-white Image" */
    if((2 != tga_image_type) && (3 != tga_image_type))
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
    if(ftell(f) + tga_image_width * tga_image_height * (tga_pixel_depth >> 3) > tga_file_size)
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
    for(int j, i = 0; i < tga_image_height; i++)
    {
        /* bit 5 is for top-t-obottom ordering: bottom(0) top(1) */
        if(tga_image_descriptor & (1 << 5))
            j = i;
        else
            j = tga_image_height - 1 - i;

        fio_get_buffer(f, img->lines_ptr[j], tga_image_width * img->bpp);
    };

    /* close file */
    fclose(f);

    *pimg = img;

    return 0;
};

#ifndef LIBJPEG

static int vzImageLoadJPEG(vzImage** pimg, char* filename)
{
    return -1;
};

#else /* LIBJPEG */

#include <jpeglib.h>
#include <jerror.h>

/*
    1. Read http://windows-tech.info/17/d4a6cd925bafac04.php
    2. download jpegsr7.zip
    3. Rename jconfig.vc to jconfig.h
    4. nmake /f makefile.vc
*/

#pragma comment(lib, "jpeg.lib")
//#pragma comment(lib, "libjpeg.lib")
//#pragma comment(lib, "jpeg-bcc.lib")

static int vzImageLoadJPEG(vzImage** pimg, char* filename)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE * infile;
    int i;

    if(!pimg) return -1;

    /* openfile file */
    infile = fopen(filename, "rb");
    if (!infile) return -1;

    /* Step 1: allocate and initialize JPEG decompression object */

    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.err = jpeg_std_error(&jerr);

    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(&cinfo);

    /* Step 2: specify data source (eg, a file) */
    jpeg_stdio_src(&cinfo, infile);

    /* Step 3: read file parameters with jpeg_read_header() */
    (void) jpeg_read_header(&cinfo, TRUE);
    /* We can ignore the return value from jpeg_read_header since
     *   (a) suspension is not possible with the stdio data source, and
     *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
     * See libjpeg.doc for more info.
     */

    /* Step 4: set parameters for decompression */
    /* In this example, we don't need to change any of the defaults set by
     * jpeg_read_header(), so we do nothing here.
     */

    /* Step 5: Start decompressor */
    (void) jpeg_start_decompress(&cinfo);

    /* create image */
    vzImage* img;
    int pix_fmt;
    switch(cinfo.output_components)
    {
        case 3: pix_fmt = VZIMAGE_PIXFMT_RGB; break;
        case 1: pix_fmt = VZIMAGE_PIXFMT_GRAY; break;
        case 4: pix_fmt = VZIMAGE_PIXFMT_RGBA; break;
        default:
        {
            jpeg_destroy_decompress(&cinfo);
            fclose(infile);
            return -10;
        }
    };   
    i = vzImageCreate(&img, cinfo.output_width, cinfo.output_height, pix_fmt);
    if(i)
    {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return i;
    };

    /* Make a one-row-high sample array that will go away when done with image */
    (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, img->line_size, img->height);

    /* Step 6: while (scan lines remain to be read) */
    for(i = 0; i < img->height; i++)
        (void) jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&img->lines_ptr[i], 1);

    /* Step 7: Finish decompression */
    (void) jpeg_finish_decompress(&cinfo);

    /* Step 8: Release JPEG decompression object */
    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_decompress(&cinfo);

    fclose(infile);

    *pimg = img;

    return 0;
};

#endif /* LIBJPEG */

#ifndef LIBPNG

static int vzImageLoadPNG(vzImage** pimg, char* filename)
{
    return -1;
};

#else /* LIBPNG */

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

#endif /* LIBPNG */


#ifndef vzImageLoadJPEG_h
#define vzImageLoadJPEG_h

#include <jpeglib.h>
#include <jerror.h>
#include <setjmp.h>

/*
    1. Read http://windows-tech.info/17/d4a6cd925bafac04.php
    2. download jpegsr7.zip
    3. Rename jconfig.vc to jconfig.h
    4. nmake /f makefile.vc

or

    https://subversion.multipole.org/subversion/jpeg/trunk/install.txt
    http://www.dpvreony.co.uk/blog/post/63

    copy jconfig.vc /B  jconfig.h /B 
    copy makejsln.vc9 /B jpeg.sln /B 
    copy makeasln.vc9 /B apps.sln /B 
    copy makejvcp.vc9 /B jpeg.vcproj /B 
    copy makecvcp.vc9 /B cjpeg.vcproj /B 
    copy makedvcp.vc9 /B djpeg.vcproj /B 
    copy maketvcp.vc9 /B jpegtran.vcproj /B 
    copy makervcp.vc9 /B rdjpgcom.vcproj /B 
    copy makewvcp.vc9 /B wrjpgcom.vcproj /B 

*/

#pragma comment(lib, "jpeg.lib")

struct jpeg_error_mgr_vz
{
    struct jpeg_error_mgr pub;      /* "public" fields */
    jmp_buf setjmp_buffer;          /* for return to caller */
};

METHODDEF(void) vz_error_exit (j_common_ptr cinfo)
{
    /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
    struct jpeg_error_mgr_vz* err = (struct jpeg_error_mgr_vz*) cinfo->err;

    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    (*cinfo->err->output_message) (cinfo);

    /* Return control to the setjmp point */
    longjmp(err->setjmp_buffer, 1);
}

static int vzImageLoadJPEG(vzImage** pimg, char* filename)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr_vz jerr;
    FILE * infile;
    int i;

    if(!pimg) return -1;

    /* openfile file */
    infile = fopen(filename, "rb");
    if (!infile) return -1;

    /* Step 1: allocate and initialize JPEG decompression object */

    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = vz_error_exit;
    /* Establish the setjmp return context for my_error_exit to use. */
    if (setjmp(jerr.setjmp_buffer))
    {
        /* If we get here, the JPEG code has signaled an error.
        * We need to clean up the JPEG object, close the input file, and return.
        */
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return -1;
    }

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

#endif /* vzImageLoadJPEG_h */

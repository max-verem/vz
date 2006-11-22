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


#include "vzImage.h"

#include <GL/glut.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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


VZIMAGE_API void vzImageFree(vzImage* image)
{
///printf("\n**vzImageFree: %dx%d @ %.8X\n",image->width,image->height,image->surface);
	free(image->surface);
	free(image);
};

VZIMAGE_API vzImage* vzImageNew(int width,int height, long surface_size)
{
	vzImage* temp = (vzImage*) malloc(sizeof(vzImage));
	memset(temp, 0, sizeof(vzImage));
	temp->width = width;
	temp->height = height;
	temp->surface = malloc(surface_size);
	temp->base_width = width;
	temp->base_height = height;
///printf("\n**vzImageNew: %dx%d @ %.8X\n",temp->width,temp->height,temp->surface);
	memset(temp->surface, 0, surface_size);
	return temp;
};

VZIMAGE_API vzImage* vzImageLoadTGA(char* filename, char** error_log)
{
	TGA_HEADER header;

//	ERROR_LOG("vzImageLoadTGA","started");

#define ERR(MSG) if(error_log) *error_log = __FILE__ "::vzImageLoadTGA: " MSG;return NULL;

//	ERROR_LOG("vzImageLoadTGA loading",filename);

	// open file
	FILE* image = fopen(filename,"rb");

	// if unable to -> FAIL
	if (!image)
	{
		ERR("Unable to open file")
	};

//	ERROR_LOG("vzImageLoadTGA loaded",filename);

#define TGA_HEADER_READER_1(FIELD,NAME) \
	if (1 != fread(&FIELD,sizeof(FIELD),1,image)) \
	{ \
		fclose(image); \
		ERR("Error reading field '" NAME "'") \
		return NULL; \
	}; 

	// reading header

	// ID Length - Field 1 (1 byte):
	// This field identifies the number of bytes contained in Field 6, the Image ID Field. The maximum number of characters is 255. A value of zero indicates that no Image ID field is included with the image.
	TGA_HEADER_READER_1(header.idlength,"idlength")

	// Color Map Type - Field 2 (1 byte):
	// This field indicates the type of color map (if any) included with the image. There are currently 2 defined values for this field:
	// 0 - indicates that no color-map data is included with this image.
	// 1 - indicates that a color-map is included with this image.
	TGA_HEADER_READER_1(header.colourmaptype,"colourmaptype")
	if(header.colourmaptype)
	{
		fclose(image);
		ERR("Unsupported format: No colourmap allowed");
	};

	// Image Type - Field 3 (1 byte):
	// The TGA File Format can be used to store Pseudo-Color, True-Color and Direct-Color images of various pixel depths. Truevision has currently defined seven image types:
	TGA_HEADER_READER_1(header.datatypecode,"datatypecode")
	if(header.datatypecode != 2) 
	{
		fclose(image);
		ERR("Unsupported format: Unsupported datatypecode");
	};

	// Color Map Specification - Field 4 (5 bytes):
	TGA_HEADER_READER_1(header.colourmaporigin,"colourmaporigin")
	TGA_HEADER_READER_1(header.colourmaplength,"colourmaplength")
	TGA_HEADER_READER_1(header.colourmapdepth,"colourmapdepth")

	// Image Specification - Field 5 (10 bytes):
	// Field 5.1 (2 bytes) - X-origin of Image:
	// These bytes specify the absolute horizontal coordinate for the lower left corner of the image as it is positioned on a display device having an origin at the lower left of the screen (e.g., the TARGA series). 
	TGA_HEADER_READER_1(header.x_origin,"x_origin")
	// Field 5.2 (2 bytes) - Y-origin of Image:
	// These bytes specify the absolute vertical coordinate for the lower left corner of the image as it is positioned on a display device having an origin at the lower left of the screen (e.g., the TARGA series). 
	TGA_HEADER_READER_1(header.y_origin,"y_origin")

	// Field 5.3 (2 bytes) - Image Width:
	// This field specifies the width of the image in pixels.
	TGA_HEADER_READER_1(header.width,"width")

	// Field 5.4 (2 bytes) - Image Height:
	// This field specifies the height of the image in pixels.
	TGA_HEADER_READER_1(header.height,"height")

	// Field 5.5 (1 byte) - Pixel Depth:
	// This field indicates the number of bits per pixel. This number includes the Attribute or Alpha channel bits. Common values are 8, 16, 24 and 32 but other pixel depths could be used.
	TGA_HEADER_READER_1(header.bitsperpixel,"bitsperpixel")
	if(header.bitsperpixel != 32) 
	{
		fclose(image);
		ERR("Unsupported format: Support only 32-bits pixel");
	};

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
	TGA_HEADER_READER_1(header.imagedescriptor,"imagedescriptor")

	// skip to image body header
	fseek(image,header.idlength,SEEK_CUR);

	// set flip flag
//	bool flip = (header.imagedescriptor >> 4) & 1;

//	ERROR_LOG("vzImageLoadTGA header loaded",filename);

	vzImage* temp = vzImageNew(header.width, header.height, (header.bitsperpixel >> 3) * header.width * header.height);

	if
		(
			1
			!= 
			fread
			(
				temp->surface,
				(header.bitsperpixel >> 3) * header.width * header.height,
				1,
				image
			)
		)
	{
//		ERROR_LOG("vzImageLoadTGA error in reading image data of",filename);
		vzImageFree(temp);
		fclose(image);
		ERR("Unable to read image data");
	};

	// cour type
	temp->surface_type = GL_BGRA_EXT;

	// de-interleaving
	if( header.imagedescriptor & ((1<<7) | (1<<6)) )
		vzImageDeinterleave(temp, (header.imagedescriptor&((1<<7)|(1<<6)))>>6);

	// flipping 
	if(!(header.imagedescriptor & (1<<5)))
		vzImageFlipVertical(temp);

	// fix base offsets
	temp->base_x = 0;
	temp->base_y = 0;

	// close file handle
	fclose(image);

//	ERROR_LOG("vzImageLoadTGA returing",filename);	
///printf("\n**vzImageLoadTGA: %dx%d @ %.8X (%s)\n",temp->width,temp->height,temp->surface,filename);
	return temp;
};


VZIMAGE_API void vzImageFlipVertical(vzImage* image)
{
	int i;

	// check if pixel width is supported or default '0' value
	if
	(
		(image->surface_type == GL_BGRA_EXT) 
		|| 
		(image->surface_type == GL_RGBA) 
		|| 
		(image->surface_type == 0)
	)
	{
		// supported 32bit values of pixel
		unsigned long step = 4*image->width;
		unsigned char *src_surface = (unsigned char *)image->surface;
		unsigned char *dst = (unsigned char *)malloc(image->width*image->height*4);
		unsigned char *dst_surface = dst + step*(image->height - 1);
		
		/* move */
		for(i=0 ; i < image->height ; i++, dst_surface -= step, src_surface += step)
			memcpy(dst_surface, src_surface, step);

		/* free old surface */
		free(image->surface);

		/* append new */
		image->surface = dst;

		/* change base */
		image->base_y = image->height - image->base_y;
	};
};

char* get_glerror()
{
	int p;
	switch(p = glGetError())
	{
		case GL_NO_ERROR: return "GL_NO_ERROR";
		case GL_INVALID_ENUM: return "GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_INVALID_VALUE: return "GL_INVALID_VALUE: A numeric argument is out of range. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION: The specified operation is not allowed in the current state. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW: This function would cause a stack overflow. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW: This function would cause a stack underflow. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY: There is not enough memory left to execute the function. The state of OpenGL is undefined, except for the state of the error flags, after this error is recorded.";
		default:
			return "UNKNOWN";
	};
};


// global variable to skip check extensions supported
static long GL_EXT_bgra_supported = 0;
static long GL_EXT_bgra_checked = 0;
static char* GL_EXT_bgra_tag = "GL_EXT_bgra ";

VZIMAGE_API vzImage* vzImageNewFromVB(long width, long height)
{
	//looking for GL_EXT_bgra extention supported
	if (!(GL_EXT_bgra_checked))
	{
		// first check

		// request extensions
		char* extensions = (char*)glGetString(GL_EXTENSIONS);
		char* error_log = get_glerror();

		// do not continue in fail case
		if(!(extensions))
			return NULL;

		if (strstr(extensions, GL_EXT_bgra_tag))
			GL_EXT_bgra_supported = 1;

		GL_EXT_bgra_checked = 1;
	};


	// create buffer

	vzImage* temp_image = vzImageNew(width,height,width*height*4);

	if (GL_EXT_bgra_supported)
	{
		//read gl pixels to buffer - ready to use
		glReadPixels(0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, temp_image->surface);
	}
	else 
	{
		//read gl pixels to buffer - needs to be converted
		glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, temp_image->surface);

		//convert pixel format
		unsigned long * pPixel = (unsigned long *) temp_image->surface;
		int pixPairCount = height * width / 2;
		__asm
		{
				mov eax, dword ptr[pPixel]; //start from address pPixel
				mov ecx, 0; //counter
				mov ebx, dword ptr [pixPairCount];//pixels / 2
        loop_start:
				//load two pixels (process two in one cycle)
				movq MM0, [eax]; 
				movq MM1, MM0;
				pslld MM0, 8;
				psrld MM1, 24;
				por MM0, MM1;
				movq [eax], MM0;

				//next two pixels
				add eax,8;

				//loop counter
				inc ecx;
				cmp ecx, ebx
				jl loop_start;
				emms;
		}
	}

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
		vzimage->width,	//	short width;
		vzimage->height,	//	short height;
		32,				//	char  bitsperpixel;
		(flipped)?0:(1<<5) //char  imagedescriptor;
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

VZIMAGE_API void vzImageBGRA2YUAYVA(void* src, void* dst_yuv, void* dst_alpha, long count)
{
/*	short _Y[] = {29,150,76,0};
	short _U[] = {127,-85,-43,128};
	short _V[] = {-21,-106,127,128};
*/
// test !!!!
//	*((unsigned long*)src) = 0x01020304;
//	*(((unsigned long*)src) + 1) = 0x05060708;

	__asm
	{
//		EMMS
		jmp		begin
ALIGN 16
mask1:
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0xFF
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0xFF
_Y:
// = 
	__asm _emit 0x1D
	__asm _emit 0x00
	__asm _emit 0x96
	__asm _emit 0x00
	__asm _emit 0x4C
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0x00
_V:
// = 
	__asm _emit 0xEB 
	__asm _emit 0xFF 
	__asm _emit 0x96 
	__asm _emit 0xFF 
	__asm _emit 0x7F 
	__asm _emit 0x00 
	__asm _emit 0x80 
	__asm _emit 0x00
_U:
// = 
	__asm _emit 0x7F 
	__asm _emit 0x00 
	__asm _emit 0xAB 
	__asm _emit 0xFF 
	__asm _emit 0xD5 
	__asm _emit 0xFF 
	__asm _emit 0x80 
	__asm _emit 0x00

begin:
		// loading source surface pointer
		mov		esi, dword ptr[src];

		// loading dst surface pointer
		mov		edi, dword ptr[dst_yuv];

		// loading dst alpha surface pointer
		mov		ebx, dword ptr[dst_alpha];

		// counter
		mov		ecx, dword ptr[count];
		sar		ecx,1	// devide on 2


pixel:
		// prefetch to cache
//		PREFETCHNTA [esi];
//		PREFETCHNTA [edi];


		

		// load 2 pixels
		movq		MM0, qword ptr[esi]

		// extract alpha
		movq		MM3, MM0
		psrad		MM3, 24
		packsswb	MM3, MM0
		packsswb	MM3, MM0
		movd		eax,MM3
		mov			word ptr [ebx], ax
		add			ebx,2

		// unpacking
		por			MM0, qword ptr [mask1]	// hide alphas
		movq		MM1,MM0		//	move to register for PIXEL 1
		movq		MM2,MM0		//  move to register for PIXEL 2
		pxor		MM0,MM0		//	clear register
		punpcklbw	MM1,MM0		//  unpacking
		punpckhbw	MM2,MM0		//  unpacking

		// loading vectors
		movq		MM3,qword ptr [_Y]
		movq		MM5,qword ptr [_U]
		movq		MM4,MM3
		movq		MM6,qword ptr [_V]

		// multiply
		pmullw		MM3,MM1
		pmullw		MM4,MM2
		pmullw		MM5,MM1
		pmullw		MM6,MM2

// PIXEL #1 (MM0 <- MM3, MM7 <- MM5) -> eax
		movq		MM0,MM3 // duplicate (1)
		movq		MM7,MM5
		psllq		MM0,32	// shift duplicated (1)
		psllq		MM7,32
		paddw		MM3,MM0 // adding (1)
		paddw		MM5,MM7
		movq		MM0,MM3 // duplicate (2)
		movq		MM7,MM5
		psrlq		MM0,16	// shift duplicated (2)
		psrlq		MM7,16
		paddw		MM3,MM0 // adding (2)
		paddw		MM5,MM7
// pack
		psrlw		MM3,8	// shift duplicated (2)
		psrlw		MM5,8	
		PUNPCKHWD	MM5,MM3
		PACKUSWB	MM5,MM5
// save to AX (Y1U1 -> AX)
		movd		eax,MM5
		and			eax,0xFFFF

// PIXEL #2 (MM0 <- MM4, MM7 <- MM6) -> edx
		movq		MM0,MM4 // duplicate (1)
		movq		MM7,MM6
		psllq		MM0,32	// shift duplicated (1)
		psllq		MM7,32
		paddw		MM4,MM0 // adding (1)
		paddw		MM6,MM7
		movq		MM0,MM4 // duplicate (2)
		movq		MM7,MM6
		psrlq		MM0,16	// shift duplicated (2)
		psrlq		MM7,16
		paddw		MM4,MM0 // adding (2)
		paddw		MM6,MM7
// pack
		psrlw		MM4,8	// shift duplicated (2)
		psrlw		MM6,8	
		PUNPCKHWD	MM6,MM4
		PACKUSWB	MM6,MM6
// save to AX (Y1U1 -> DX)
		movd		edx,MM6

// save to memmory
		sal			edx,16
		or			edx,eax
		mov			dword ptr [edi],edx

// increment
		add			esi,8
		add			edi,4
		dec			ecx

		jnz			pixel
		emms
	}

	return;
};

// ARGB
#define BGRA_A(ARG) ((0xFF000000 & ARG)>>24)
#define BGRA_R(ARG) ((0x00FF0000 & ARG)>>16)
#define BGRA_G(ARG) ((0x0000FF00 & ARG)>>8)
#define BGRA_B(ARG) ((0x000000FF & ARG)>>0)


VZIMAGE_API void vzImageBGRA2YUAYVA_0(vzImage* image,void* yuv,void* alpha)
{
	unsigned char* buffer_alpha = (unsigned char*)alpha;
	unsigned short* buffer_yuv = (unsigned short*)yuv;

	unsigned long R,G,B,A,Y,U,V;
	for(int row=0,i=0;row<image->height;row++)
		for(int col=0;col<image->width;)
		{
			// pixel 1
			R = BGRA_R(((unsigned long*)image->surface)[i]);
			G = BGRA_G(((unsigned long*)image->surface)[i]);
			B = BGRA_B(((unsigned long*)image->surface)[i]);
			A = BGRA_A(((unsigned long*)image->surface)[i]);

			Y = ((long)(219l* 77*R + 219l*150*G + 219l* 29*B)>>16)+16;
			U = ((long)(219l*131*B - 219l* 87*G - 219l* 44*R)>>16)+128;

			*buffer_yuv++ = (unsigned short) ((Y & 0xFF) << 8 | (U & 0xFF));
			*buffer_alpha++ = (unsigned char)A;

			i++;col++;

			// pixel 2
			R = BGRA_R(((unsigned long*)image->surface)[i]);
			G = BGRA_G(((unsigned long*)image->surface)[i]);
			B = BGRA_B(((unsigned long*)image->surface)[i]);
			A = BGRA_A(((unsigned long*)image->surface)[i]);

			Y = ((long)(219l* 77*R + 219l*150*G + 219l* 29*B)>>16)+16;
			V = ((long)(219l*131*R - 219l*110*G - 219l* 21*B)>>16)+128;

			*buffer_yuv++ = (unsigned short) ((Y & 0xFF) << 8 | (V & 0xFF));
			*buffer_alpha++ = (unsigned char)A;

			i++;col++;
		};

};


VZIMAGE_API vzImage* vzImageExpand2X(vzImage* src)
{
	unsigned long* surface = (unsigned long*)src->surface;

	int new_width, new_height;
	
	// looking for new dims
	for(new_width = 1;new_width<src->width;new_width<<=1);
	for(new_height = 1;new_height<src->height;new_height<<=1);

	// create new surfase
	unsigned long* new_surface = (unsigned long*)malloc(4*new_height*new_width);

	// fill (clear)
	memset(new_surface,0,4*new_height*new_width);
	
	// offsets
	long ox = (new_width - src->width)>>1;
	long oy = (new_height - src->height)>>1;

	// copying
	long row=0;
	for
	(
		unsigned long 
			*p_src = surface, 
			*p_dst = new_surface + ox + new_width*oy 
		;
			row<src->height
		;
			p_src+=src->width,
			p_dst+=new_width,
			row++
	)
		memcpy(p_dst,p_src,4*src->width);

///printf("\n**vzImageExpand2X: [%dx%d @ %.8X] ->  [%dx%d @ %.8X]\n",
///src->width,src->height,src->surface,
///new_width, new_height, new_surface);

	// replacing parameters
	src->base_height = src->width;
	src->base_height = src->height;
	src->base_x += ox;
	src->base_y += oy;
	src->height = new_height;
	src->width = new_width;
	src->surface = new_surface;

	// free mem of old surface
	free(surface);

	// return value
	return src;
};
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


#include "vzTTFont.h"
#include "unicode.h"

#include <windows.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

FT_Library  ft_library;


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			FT_Init_FreeType( &ft_library );
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}


#define _MAX(A,B) ((A>B)?A:B)

VZTTFONT_API vzTTFont::vzTTFont(char* name,int height,int width)
{
	_ready = 0;
	lock = CreateMutex(NULL,FALSE,NULL);

	//filename
	sprintf(_file_name,"fonts/%s.ttf",name);

	// clear cache of glyphs
	for(int i=0;i<65536;i++) _glyphs_cache[i] = NULL;

	// init new face
	if(FT_New_Face(ft_library,_file_name,0,(FT_Face*)&_font_face))
		return;

	// setting font dims
	if(FT_Set_Pixel_Sizes((FT_Face)_font_face,_width = width,_height = height))
		return;

//	if(FT_Set_Char_Size(_font_face,(_width = width)*64,(_height = height)*64,72,72))
//		return;

	_baseline = (((FT_Face)_font_face)->bbox.yMax*_height) / (((FT_Face)_font_face)->bbox.yMax - ((FT_Face)_font_face)->bbox.yMin);

	_ready = 1;
};

VZTTFONT_API vzTTFont::~vzTTFont()
{
	CloseHandle(lock);

	for(int i=0;i<65536;i++)
		if (_glyphs_cache[i])
			FT_Done_Glyph((FT_Glyph)_glyphs_cache[i]);

	FT_Done_Face((FT_Face)_font_face);
};


void* vzTTFont::_get_glyph(unsigned short char_code)
{
	// lock cache
	WaitForSingleObject((HANDLE)lock,INFINITE);

	if (_glyphs_cache[char_code] == NULL)
	{
		// determinating glyph index
		FT_UInt glyph_index = FT_Get_Char_Index( (FT_Face)_font_face, char_code);

		// check if glyph defined?
		if(glyph_index == 0)
		{
			// error happen !! no such symbol

			// unlock
			ReleaseMutex((HANDLE)lock);

			return NULL;
		};


		// saving glyph index
		_glyphs_indexes[char_code] = glyph_index;

		// loading glyph info face
		FT_Load_Glyph( (FT_Face)_font_face, glyph_index, FT_LOAD_RENDER | FT_LOAD_NO_HINTING);
	
		// storing 
		FT_Glyph temp_glyph;
		FT_Get_Glyph( ((FT_Face)_font_face)->glyph, &temp_glyph);
		FT_BitmapGlyph bmp = (FT_BitmapGlyph)temp_glyph;
		_glyphs_cache[char_code] = temp_glyph;
	};
	
	// unlock
	ReleaseMutex((HANDLE)lock);

	// return glyph object
	return _glyphs_cache[char_code];
};

typedef struct
{
	FT_BitmapGlyph bmp;
	FT_Glyph glyph;
	unsigned short character;
	long x,y;
	long draw;
} vzTTFontSymbol;

VZTTFONT_API long vzTTFont::ready()
{
	return _ready;
};

VZTTFONT_API vzImage* vzTTFont::render(char* text, long colour, float line_space, long break_word, long limit_width,long limit_height)
{
	if(!(_ready))
		return NULL;

	// line space
	long __height = (long) (line_space*_height);


	// possibly incorrectly counting baseline cause problem
	// how base line could be less then top?
	// could be!!!
	int _baseline = _height;

	// convert to unicode string
	unsigned short int* string = utf8uni((unsigned char*)text);

	// determinate string length
	int length = 0; for(;string[length];length++);

	// building array of renderer bitmaps
	vzTTFontSymbol *symbols = (vzTTFontSymbol*) malloc(length*sizeof(vzTTFontSymbol));

	long posX = 0, posY = 0;

	// filling
	for(int i_text=0;i_text<length;i_text++)
	{
		symbols[i_text].glyph		=	(FT_Glyph)_get_glyph(string[i_text]);
		symbols[i_text].bmp			=	(FT_BitmapGlyph)symbols[i_text].glyph;
		symbols[i_text].character	=	string[i_text];

		symbols[i_text].x = posX;
		symbols[i_text].y = posY;

//		symbols[i_text].draw = 0;

		// control symbols
		if (string[i_text] == 0x0A)
		{
			posX = 0;
			continue;
		};
			
		if(string[i_text] == 0x0D)
		{
			posY += __height;
			continue;
		};

		// check if glyph is present
		if(!(symbols[i_text].glyph))
#ifdef _DEBUG
		{
			// try to log (may by utf convertion fails?
			FILE* f = fopen("vzTTFont.log","at+");
			fprintf(f,"Error symbol (%X) found in string \"%s\"\n",string[i_text],text);
			fclose(f);

			// if glyph not correctly defined - return;

			continue;
		};
#else
			continue;
#endif

//		symbols[i_text].draw = 1;

//		if (FT_HAS_KERNING(((FT_Face)_font_face)) && (i_text))
//		{
//			FT_Vector kerning_delta;
//			FT_Get_Kerning((FT_Face)_font_face,string[i_text - 1],string[i_text],FT_KERNING_DEFAULT,&kerning_delta);
//			posX += kerning_delta.x;
//		};

		// fix position of symbol
		symbols[i_text].x += symbols[i_text].bmp->left;
		symbols[i_text].y += _baseline - symbols[i_text].bmp->top;

		// increment position of pointer
		if (string[i_text] == 0x20)
		{
			// space
			posX += _height/4;	// space is half height
		}
		else
		{
			posX += symbols[i_text].bmp->left + symbols[i_text].bmp->bitmap.width;
		};
	};

	free(string);

	// WRAP IT
	// lets wrap it!!!!!!!!!!
	if(limit_width != (-1))
	{

		int i_line_begin = 0;
		for(i_text=0;i_text<length;i_text++)
		{
			// check for specials newlines symbols
			if ((symbols[i_text].character == 0x0A) || (symbols[i_text].character == 0x0D))
			{
				i_line_begin = i_text + 1;
				continue;
			};

			// skip whitespace
			if (symbols[i_text].character == 0x20)
				continue;

			// check if glyph is present
			if(!(symbols[i_text].glyph))
				// if glyph not correctly defined - skip;
				continue;


			long offset;
			if (((offset = symbols[i_text].x) + symbols[i_text].bmp->bitmap.width) >= limit_width)
			{
				// symbol that comes out of limit found
				
				// there are two methods:
				//		1. break words and move letters to newline
				//		2. look for white-space character back
				if(break_word)
				{
					// look back to line start and found first 
					// symbol that is left then that

					// check that current symbol is not first
					if (i_line_begin != i_text)
					{
						// ok 
						// change position of symbols from current to end
						// vec = (-offset,+__height)
						for(int j=i_text;j<length;j++)
						{
							long x_prev = symbols[j].x;

							symbols[j].x -= offset;
							symbols[j].y += __height;

							if (symbols[j].x < 0)
							{
								symbols[j].x = x_prev;
								offset = 0;
							};
						};
						// current symbol begin newline
						i_line_begin = i_text;
					};
				}
				else
				{
					// look back to line begin (position is i_line_begin)
					// and find first white space
					for(int j=i_text - 1; (j > i_line_begin) && (symbols[j].character != 0x20) && (symbols[j].character != 0x0D) && (symbols[j].character != 0x0A); j--);

					// check symbol code found
					if( (symbols[j].character == 0x20) || (symbols[j].character == 0x0D) || (symbols[j].character == 0x0A) )
					{
						// change position from next character to end
						for(int k = j + 1; k < length; k++)
						{
							// calculate offset
							if(k == j + 1)
							{
								offset = symbols[k].x;
							};

							long x_prev = symbols[k].x;

							symbols[k].x -= offset;
							symbols[k].y += __height;

							if (symbols[k].x < 0)
							{
								symbols[k].x = x_prev;
								offset = 0;
							};
						};
						// current symbol begin newline
						i_line_begin = j + 1;
					};
				};
			};

		};

	};

	// calculate bounding box
	long maxX = 0, maxY = 0;
	for(i_text=0;i_text<length;i_text++)
	{
		// set flag about draw
		symbols[i_text].draw = 0;

		// check if glyph is present
		if(!(symbols[i_text].glyph))
			// if glyph not correctly defined - skip
			continue;

		// vertical limit check
		if ( (limit_height <= symbols[i_text].y + symbols[i_text].bmp->bitmap.rows) && (limit_height != (-1)) )
			continue;

		maxY = _MAX(maxY,symbols[i_text].y + symbols[i_text].bmp->bitmap.rows); 
		maxX = _MAX(maxX,symbols[i_text].x + symbols[i_text].bmp->left + symbols[i_text].bmp->bitmap.width);
		symbols[i_text].draw = 1;
	};

	// RENDER IMAGE
	long image_width = maxX + 1;
	long image_height = maxY + 1;
	// create a surface
	vzImage* temp = vzImageNew			
	(
		image_width,
		image_height,
		4*image_width*image_height
	);

	// clear surface
	memset(temp->surface,0,4*image_width*image_height);

#ifdef _DEBUG
// critical section... some times seem something wrong happens
try
{
#endif

	// draw into surface
	for(i_text=0;i_text<length;i_text++)
	{
		// check if glyph is present
		if(!(symbols[i_text].draw))
			// if glyph not correctly defined - return;
			continue;

		// dest surface begins
		unsigned long* dst = (unsigned long* )temp->surface;
		// incement on x position
		dst += symbols[i_text].x;
		// increment on image width multiplyed on Y position
		dst += 	image_width*symbols[i_text].y;


		unsigned char* src = symbols[i_text].bmp->bitmap.buffer;


		for(int j=0,c=0;j<symbols[i_text].bmp->bitmap.rows;j++,dst+=image_width)
			for(int i=0;i<symbols[i_text].bmp->bitmap.width;i++,c++)
			{
#ifdef _DEBUG
				// some checks
				{
					unsigned long* from = (unsigned long* )temp->surface;
					unsigned long* to = dst + i;
					long length = to - from;
					if ((length >= image_width*image_height) || (length <0))
					{
						FILE* f = fopen("vzTTFont.log","wt");
						fprintf
						(
							f,
							__FILE__ "!Error Here! (image_width='%ld', image_height='%ld', j='%d', i='%d', from='%ld', to='%ld', length='%ld',  text='%s', i_text='%d' ", 
							image_width,
							image_height,
							j,
							i,
							from,
							to,
							length,
							text,
							i_text
						);
						fclose(f);
						exit(-1);
					};
				};
#endif
				*(dst + i) = (colour & 0x00FFFFFF) | ((unsigned long)src[c])<<24;
			};
	};


#ifdef _DEBUG
}
catch(char *error_string)
{
	// try to log (may by utf convertion fails?
	FILE* f = fopen("vzTTFont.log","wt");
	fprintf(f,"Error '%s' happen with string \"%s\"\n",error_string, text);
	fclose(f);
};
#endif

//	char filenam
//	vzImageSaveTGA("d:\\temp\\test_vzTTFont_0.tga",temp,NULL,0);

	free(symbols);

	return temp;
};
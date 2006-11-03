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
	2006-09-21:
		*Fix not initialized '_font_face' destroying
		*Split code of glyphs layouting

	2006-05-06:
		*translating coordinates of glyphs to prevent negative value of
		x axis position.
		*freetype seems not multithread safe. lock it.
		*FT library initalization and moved to LIB load section

    2005-06-08: Code cleanup

*/


#include "vzTTFont.h"
#include "unicode.h"

#include <windows.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

static FT_Library  ft_library;
static HANDLE ft_library_lock;


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			ft_library_lock = CreateMutex(NULL,FALSE,NULL);
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
#define _MIN(A,B) ((A<B)?A:B)

struct vzTTFontSymbol
{
	FT_BitmapGlyph bmp;
	FT_Glyph glyph;
	unsigned short character;
	long x,y;
	long draw;
} ;

struct vzTTFontSymbols
{
	long length;
	long width, height;
	struct vzTTFontSymbol *data;
} ;

VZTTFONT_API vzTTFont::vzTTFont(char* name,int height,int width)
{
	_ready = 0;
	_font_face = NULL;
	lock = CreateMutex(NULL,FALSE,NULL);

	// init symbols collection
	_symbols_lock = CreateMutex(NULL,FALSE,NULL);
	_symbols_count = 0;
	_symbols = malloc(0);

	//filename
	sprintf(_file_name,"fonts/%s.ttf",name);

	// clear cache of glyphs
	for(int i=0;i<65536;i++) _glyphs_cache[i] = NULL;

	// lock ft library
	WaitForSingleObject(ft_library_lock,INFINITE);

	// init new face
	if(FT_New_Face(ft_library,_file_name,0,(FT_Face*)&_font_face))
	{
		// unlock ft library
		ReleaseMutex(ft_library_lock);

		return;
	};

	// setting font dims
	if(FT_Set_Pixel_Sizes((FT_Face)_font_face,_width = width,_height = height))
	{
		// unlock ft library
		ReleaseMutex(ft_library_lock);

		return;
	};

//	if(FT_Set_Char_Size(_font_face,(_width = width)*64,(_height = height)*64,72,72))
//		return;

	_baseline = (((FT_Face)_font_face)->bbox.yMax*_height) / (((FT_Face)_font_face)->bbox.yMax - ((FT_Face)_font_face)->bbox.yMin);

	_ready = 1;

	// unlock ft library
	ReleaseMutex(ft_library_lock);
};

VZTTFONT_API vzTTFont::~vzTTFont()
{
	int i;

	CloseHandle(lock);

	/* destroy symbols collection*/
	for(i=0; i<_symbols_count; i++)
	{
		vzTTFontSymbols* item = ((vzTTFontSymbols**)_symbols)[i];
		if(item)
		{
			free(item->data);
			free(item);
		}

	};
	free(_symbols);
	CloseHandle(_symbols_lock);


	// lock ft library
	WaitForSingleObject(ft_library_lock,INFINITE);

	// free all glyphs
	for(i=0;i<65536;i++)
		if (_glyphs_cache[i])
			FT_Done_Glyph((FT_Glyph)_glyphs_cache[i]);


	if (_font_face)
		FT_Done_Face((FT_Face)_font_face);

	// unlock ft library
	ReleaseMutex(ft_library_lock);

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

		// lock ft library
		WaitForSingleObject(ft_library_lock,INFINITE);

		// loading glyph info face
		FT_Load_Glyph( (FT_Face)_font_face, glyph_index, FT_LOAD_RENDER | FT_LOAD_NO_HINTING);
	
		// storing 
		FT_Glyph temp_glyph;
		FT_Get_Glyph( ((FT_Face)_font_face)->glyph, &temp_glyph);
//		FT_BitmapGlyph bmp = (FT_BitmapGlyph)temp_glyph;
		_glyphs_cache[char_code] = temp_glyph;

		// unlock ft library
		ReleaseMutex(ft_library_lock);
	};
	
	// unlock
	ReleaseMutex((HANDLE)lock);

	// return glyph object
	return _glyphs_cache[char_code];
};


VZTTFONT_API long vzTTFont::ready()
{
	return _ready;
};

VZTTFONT_API void vzTTFont::delete_symbols(long id)
{
	/* lock struct */
	WaitForSingleObject((HANDLE)_symbols_lock,INFINITE);

	/* cast array */
	vzTTFontSymbols **symbols = (vzTTFontSymbols **)_symbols;

	/* check condition */
	if
	(
		(id >= 0)
		&&
		(id < _symbols_count)
		&&
		(symbols[id] != NULL)
	)
	{
		free(symbols[id]->data);
		free(symbols[id]);
		symbols[id] = NULL;
	};

	ReleaseMutex((HANDLE)_symbols_lock);
};

VZTTFONT_API long vzTTFont::get_symbol_width(long id)
{
	/* default value for param */
	long param = 0;

	/* lock struct */
	WaitForSingleObject((HANDLE)_symbols_lock,INFINITE);

	/* cast array */
	vzTTFontSymbols **symbols = (vzTTFontSymbols **)_symbols;

	/* check condition */
	if
	(
		(id >= 0)
		&&
		(id < _symbols_count)
		&&
		(symbols[id] != NULL)
	)
		param = symbols[id]->width;

	ReleaseMutex((HANDLE)_symbols_lock);

	return param;
};

VZTTFONT_API long vzTTFont::get_symbol_height(long id)
{
	/* default value for param */
	long param = 0;

	/* lock struct */
	WaitForSingleObject((HANDLE)_symbols_lock,INFINITE);

	/* cast array */
	vzTTFontSymbols **symbols = (vzTTFontSymbols **)_symbols;

	/* check condition */
	if
	(
		(id >= 0)
		&&
		(id < _symbols_count)
		&&
		(symbols[id] != NULL)
	)
		param = symbols[id]->height;

	ReleaseMutex((HANDLE)_symbols_lock);

	return param;
}



VZTTFONT_API long vzTTFont::insert_symbols(void *new_sym)
{
	int i = 0;

	/* lock struct */
	WaitForSingleObject((HANDLE)_symbols_lock,INFINITE);

	/* cast array */
	vzTTFontSymbols **symbols = (vzTTFontSymbols **)_symbols;

	/* cast input */
	vzTTFontSymbols *new_symbols = (vzTTFontSymbols*)new_sym;

	/* search for empty slots */
	for(i=0; (i<_symbols_count) && (symbols[i] != NULL); i++);
	if(i == _symbols_count)
	{
		/* no empty slots found */
		_symbols_count++;
		_symbols = (symbols = (vzTTFontSymbols**) realloc(_symbols, sizeof(struct vzTTFontSymbols)*_symbols_count));
	};

	/* insert pointer into collection */
	symbols[i] = new_symbols;

	ReleaseMutex((HANDLE)_symbols_lock);

	return i;
};

VZTTFONT_API long vzTTFont::compose(char* string_utf8, struct vzTTFontLayoutConf* l)
{
	struct vzTTFontLayoutConf layout_conf;

	/* check if font inited */
	if(!(_ready))
		return -1;

	/* layout conf */
	layout_conf = (l)?(*l):vzTTFontLayoutConfDefault();

	// line space
	long __height = (long) (layout_conf.line_space*_height);


	// possibly incorrectly counting baseline cause problem
	// how base line could be less then top?
	// could be!!!
	int _baseline = _height;

	// convert to unicode string
	unsigned short int* string_uni = utf8uni((unsigned char*)string_utf8);

	// determinate string length
	int length = 0; for(;string_uni[length];length++);

	// building array of renderer bitmaps
	vzTTFontSymbols *symbols = (vzTTFontSymbols *)malloc(sizeof(struct vzTTFontSymbols));
	symbols->data = (vzTTFontSymbol*) malloc(length*sizeof(struct vzTTFontSymbol));
	symbols->length = length;

	/* start coords */
	long posX = 0, posY = 0;

	// filling
	for(int i_text=0;i_text<length;i_text++)
	{
		symbols->data[i_text].glyph		=	(FT_Glyph)_get_glyph(string_uni[i_text]);
		symbols->data[i_text].bmp		=	(FT_BitmapGlyph)symbols->data[i_text].glyph;
		symbols->data[i_text].character	=	string_uni[i_text];
		symbols->data[i_text].x			= posX;
		symbols->data[i_text].y			= posY;

//		symbols[i_text].draw = 0;

		// control symbols
		if (string_uni[i_text] == 0x0A)
		{
			posX = 0;
			continue;
		};
			
		if(string_uni[i_text] == 0x0D)
		{
			posY += __height;
			continue;
		};

		// check if glyph is present
		if(!(symbols->data[i_text].glyph))
#ifdef _DEBUG
		{
			// try to log (may by utf convertion fails?
			FILE* f = fopen("vzTTFont.log","at+");
			fprintf(f,"Error symbol (%X) found in string \"%s\"\n",string_uni[i_text], string_utf8);
			fclose(f);

			// if glyph not correctly defined - return;

			continue;
		};
#else
			continue;
#endif
/*
		int p = FT_HAS_KERNING(((FT_Face)_font_face));

		if (i_text)
		{
			FT_Vector kerning_delta;

			unsigned int prev = _glyphs_indexes[string_uni[i_text - 1]];
			unsigned int curr = _glyphs_indexes[string_uni[i_text]];

			if 
			(
				0 == FT_Get_Kerning
				(
					(FT_Face)_font_face,
					prev,
					curr,
					FT_KERNING_UNSCALED,
					&kerning_delta
				)
			)
			{
				posX += kerning_delta.x;
			};

		};
*/
		// fix position of symbol
		if(layout_conf.fixed_kerning)
		{
			switch(layout_conf.fixed_kerning_align)
			{
				/* left */
				case 0:
					symbols->data[i_text].x += symbols->data[i_text].bmp->left;
					break;

				/* center */
				case 1:
					symbols->data[i_text].x += (layout_conf.fixed_kerning - symbols->data[i_text].bmp->bitmap.width) /2 ;
					break;

				/* right */
				case 2:
					symbols->data[i_text].x += (layout_conf.fixed_kerning - symbols->data[i_text].bmp->bitmap.width);
					break;
			};
		}
		else
			symbols->data[i_text].x += symbols->data[i_text].bmp->left;

		symbols->data[i_text].y += _baseline - symbols->data[i_text].bmp->top;

		// increment position of pointer
		if (string_uni[i_text] == 0x20)
		{
			// space
			posX += _height/4;	// space is half height
		}
		else
		{
			if(layout_conf.fixed_kerning)
				posX += layout_conf.fixed_kerning;
			else
				posX += symbols->data[i_text].bmp->left + symbols->data[i_text].bmp->bitmap.width;
		};
	};

	free(string_uni);

	// WRAP IT
	// lets wrap it!!!!!!!!!!
	if(layout_conf.limit_width != (-1))
	{

		int i_line_begin = 0;
		for(i_text=0;i_text<length;i_text++)
		{
			// check for specials newlines symbols
			if 
			(
				(symbols->data[i_text].character == 0x0A)
				||
				(symbols->data[i_text].character == 0x0D)
			)
			{
				i_line_begin = i_text + 1;
				continue;
			};

			// skip whitespace
			if (symbols->data[i_text].character == 0x20)
				continue;

			// check if glyph is present
			if(!(symbols->data[i_text].glyph))
				// if glyph not correctly defined - skip;
				continue;

			long offset;
			if 
			(
				(
					(offset = symbols->data[i_text].x)
					+
					symbols->data[i_text].bmp->bitmap.width
				)
				>= 
				layout_conf.limit_width
			)
			{
				// symbol that comes out of limit found
				
				// there are two methods:
				//		1. break words and move letters to newline
				//		2. look for white-space character back
				if(layout_conf.break_word)
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
							long x_prev = symbols->data[j].x;

							symbols->data[j].x -= offset;
							symbols->data[j].y += __height;

							if (symbols->data[j].x < 0)
							{
								symbols->data[j].x = x_prev;
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
					for
					(
						int j=i_text - 1; 
						(j > i_line_begin) 
						&& 
						(symbols->data[j].character != 0x20)
						&&
						(symbols->data[j].character != 0x0D)
						&&
						(symbols->data[j].character != 0x0A); 
						j--
					);

					// check symbol code found
					if
					(
						(symbols->data[j].character == 0x20)
						||
						(symbols->data[j].character == 0x0D)
						||
						(symbols->data[j].character == 0x0A) 
					)
					{
						// change position from next character to end
						for(int k = j + 1; k < length; k++)
						{
							// calculate offset
							if(k == j + 1)
								offset = symbols->data[k].x;

							long x_prev = symbols->data[k].x;

							symbols->data[k].x -= offset;
							symbols->data[k].y += __height;

							if (symbols->data[k].x < 0)
							{
								symbols->data[k].x = x_prev;
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
	long maxX = 0, maxY = 0, minX = 0, minY = 0;
	for(i_text=0;i_text<length;i_text++)
	{
		// set flag about draw
		symbols->data[i_text].draw = 0;

		// check if glyph is present
		if(!(symbols->data[i_text].glyph))
			// if glyph not correctly defined - skip
			continue;

		// vertical limit check
		if
		(
			(layout_conf.limit_height <= symbols->data[i_text].y + symbols->data[i_text].bmp->bitmap.rows)
			&& 
			(layout_conf.limit_height != (-1)) 
		)
			continue;

		// calc new min/max
		maxY = _MAX(maxY,symbols->data[i_text].y + symbols->data[i_text].bmp->bitmap.rows); 
		minY = _MIN(minY,symbols->data[i_text].y); 
		maxX = _MAX(maxX,symbols->data[i_text].x + symbols->data[i_text].bmp->left + symbols->data[i_text].bmp->bitmap.width);
		minX = _MIN(minX,symbols->data[i_text].x);
		symbols->data[i_text].draw = 1;
	};

	// translate with X coordinate
	if(minX < 0)
	{
		for(i_text=0;i_text<length;i_text++)
			symbols->data[i_text].x -= minX;
		maxX -= minX;
		minX = 0;
	};

	symbols->width = maxX + 1;
	symbols->height = maxY + 1;

	return insert_symbols(symbols);
};

inline void blit_glyph(unsigned long* dst, long dst_width, unsigned char* src, long src_height, long src_width, unsigned long colour
#ifdef _DEBUG
, unsigned long* dst_limit
#endif
)
{
	for
	(
		int j=0,c=0;
		j<src_height;
		j++,dst+=dst_width,src+=src_width
	)
		for
		(
			int i=0;
			i<src_width;
			i++,c++
		)
		{
#ifdef _DEBUG
			if (dst > dst_limit)
			{
				fprintf(stderr, "Assert in 'blit_glyph' - dst > dst_limit\n");
			}
			else
			{
#endif

			*(dst + i) |= (colour & 0x00FFFFFF) | ((unsigned long)src[i])<<24;
#ifdef _DEBUG
			}
#endif
		};
}


VZTTFONT_API void vzTTFont::render_to(vzImage* image, long x , long y, long text_id, long colour)
{
	/* get symbol */
	WaitForSingleObject((HANDLE)_symbols_lock,INFINITE);
	vzTTFontSymbols *symbols = ((vzTTFontSymbols **)_symbols)[text_id];
	ReleaseMutex((HANDLE)_symbols_lock);	

	if (symbols == NULL) return;

#ifdef _DEBUG
// critical section... some times seem something wrong happens
try
{
#endif

	// draw into surface
	for(int i_text=0;i_text<symbols->length;i_text++)
	{
		// check if glyph is present
		if(!(symbols->data[i_text].draw))
			// if glyph not correctly defined - return;
			continue;

		/* check if glyph is inside of surface */
		if
		(
			/* symbols is right of surface*/
			((symbols->data[i_text].x + x) < 0)
			||
			/* symbols is below of surface*/
			((symbols->data[i_text].y + y) < 0)
			||
			/* symbols is right of surface*/
			((symbols->data[i_text].x + x + symbols->data[i_text].bmp->bitmap.width) > image->width)
			||
			/* symbols is below of surface*/
			((symbols->data[i_text].y + y + symbols->data[i_text].bmp->bitmap.rows) > image->height)
		)
			continue;

		/* blit glyph */
		blit_glyph
		(
			/* base dst pointer */
			(unsigned long* )image->surface 
				/* incement on x position */
				+ symbols->data[i_text].x + x					
				/* incement on y position */
				+ (symbols->data[i_text].y + y)*image->width,
			/* source width */
			image->width,
			/* glyph surface ptr */
			symbols->data[i_text].bmp->bitmap.buffer,
			/* glyph rows */
			symbols->data[i_text].bmp->bitmap.rows,
			/* glyph width */
			symbols->data[i_text].bmp->bitmap.width,
			/* colour */
			colour
#ifdef _DEBUG
			, (unsigned long* )image->surface + image->width*image->height
#endif
		);

	};


#ifdef _DEBUG
}
catch(char *error_string)
{
	// try to log (may by utf convertion fails?
	fprintf(stderr, "Error happens: '%s' \n", error_string);
};
#endif

//	char filenam
//	vzImageSaveTGA("d:\\temp\\test_vzTTFont_0.tga",temp,NULL,0);
}

VZTTFONT_API vzImage* vzTTFont::render(char* text, long colour, struct vzTTFontLayoutConf* l)
{
	struct vzTTFontLayoutConf layout_conf;

	if(!(_ready))
		return NULL;

	/* layout conf */
	layout_conf = (l)?(*l):vzTTFontLayoutConfDefault();


	/* compose text layout */
	long text_id = compose(text, &layout_conf);
	if(text_id == (-1))
		return NULL;

	/* get symbol */
	WaitForSingleObject((HANDLE)_symbols_lock,INFINITE);
	vzTTFontSymbols *symbols = ((vzTTFontSymbols **)_symbols)[text_id];
	ReleaseMutex((HANDLE)_symbols_lock);	

	// create a surface
	vzImage* temp = vzImageNew			
	(
		symbols->width,
		symbols->height,
		4*symbols->width*symbols->height
	);

	// RENDER IMAGE
	render_to(temp, 0, 0, text_id, colour);

	delete_symbols(text_id);

	return temp;
};

struct vzTTFontLayoutConf vzTTFontLayoutConfDefault(void) 
{ 
	struct vzTTFontLayoutConf d = vzTTFontLayoutConfDefaultData;
	return d;
};
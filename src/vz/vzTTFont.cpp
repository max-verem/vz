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
    2008-09-24:
        *logger use for message outputs

	2007-11-16: 
		*Visual Studio 2005 migration.

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

#define _CRT_SECURE_NO_WARNINGS

#include "vzTTFont.h"
#include "unicode.h"
#include "vzLogger.h"

#include <windows.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_STROKER_H

static FT_Library  ft_library;
static HANDLE ft_library_lock;

#define MAX_FONT_PATHS 16
static char* font_paths[MAX_FONT_PATHS];

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:

			/* init library */
			ft_library_lock = CreateMutex(NULL,FALSE,NULL);
			FT_Init_FreeType( &ft_library );

			/* fonts path storage init */
			memset(font_paths, 0, MAX_FONT_PATHS*sizeof(char*));

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
	struct
	{
		long offset_x;
		long offset_y;
		FT_BitmapGlyph bmp;
		FT_Glyph glyph;
	} layers[2];		/* layer 0 - base image, layer 1 - stroke */

	/* character code */
	unsigned short character;

	/* symbol position */
	long x,y;

	long draw;
};

struct vzTTFontSymbols
{
	long length;
	long width, height;
	struct vzTTFontSymbol *data;
} ;

static struct vzTTFontParams vzTTFontParamsDefaultData = vzTTFontParamsDefault;

VZTTFONT_API vzTTFont::vzTTFont(char* name, struct vzTTFontParams* params)
{
	_ready = 0;
	_font_face = NULL;
	_font_stroker = NULL;
	lock = CreateMutex(NULL,FALSE,NULL);

	// init symbols collection
	_symbols_lock = CreateMutex(NULL,FALSE,NULL);
	_symbols_count = 0;
	_symbols = malloc(0);


	/* params */
	_params = (params)?*params:vzTTFontParamsDefaultData;

	// clear cache of glyphs
	memset(_glyphs_cache, 0, sizeof(void*)*VZTTFONT_MAX_GLYPHS);
	memset(_strokes_cache, 0, sizeof(void*)*VZTTFONT_MAX_GLYPHS);
	memset(_glyphs_indexes, 0, sizeof(long)*VZTTFONT_MAX_GLYPHS);

	// lock ft library
	WaitForSingleObject(ft_library_lock,INFINITE);

	// init new face
	for(int i = 0; (i<MAX_FONT_PATHS) && (NULL == _font_face) && (NULL != font_paths[i]); i++)
	{
		/* prepare file name */
		sprintf
		(
			_file_name,
			"%s/%s.ttf",
			font_paths[i],
			name
		);

		/* init face */
		FT_New_Face(ft_library,_file_name,0,(FT_Face*)&_font_face);
	};
	/* check if it was possible to init font face */
	if(NULL == _font_face)
	{
		// unlock ft library
		ReleaseMutex(ft_library_lock);

		return;
	};

	// setting font dims
	if(FT_Set_Pixel_Sizes((FT_Face)_font_face, _params.width, _params.height))
	{
		// unlock ft library
		ReleaseMutex(ft_library_lock);

		return;
	};

	/* create a stroker */
	if(0 != _params.stroke_radius)
	{
		FT_Stroker_New(ft_library, (FT_Stroker*)&_font_stroker );
	    FT_Stroker_Set
		(
			(FT_Stroker)_font_stroker, 
			(unsigned long)(_params.stroke_radius * 16.0),
			(FT_Stroker_LineCap)_params.stroke_line_cap, 
			(FT_Stroker_LineJoin)_params.stroke_line_join, 
			0x20000
		);
	};

//	if(FT_Set_Char_Size(_font_face,(_width = width)*64,(_height = height)*64,72,72))
//		return;

	_baseline = 
		(((FT_Face)_font_face)->bbox.yMax * _params.height) 
		/ 
		(
			((FT_Face)_font_face)->bbox.yMax 
			- 
			((FT_Face)_font_face)->bbox.yMin
		);

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
	{
		if (_glyphs_cache[i])
			FT_Done_Glyph((FT_Glyph)_glyphs_cache[i]);

		if (_strokes_cache[i])
			FT_Done_Glyph((FT_Glyph)_strokes_cache[i]);
	};

	if(_font_stroker)
		FT_Stroker_Done((FT_Stroker)_font_stroker);

	if (_font_face)
		FT_Done_Face((FT_Face)_font_face);

	// unlock ft library
	ReleaseMutex(ft_library_lock);

};


int vzTTFont::_get_glyph(unsigned short char_code, void** font_glyph, void** stroke_glyph)
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
			*font_glyph = NULL;
			*stroke_glyph = NULL;

			// unlock
			ReleaseMutex((HANDLE)lock);

			return NULL;
		};


		// saving glyph index
		_glyphs_indexes[char_code] = glyph_index;

		// lock ft library
		WaitForSingleObject(ft_library_lock,INFINITE);

		// loading glyph info face
		FT_Load_Glyph( (FT_Face)_font_face, glyph_index, FT_LOAD_RENDER | FT_LOAD_NO_HINTING );
	
		// storing 
		FT_Glyph temp_glyph;
		FT_Get_Glyph( ((FT_Face)_font_face)->glyph, &temp_glyph);
//		FT_BitmapGlyph bmp = (FT_BitmapGlyph)temp_glyph;
		_glyphs_cache[char_code] = temp_glyph;

		/* load again glyph for stroking */
		if(_font_stroker)
		{
			FT_Load_Glyph( (FT_Face)_font_face, glyph_index, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING);
			FT_Get_Glyph( ((FT_Face)_font_face)->glyph, &temp_glyph);
			FT_Glyph_Stroke( &temp_glyph, (FT_Stroker)_font_stroker, 1);
			FT_Glyph_To_Bitmap(&temp_glyph, FT_RENDER_MODE_NORMAL, NULL, 0);
			_strokes_cache[char_code] = temp_glyph;
		};

		// unlock ft library
		ReleaseMutex(ft_library_lock);
	};

	/* setup params */
	*font_glyph = _glyphs_cache[char_code];
	*stroke_glyph = _strokes_cache[char_code];

	// unlock
	ReleaseMutex((HANDLE)lock);

	// return glyph object
	return 1;
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
	int i_text;

	struct vzTTFontLayoutConf layout_conf;

	/* check if font inited */
	if(!(_ready))
		return -1;

	/* layout conf */
	layout_conf = (l)?(*l):vzTTFontLayoutConfDefault();

	// line space
	long __height = (long) (layout_conf.line_space * _params.height);


	// possibly incorrectly counting baseline cause problem
	// how base line could be less then top?
	// could be!!!
	int _baseline = _params.height;

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

	/* Here we should determinate what layer is used for calculating layout */
	int i_layer;
	if(0 != _params.stroke_radius)
		/* all dimensions calculated according to outlines */
		i_layer = 1;
	else
		i_layer = 0;

	// filling
	for(i_text=0;i_text<length;i_text++)
	{
		/* set glyphs */
		_get_glyph
		(
			string_uni[i_text], 
			(void**)&symbols->data[i_text].layers[0].glyph, /* main glyph */
			(void**)&symbols->data[i_text].layers[1].glyph	/* stroke */
		);

		/* request bitmap for font */
		symbols->data[i_text].layers[0].bmp =	
			(FT_BitmapGlyph)symbols->data[i_text].layers[0].glyph;
		
		/* request bitmap for stroke */
		if(NULL != symbols->data[i_text].layers[1].glyph) 
			symbols->data[i_text].layers[1].bmp =
				(FT_BitmapGlyph)symbols->data[i_text].layers[1].glyph;
		else
			symbols->data[i_text].layers[1].bmp = NULL;

		/* base params */
		symbols->data[i_text].character	= string_uni[i_text];
		symbols->data[i_text].x			= posX;
		symbols->data[i_text].y			= posY;

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

		/* layers offset */
		for(int l = 0; (l < 2) && (NULL != symbols->data[i_text].layers[l].bmp); l++)
		{
			symbols->data[i_text].layers[l].offset_x = 
				- symbols->data[i_text].layers[i_layer].bmp->left
				+
				symbols->data[i_text].layers[l].bmp->left;
			
			symbols->data[i_text].layers[l].offset_y = 
				symbols->data[i_text].layers[i_layer].bmp->top
				- 
				symbols->data[i_text].layers[l].bmp->top;
		};

//		symbols[i_text].draw = 0;

		// check if glyph is present
		if(!(symbols->data[i_text].layers[i_layer].glyph))
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


		// fix position of symbol
		long x_advance = ((long)(layout_conf.advance_ratio * symbols->data[i_text].layers[i_layer].glyph->advance.x)) >> 16;
		if(layout_conf.fixed_kerning)
		{
			switch(layout_conf.fixed_kerning_align)
			{
				/* left */
				case 0:
					symbols->data[i_text].x += 
						symbols->data[i_text].layers[i_layer].bmp->left;
					break;

				/* center */
				case 1:
					symbols->data[i_text].x += 
						(layout_conf.fixed_kerning - x_advance) /2 ;
					break;

				/* right */
				case 2:
					symbols->data[i_text].x += 
						(layout_conf.fixed_kerning - x_advance);
					break;
			};
		}
		else
		{
			symbols->data[i_text].x += 
				symbols->data[i_text].layers[i_layer].bmp->left;

			if (i_text)
			{
				FT_Vector kerning_delta;

				if 
				(
					0 == FT_Get_Kerning
					(
						(FT_Face)_font_face,
						_glyphs_indexes[ string_uni[i_text - 1]],
						_glyphs_indexes[ string_uni[i_text - 0]],
						FT_KERNING_DEFAULT, 
						&kerning_delta
					)
				)
				{
					symbols->data[i_text].x -= kerning_delta.x >> 6;
					posX -= kerning_delta.x >> 6;
				};
			};
		};

		symbols->data[i_text].y += 
			_baseline - 
			symbols->data[i_text].layers[i_layer].bmp->top;

		// increment position of pointer
		if (string_uni[i_text] == 0x20)
		{
			// space
			posX += _params.height/4;	// space is half height
		}
		else
		{
			if(layout_conf.fixed_kerning)
				posX += layout_conf.fixed_kerning;
			else
				posX += x_advance;
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
			if(!(symbols->data[i_text].layers[i_layer].glyph))
				// if glyph not correctly defined - skip;
				continue;

			long offset;
			if 
			(
				(
					(offset = symbols->data[i_text].x)
					+
					symbols->data[i_text].layers[i_layer].bmp->bitmap.width
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
					int j;
					for
					(
						j=i_text - 1; 
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
	for(int l = 0; (l < 2) && (NULL != symbols->data[i_text].layers[l].bmp); l++)
	{
		// set flag about draw
		symbols->data[i_text].draw = 0;

		// check if glyph is present
		if(!(symbols->data[i_text].layers[l].glyph))
			// if glyph not correctly defined - skip
			continue;

		// vertical limit check
		if
		(
			(
				layout_conf.limit_height 
				<= 
				(
					symbols->data[i_text].y 
					+ 
					symbols->data[i_text].layers[l].bmp->bitmap.rows
					+
					symbols->data[i_text].layers[l].offset_y
				)
			)
			&& 
			(layout_conf.limit_height != (-1)) 
		)
			continue;

		// calc new min/max
		maxY = _MAX
		(
			maxY,
			symbols->data[i_text].y + 
			symbols->data[i_text].layers[l].bmp->bitmap.rows +
			symbols->data[i_text].layers[l].offset_y
		); 
		minY = _MIN(minY,symbols->data[i_text].y); 

		maxX = _MAX
		(
			maxX,
			symbols->data[i_text].x + 
			symbols->data[i_text].layers[l].bmp->left + 
			symbols->data[i_text].layers[l].bmp->bitmap.width +
			symbols->data[i_text].layers[l].offset_x
		);
		minX = _MIN(minX,symbols->data[i_text].x);

		symbols->data[i_text].draw = 1;
	};

	// translate with X coordinate
	if(minX <= 0)
	{
		for(i_text=0;i_text<length;i_text++)
			symbols->data[i_text].x -= minX - 1;
		maxX -= minX - 1;
		minX = 0;
	};

	symbols->width = maxX + 1;
	symbols->height = maxY + 1;

	return 
		insert_symbols(symbols);
};

static inline void blit_glyph(unsigned long* dst, long dst_width, unsigned char* src, long src_height, long src_width, unsigned long colour
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
				logger_printf(1, "vzTTFong: Assert in 'blit_glyph' - dst > dst_limit");
			}
			else
			{
#else /* !_DEBUG */
			{
#endif /* _DEBUG */

#define _cA(V) ((unsigned long)((V & 0xFF000000) >> 24))
#define _cR(V) ((unsigned long)((V & 0x00FF0000) >> 16))
#define _cG(V) ((unsigned long)((V & 0x0000FF00) >> 8))
#define _cB(V) ((unsigned long)((V & 0x000000FF) >> 0))

				/* multiply glyph alpha with colour alpha */
				unsigned long _Ta = (((unsigned long)src[i])*_cA(colour)) >> 8;
				
				/* for more information see BLENDING file */
				if(_Ta > 3)
				{
					unsigned long _B = *(dst + i);
					unsigned long _Ba = _cA(_B);
					unsigned long t1 = (0x100 - _Ta)*_Ba + (_Ta<<8);
					unsigned long _Ra = t1 >> 8;
					if(t1)
					{
						unsigned long
							t2r = (0x100 - _Ta)*_cR(_B)*_Ba + ((_cR(colour)*_Ta)<<8),
							t2g = (0x100 - _Ta)*_cG(_B)*_Ba + ((_cG(colour)*_Ta)<<8),
							t2b = (0x100 - _Ta)*_cB(_B)*_Ba + ((_cB(colour)*_Ta)<<8);

						*(dst + i) = 
							(_Ra << 24) |
							((t2r / t1 ) << 16) |
							((t2g / t1 ) << 8) |
							((t2b / t1 ) << 0);
					}
					else
						*(dst + i) = 0;

				};

/////				*(dst + i) |= (colour & 0x00FFFFFF) | ((unsigned long)(src[i]))<<24;
#ifdef _DEBUG
			}
#else /* !_DEBUG */
			}
#endif /* _DEBUG */
		};
}


VZTTFONT_API void vzTTFont::render_to(vzImage* image, long x , long y, long text_id, long font_colour, long stroke_colour)
{
	/* get symbol */
	WaitForSingleObject((HANDLE)_symbols_lock,INFINITE);
	vzTTFontSymbols *symbols = ((vzTTFontSymbols **)_symbols)[text_id];
	ReleaseMutex((HANDLE)_symbols_lock);	

	if (symbols == NULL) return;

	/* check length of sequence */
	if (0 == symbols->length) return;


#ifdef _DEBUG
// critical section... some times seem something wrong happens
try
{
#endif

	// draw into surface
	for(int l = 0; (l<2) && (NULL != symbols->data[0].layers[l].bmp); l++)
	for(int i_text=0;i_text<symbols->length;i_text++)
	{
		// check if glyph is present
		if(!(symbols->data[i_text].draw))
			// if glyph not correctly defined - return;
			continue;
		if(NULL == symbols->data[i_text].layers[l].bmp)
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
			((symbols->data[i_text].x + x + symbols->data[i_text].layers[l].bmp->bitmap.width) > image->width)
			||
			/* symbols is below of surface*/
			((symbols->data[i_text].y + y + symbols->data[i_text].layers[l].bmp->bitmap.rows) > image->height)
		)
			continue;

		/* blit glyph */
		blit_glyph
		(
			/* base dst pointer */
			(unsigned long* )image->surface 
				/* incement on x position */
				+ symbols->data[i_text].x + x + symbols->data[i_text].layers[l].offset_x
				/* incement on y position */
				+ 
					(symbols->data[i_text].y + y + symbols->data[i_text].layers[l].offset_y)
					*
					image->width,
			/* source width */
			image->width,
			/* glyph surface ptr */
			symbols->data[i_text].layers[l].bmp->bitmap.buffer,
			/* glyph rows */
			symbols->data[i_text].layers[l].bmp->bitmap.rows,
			/* glyph width */
			symbols->data[i_text].layers[l].bmp->bitmap.width,
			/* colour */
			(0 == l)?font_colour:stroke_colour
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
	logger_printf(1, "vzTTFont: Error happens: '%s'", error_string);
};
#endif

//	char filenam
//	vzImageSaveTGA("d:\\temp\\test_vzTTFont_0.tga",temp,NULL,0);
}

VZTTFONT_API vzImage* vzTTFont::render(char* text, long font_colour, long stroke_colour, struct vzTTFontLayoutConf* l)
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
	vzImage* temp;
    vzImageCreate(&temp, symbols->width, symbols->height);

	// RENDER IMAGE
	render_to(temp, 0, 0, text_id, font_colour, stroke_colour);

	delete_symbols(text_id);

	return temp;
};

VZTTFONT_API void vzTTFontAddFontPath(char* path)
{
	int i;

	/* search for next slot */
	for(i = 0; i<MAX_FONT_PATHS; i++)
		if(NULL == font_paths[i])
		{
			font_paths[i] = path;
			return;
		};
};

struct vzTTFontLayoutConf vzTTFontLayoutConfDefault(void) 
{ 
	struct vzTTFontLayoutConf d = vzTTFontLayoutConfDefaultData;
	return d;
};


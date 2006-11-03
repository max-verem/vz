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
	2006-05-06:
		*Migration to freetype-2.1.10 - linking with freetype.lib instead of 
		libfreetype.lib

    2005-06-08: Code cleanup

*/


#ifndef VZTTFONT_H
#define VZTTFONT_H

#ifdef VZTTFONT_EXPORTS
#define VZTTFONT_API __declspec(dllexport)
#ifdef _DEBUG
#pragma comment(lib, "freetype221_D.lib") 
#else
#pragma comment(lib, "freetype221.lib") 
#endif
//#pragma comment(lib, "freetype.lib") 
//#pragma comment(lib, "libfreetype.lib") 
#else
#define VZTTFONT_API __declspec(dllimport)
#pragma comment(lib, "vzTTFont.lib") 
#endif

#include "../vz/vzImage.h"

struct vzTTFontLayoutConf
{
	float line_space;
	long break_word;
	long limit_width;
	long limit_height;
	long fixed_kerning;
	long fixed_kerning_align; /* 0 - left, 1 - center, 2 - right */
};

#define vzTTFontLayoutConfDefaultData		\
{											\
	1.0,	/* line_space			*/		\
	0,		/* break_word			*/		\
	-1,		/* limit_width			*/		\
	-1,		/* limit_height			*/		\
	0,		/* fixed_kerning		*/		\
	1		/* fixed_kerning_align	*/		\
}


struct vzTTFontLayoutConf vzTTFontLayoutConfDefault(void);

class VZTTFONT_API vzTTFont
{
	// freetype font face
	void* _font_face;

	// caches
	void* _glyphs_cache[65536];
	long _glyphs_indexes[65536];

	// font file name
	char _file_name[128];

	// geoms
	long _height;
	long _width;
	long _baseline;
	long _limit_width;
	long _limit_height;

	// try to lock
	void* lock;

	// flags
	long _ready;

	// symbols collection
	void* _symbols_lock;
	void* _symbols;
	long _symbols_count;
	long insert_symbols(void *new_symbols);

public:
	void* _get_glyph(unsigned short char_code);

	vzTTFont(char* name,int height,int width = 0);
	long ready();
	vzImage* render(char* text, long colour, struct vzTTFontLayoutConf* l = 0);

	void delete_symbols(long id);
	long compose(char* string_utf8, struct vzTTFontLayoutConf* l = 0);
	void render_to(vzImage* image, long x , long y, long text_id, long colour);
	long get_symbol_width(long id);
	long get_symbol_height(long id);

	~vzTTFont();
};

#endif
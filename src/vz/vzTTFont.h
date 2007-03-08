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
#pragma comment(lib, "freetype231_D.lib") 
#else
#pragma comment(lib, "freetype231.lib") 
#endif
//#pragma comment(lib, "freetype.lib") 
//#pragma comment(lib, "libfreetype.lib") 
#else
#define VZTTFONT_API __declspec(dllimport)
#pragma comment(lib, "vzTTFont.lib") 
#endif

#include "../vz/vzImage.h"

#define VZTTFONT_MAX_GLYPHS 65536

struct vzTTFontLayoutConf
{
	float line_space;
	long break_word;
	long limit_width;
	long limit_height;
	long fixed_kerning;
	long fixed_kerning_align; /* 0 - left, 1 - center, 2 - right */
	float advance_ratio;
};

struct vzTTFontParams
{
	long height;
	long width;
	float stroke_radius;
	long stroke_line_cap;
	long stroke_line_join;
};

#define vzTTFontParamsDefault				\
{											\
	0,		/* height				*/		\
	0,		/* width				*/		\
	0.0,	/* stroke_radius		*/		\
	0,		/* stroke_line_cap		*/		\
	0		/* stroke_line_join		*/		\
}

#define vzTTFontLayoutConfDefaultData		\
{											\
	1.0,	/* line_space			*/		\
	0,		/* break_word			*/		\
	-1,		/* limit_width			*/		\
	-1,		/* limit_height			*/		\
	0,		/* fixed_kerning		*/		\
	1,		/* fixed_kerning_align	*/		\
	1.0		/* advance_ratio		*/		\
}


struct vzTTFontLayoutConf vzTTFontLayoutConfDefault(void);

class VZTTFONT_API vzTTFont
{
	// freetype font face
	void* _font_face;
	void* _font_stroker;

	// caches
	void* _glyphs_cache[VZTTFONT_MAX_GLYPHS];
	void* _strokes_cache[VZTTFONT_MAX_GLYPHS];
	long _glyphs_indexes[VZTTFONT_MAX_GLYPHS];

	// font file name
	char _file_name[128];

	struct vzTTFontParams _params;

	// geoms
	long _baseline;
//	long _limit_width;
//	long _limit_height;

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
	int _get_glyph(unsigned short char_code, void** font_glyph, void** stroke_glyph);

	vzTTFont(char* name, struct vzTTFontParams* params);
	long ready();
	vzImage* render(char* text, long font_colour, long stroke_colour, struct vzTTFontLayoutConf* l = 0);

	void delete_symbols(long id);
	long compose(char* string_utf8, struct vzTTFontLayoutConf* l = 0);
	void render_to(vzImage* image, long x , long y, long text_id, long font_colour, long stroke_colour);
	long get_symbol_width(long id);
	long get_symbol_height(long id);

	~vzTTFont();
};

#endif
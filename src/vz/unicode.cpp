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

#include "memleakcheck.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define uint16 unsigned short

char* uni2utf8(short uni[])
{
	int uni_len=0;

	// determinating unicode string length
	while(uni[uni_len++]);

	// allocating memmory for destination buffer
	char* string = (char*)malloc(3*uni_len+3);
	memset(string,0,3*uni_len + 3);

    int uni_pos=0,str_pos=0;
    for(; uni_pos < uni_len ;uni++,uni_pos++)
    {
		uint16 uni_char = *uni;

		if(uni_char<128)
		{
			string[str_pos + 0] = (char)uni_char;
			string[str_pos + 1] = 0;
			str_pos += 1;
		}
		else
		if(uni_char<2048)
		{
			string[str_pos + 0] = (char)((uni_char>>6)+192);
			string[str_pos + 1] = (char)((uni_char&63)+128);
			string[str_pos + 2] = 0;
			str_pos += 2;
		}
		else
		if(uni_char<32768)
		{
			string[str_pos + 0] = (char)((uni_char>>12)+224);
			string[str_pos + 1] = (char)(((uni_char>>6)&63)+128);
			string[str_pos + 2] = (char)((uni_char&63)+128);
			string[str_pos + 3] = 0;
			str_pos += 3;
		}
 
	};

	return string;
};

typedef enum UTF8STAGES
{
	start,
	finish,
	found_unicode,
	one1,
	two1,
	two2,
	three1,
	three2,
	three3
};

void free_unicode_data(void* data)
{
	free(data);
};


unsigned short int* utf8uni(unsigned char* utf)
{
	// init space
	unsigned short int* unicode = (unsigned short int*) malloc(sizeof(unsigned short) * strlen((char*)utf) + 2);
	unsigned short int* output_string = unicode;

	UTF8STAGES stage = start;
	unsigned short int symbol;
	while(stage != finish)
		switch(stage)
		{
			case start:
				*unicode = 0;
				if (*utf)
				{
					if((*utf)<128)
					{
						stage = one1;
						break;
					};

					if((*utf>=192)&&(*utf<224))
					{
						stage = two1;
						break;
					};

					if((*utf>=224)&&(*utf<240))
					{
						stage = three1;
						break;
					};
				}
				else
				{
					stage = finish;
					break;
				};
				utf++;
				break;

			case one1:
				*unicode = (symbol = *utf);
				unicode++,utf++;
				stage = start;
				break;

			case two1:
				*unicode = (*utf - 192) << 6;
				utf++;
				stage = (*utf>=128)?two2:start;
				break;

			case two2:
				*unicode = *unicode + *utf - 128;
				unicode++,utf++;
				stage = start;
				break;

			case three1:
				*unicode = (*utf - 224) << 12;
				utf++;
				stage = (*utf>=128)?three2:start;
				break;

			case three2:
				*unicode |= (*utf - 128) << 6;
				utf++;
				stage = (*utf>=128)?three3:start;
				break;

			case three3:
				*unicode |= (*utf - 128);
				unicode++,utf++;
				stage = start;
				break;

		}
	return output_string;
};


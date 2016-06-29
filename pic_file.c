/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin
    Copyright (C) 2003-2007 Lucas Martin-King

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This file incorporates work covered by the following copyright and
    permission notice:

    Copyright (c) 2013, Cong Xu
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/
#include "pic_file.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


void swap16 (void *d)
{
	return;
}
size_t f_read(FILE *f, void *buf, size_t size)
{
	return fread(buf, size, 1, f);
}
size_t f_read16(FILE *f, void *buf, size_t size)
{
	size_t ret = 0;
	if (buf)
	{
		ret = f_read(f, buf, size);
		swap16((int16_t *)buf);
	}
	return ret;
}
#define CMALLOC(_var, _size)\
{\
	_var = malloc(_size);\
}

// Some pics are palette swapped; use white for them
static uint8_t cWhiteValues[] = { 64, 56, 46, 36, 30, 24, 20, 16 };

typedef struct
{
	uint8_t r, g, b;
} PaletteColor;

#define WALL_COLORS       208
#define FLOOR_COLORS      216
#define ROOM_COLORS       232
#define ALT_COLORS        224
static void SetGreyRange(const int start, TPalette palette)
{
	for (int i = 0; i < 8; i++)
	{
		palette[start + i].r =
		palette[start + i].g =
		palette[start + i].b = cWhiteValues[i];
		palette[start + i].a = 255;
	}
}
static void SetRedRange(const int start, TPalette palette)
{
	for (int i = 0; i < 8; i++)
	{
		palette[start + i].r = cWhiteValues[i];
		palette[start + i].g =
		palette[start + i].b = 0;
		palette[start + i].a = 255;
	}
}
static void SetPaletteGreyRanges(TPalette palette)
{
	SetGreyRange(WALL_COLORS, palette);
	SetGreyRange(FLOOR_COLORS, palette);
	SetGreyRange(ROOM_COLORS, palette);
	// Set alt colours as red so they can be distinguished
	SetRedRange(ALT_COLORS, palette);
}

int ReadPics(
	const char *filename, PicPaletted **pics, int maxPics, TPalette *palette)
{
	FILE *f;
	int is_eof = 0;
	uint16_t size;
	int i = 0;

	f = fopen(filename, "rb");
	if (f != NULL) {
		size_t elementsRead;
	#define CHECK_FREAD(count)\
		if (elementsRead != count) {\
			fclose(f);\
			return 0;\
		}
		for (i = 0; i < 256; i++)
		{
			PaletteColor pc;
			elementsRead = fread(&pc, sizeof pc, 1, f);
			CHECK_FREAD(1)
			if (palette != NULL && *palette)
			{
				(*palette)[i].r = pc.r;
				(*palette)[i].g = pc.g;
				(*palette)[i].b = pc.b;
				(*palette)[i].a = 255;
			}
		}
		if (palette != NULL) SetPaletteGreyRanges(*palette);

		i = 0;
		while (!is_eof)
		{
			elementsRead = fread(&size, sizeof(size), 1, f);
			if (elementsRead != 1)
			{
				break;
			}
			swap16(&size);
			if (size > 0)
			{
				PicPaletted *p;
				CMALLOC(p, size);

				f_read16(f, &p->w, 2);
				f_read16(f, &p->h, 2);

				f_read(f, &p->data, size - 4);

				pics[i] = p;

				if (ferror(f) || feof(f))
				{
					is_eof = 1;
				}
			}
			else
			{
				pics[i] = NULL;
			}
			i++;
		}
		fclose(f);
	#undef CHECK_FREAD
	}
	return i;
}

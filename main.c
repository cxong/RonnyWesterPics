#include "bmpfile.h"
#include "pic_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define CLAMP(v, _min, _max) MAX((_min), MIN((_max), (v)))

#ifndef strcasecmp
#define strcasecmp stricmp
#endif

TPalette palette;

static void PicPalettedToBMP(const PicPaletted *pic, const TPalette palette);

int ReadPicsFile(const char *filename, const bool hasPalette)
{
	PicPaletted **pics = calloc(9999, sizeof(PicPaletted *));
	int i;
	int picsRead = ReadPics(filename, pics, 9999, hasPalette ? &palette : NULL);
	for (i = 0; i < picsRead; i++)
	{
		PicPaletted *pic = pics[i];
		if (pic != NULL)
		{
			PicPalettedToBMP(pic, palette, filename, i, 4);
		}
	}
	free(pics);
	return picsRead;
}

void ReadMagusArt(const char *filename);

int main(int argc, char *argv[])
{
	for (int i = 1; i < argc; i++)
	{
		printf("Reading %s...", argv[i]);
		if (strcasecmp(argv[i] + strlen(argv[i]) - 4, ".art") == 0)
		{
			// magus.art
			ReadMagusArt(argv[i]);
		}
		else if (strcasecmp(argv[i] + strlen(argv[i]) - 4, ".mgs") == 0)
		{
			// world.mgs
		}
		else
		{
			// Cyberdogs/C-Dogs px
			const int read = ReadPicsFile(argv[i], i == 1);
			printf("read %d\n", read);
		}
	}
	return 0;
}

void ReadMagusArt(const char *filename)
{
	FILE *f = fopen(filename, "rb");
	if (f == NULL)
	{
		printf("Error: cannot open file %s\n", filename);
		goto bail;
	}

	char pal[32];
#define FREAD(dest, size)\
	if (fread(dest, size, 1, f) != 1) {\
		fclose(f);\
		goto bail;\
	}
	FREAD(pal, sizeof pal);
	// Convert to 0-255 range
	color_t palette[16];
	for (int i = 0; i < 16; i++)
	{
		// RGB stored in two bytes:
		// Lower byte is R
		// Upper byte contains GB (4 bits each)
		char b1 = pal[i * 2];
		char b2 = pal[i * 2 + 1];
		int r = b1;
		int g = b2 >> 4;
		int b = b2 & 0x0f;
		// Colours are actually 3-bits, giving a range of 0-7
		// Max brightness is actually based on 6-bit depth (i.e. max 63); this
		// was done by multiplying by 8
		// Now we convert to 0-255 range, which is an additional multiply by 4
		// Therefore we multiply by 32
		const int gamma = 32;
		palette[i].r = r * gamma;
		palette[i].g = g * gamma;
		palette[i].b = b * gamma;
	}

	// 200 pictures in Magus
	for (int i = 0; i < 200; i++)
	{
		uint16_t size;
		FREAD(&size, sizeof size);
		if (size == 0)
		{
			continue;
		}
		uint16_t w;
		FREAD(&w, sizeof w);
		w++;
		uint16_t h;
		FREAD(&h, sizeof h);
		h++;
		PicPaletted *pic = malloc(2 + 2 + w * h);
		pic->w = w;
		pic->h = h;

		// Pixels stored in planar mode, so convert from that to regular pixels
		char *buf = malloc(size - 4);
		FREAD(buf, size - 4);
		int planeSize = (pic->w + 7) / 8;
		for (int x = 0; x < pic->w; x++)
		{
			for (int y = 0; y < pic->h; y++)
			{
				int result = 0;
				int rowStartsAt = (y * planeSize * 8) / 2;
				int byteId = x / 8;
				int shift = 7 - x % 8;
				// There are 4 planes; iterate right to left
				for (int pnum = 3; pnum >= 0; pnum--)
				{
					result <<= 1;
					int index = rowStartsAt + (pnum * planeSize) + byteId;
					result |= (buf[index] >> shift) & 0x01;
				}
				pic->data[x + y*pic->w] = result;
			}
		}
		free(buf);

		PicPalettedToBMP(pic, palette, filename, i, 1);
		free(pic);
	}

bail:
#undef FREAD
	if (f != NULL)
	{
		fclose(f);
	}
}

static void PicPalettedToBMP(
	const PicPaletted *pic, const TPalette palette,
	const char *filename, const int i, const int gamma)
{
	bmpfile_t *bmp = bmp_create(pic->w, pic->h, 32);
	char outName[256];
	for (int j = 0; j < pic->w * pic->h; j++)
	{
		int x = j % pic->w;
		int y = j / pic->w;
		rgb_pixel_t pixel;
		int paletteIndex = (int)pic->data[j];
		pixel.blue = (uint8_t)CLAMP(palette[paletteIndex].b * gamma, 0, 255);
		pixel.green = (uint8_t)CLAMP(palette[paletteIndex].g * gamma, 0, 255);
		pixel.red = (uint8_t)CLAMP(palette[paletteIndex].r * gamma, 0, 255);
		pixel.alpha = 255;
		bmp_set_pixel(bmp, x, y, pixel);
	}
	sprintf(outName, "%s%d.bmp", filename, i);
	bmp_save(bmp, outName);
	bmp_destroy(bmp);
}

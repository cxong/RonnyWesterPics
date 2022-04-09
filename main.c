#include "bmpfile.h"
#include "pic_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define CLAMP(v, _min, _max) MAX((_min), MIN((_max), (v)))

#ifndef strcasecmp
#include <ctype.h>
// https://stackoverflow.com/a/30734030/2038264
int strcasecmp(const char *a, const char *b)
{
	int ca, cb;
	do {
		ca = (unsigned char) *a++;
		cb = (unsigned char) *b++;
		ca = tolower(toupper(ca));
		cb = tolower(toupper(cb));
	} while (ca == cb && ca != '\0');
	return ca - cb;
}
#endif

TPalette palette;

static void PicPalettedToBMP(
	const PicPaletted *pic, const TPalette palette,
	const char *filename, const int i, const int gamma);

int ReadPicsFile(const char *filename, const bool hasPalette)
{
	PicPaletted **pics = calloc(9999, sizeof(PicPaletted *));
	const int picsRead = ReadPics(filename, pics, 9999, hasPalette ? &palette : NULL);
	// Create a picture containing the palette too
	PicPaletted *palettePic = malloc(2 + 2 + 256);
	palettePic->w = palettePic->h = 16;
	for (int i = 0; i < 256; i++)
	{
		palettePic->data[i] = (unsigned char)i;
	}
	char buf[256];
	sprintf(buf, "%s.palette", filename);
	PicPalettedToBMP(palettePic, palette, buf, 0, 4);
	free(palettePic);
	for (int i = 0; i < picsRead; i++)
	{
		PicPaletted *pic = pics[i];
		if (pic != NULL)
		{
			PicPalettedToBMP(pic, palette, filename, i, 4);
		}
		free(pic);
	}
	free(pics);
	return picsRead;
}

void ReadMagusArt(
	const char *filename, color_t palette[16], PicPaletted *pics[200]);
void ReadMagusWorld(
	const char *filename,
	const color_t palette[16], const PicPaletted *pics[200]);

int main(int argc, char *argv[])
{
	color_t magusPalette[16];
	PicPaletted *magusPics[200];
	memset(magusPics, 0, sizeof magusPics);
	for (int i = 1; i < argc; i++)
	{
		printf("Reading %s...", argv[i]);
		if (strcasecmp(argv[i] + strlen(argv[i]) - 4, ".art") == 0)
		{
			// magus.art
			ReadMagusArt(argv[i], magusPalette, magusPics);
		}
		else if (strcasecmp(argv[i] + strlen(argv[i]) - 4, ".mgs") == 0)
		{
			// world.mgs
			ReadMagusWorld(argv[i], magusPalette, magusPics);
		}
		else
		{
			// Cyberdogs/C-Dogs px
			const int read = ReadPicsFile(argv[i], i == 1);
			printf("read %d\n", read);
		}
	}
	for (int i = 0; i < 200; i++)
	{
		free(magusPics[i]);
	}
	return 0;
}

void ReadMagusArt(
	const char *filename, color_t palette[16], PicPaletted *pics[200])
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
		pics[i] = malloc(2 + 2 + w * h);
		pics[i]->w = w;
		pics[i]->h = h;

		// Pixels stored in planar mode, so convert from that to regular pixels
		char *buf = malloc(size - 4);
		FREAD(buf, size - 4);
		int planeSize = (pics[i]->w + 7) / 8;
		for (int x = 0; x < pics[i]->w; x++)
		{
			for (int y = 0; y < pics[i]->h; y++)
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
				pics[i]->data[x + y*pics[i]->w] = result;
			}
		}
		free(buf);

		PicPalettedToBMP(pics[i], palette, filename, i, 1);
	}

bail:
#undef FREAD
	if (f != NULL)
	{
		fclose(f);
	}
}

void ReadMagusWorld(
	const char *filename,
	const color_t palette[16], const PicPaletted *pics[200])
{
	PicPaletted *pic = NULL;
	FILE *f = fopen(filename, "rb");
	if (f == NULL)
	{
		printf("Error: cannot open file %s\n", filename);
		goto bail;
	}

	const int tw = 24;
	const int th = 21;
	const int w = 200;
	const int h = 320;
	const int mw = w * tw;
	const int mh = h * th;
	pic = malloc(2 + 2 + mw * mh);
	pic->w = mw;
	pic->h = mh;
#define FREAD(dest, size)\
	if (fread(dest, size, 1, f) != 1) {\
		fclose(f);\
		goto bail;\
	}
	for (int x = 0; x < w; x++)
	{
		for (int y = 0; y < h; y++)
		{
			char flags;
			FREAD(&flags, 1);
			char sprite;
			FREAD(&sprite, 1);
			// Copy pixels over, one row of pixels at a time
			const int px = x * tw;
			const int py = y * th;
			for (int row = 0; row < th; row++)
			{
				memcpy(
					pic->data + px + (py + row) * mw,
					pics[sprite]->data + row * tw,
					tw);
			}
		}
	}

	PicPalettedToBMP(pic, palette, filename, 0, 1);

bail:
#undef FREAD
	if (f != NULL)
	{
		fclose(f);
	}
	free(pic);
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

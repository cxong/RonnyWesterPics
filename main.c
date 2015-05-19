#include "bmpfile.h"
#include "pic_file.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define CLAMP(v, _min, _max) MAX((_min), MIN((_max), (v)))

void ReadPicsFile(const char *filename)
{
	PicPaletted **pics = calloc(9999, sizeof(PicPaletted *));
	TPalette palette;
	int i;
	int picsRead = ReadPics(filename, pics, 9999, palette);
	for (i = 0; i < picsRead; i++)
	{
		PicPaletted *pic = pics[i];
		if (pic != NULL)
		{
			bmpfile_t *bmp = bmp_create(pic->w, pic->h, 8);
			int j;
			char outName[256];
			for (j = 0; j < pic->w * pic->h; j++)
			{
				int x = j % pic->w;
				int y = j / pic->w;
				rgb_pixel_t pixel;
				int paletteIndex = *((char *)&pic->data + j);
				int gamma = 4;
				pixel.blue = (uint8_t)CLAMP(palette[paletteIndex].blue * gamma, 0, 255);
				pixel.green = (uint8_t)CLAMP(palette[paletteIndex].green * gamma, 0, 255);
				pixel.red = (uint8_t)CLAMP(palette[paletteIndex].red * gamma, 0, 255);
				pixel.alpha = 255;
				bmp_set_pixel(bmp, x, y, pixel);
			}
			sprintf(outName, "%s%d.bmp", filename, i);
			bmp_save(bmp, outName);
			bmp_destroy(bmp);
		}
	}
}

int main(int argc, char *argv[])
{
	for (int i = 1; i < argc; i++)
	{
		ReadPicsFile(argv[i]);
	}
	return 0;
}

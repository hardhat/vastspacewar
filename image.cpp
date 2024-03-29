/* image - loads, saves, and swizzles images */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libpng16/png.h>

#ifdef _PSP
#include <pspgu.h>
#else
#define GU_PSM_5650 (0)
#define GU_PSM_8888 (3)
#define GU_PSM_T4 (4)
#define GU_PSM_T8 (5)

#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <SDL/SDL.h>
#endif

#include "main.h"
//#define Color unsigned long

#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
int imageRamAlloc = 0;
void freeVRam(void *address, int length);

static int getNextPower2(int width)
{
	int b = width;
	int n;
	for (n = 0; b != 0; n++) b >>= 1;
	b = 1 << n;
	if (b == 2 * width) b >>= 1;
	return b;
}

static void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg)
{
	printf("PNGERROR: %s\n", warning_msg);
}

Image *newImage(int width, int height)
{
	Image *image = (Image *)calloc(sizeof(Image),1);
	image->format = GU_PSM_8888;
	image->imageWidth = width;
	image->textureWidth = 512;
	while((image->textureWidth >> 1) >= width) image->textureWidth >>= 1;
	image->imageHeight = height;
	image->textureHeight = 512;
	while((image->textureHeight >> 1) >= height) image->textureHeight >>= 1;

	image->data = (Color *)malloc(image->imageHeight * image->textureWidth * 4);
	imageRamAlloc += image->imageHeight * image->textureWidth * 4;
//printf("NEWImage ram usage: %.4f MB\n",imageRamAlloc/(1024.0f*1024.0f));

	return image;
}

Image* loadPng(const char* filename)
{
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type, x, y;
	unsigned int* line;
	FILE *fp;
	Image* image = (Image*) malloc(sizeof(Image));
	if (!image) return NULL;
	image->texid = 0;
	image->isSwizzled = 0;
	image->vram = 0;
	image->palette = 0;
	image->format = GU_PSM_8888;

	//printf("Loading image '%s'\n",filename);

	if ((fp = fopen(filename, "rb")) == NULL) return NULL;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		free(image);
		fclose(fp);
		printf("Couldn't load %s (%08lx)\n", filename, (long unsigned int)(intptr_t)image);
		return NULL;
	}
	png_set_error_fn(png_ptr, (png_voidp) NULL, (png_error_ptr) NULL, user_warning_fn);
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		printf("Couldn't load %s (%08lx)\n", filename, (long unsigned int)(intptr_t)image);
		return NULL;
	}
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, sig_read);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
#ifdef _PSP
	if (width > 512 || height > 512) {
#else
	if (width > 2048 || height > 2048) {
#endif
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		printf("Couldn't load %s (%08lx)\n", filename, (long unsigned int)(intptr_t)image);
		return NULL;
	}
	image->imageWidth = width;
	image->imageHeight = height;
	image->textureWidth = getNextPower2(width);
	image->textureHeight = getNextPower2(height);
	png_set_strip_16(png_ptr);
	png_set_packing(png_ptr);
	if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
#ifdef _PSP
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_gray_1_2_4_to_8(png_ptr);
#else
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
#endif
	if (color_type == PNG_COLOR_TYPE_GRAY) png_set_gray_to_rgb(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
#ifdef _PSP
	image->data = (Color*) malloc( image->textureWidth * image->imageHeight * sizeof(Color));
#else
	image->data = (Color*) malloc( image->textureWidth * image->textureHeight * sizeof(Color));
#endif

	if (!image->data) {
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		printf("Couldn't load %s (%08lx)\n", filename, (long unsigned int)(intptr_t)image);
		return NULL;
	}
	imageRamAlloc += image->imageHeight * image->textureWidth * 4;
//printf("LOADImage ram usage: %.4f MB\n",imageRamAlloc/(1024.0f*1024.0f));
	line = (unsigned int *) malloc(width * 4);
	if (!line) {
		free(image->data);
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		printf("Couldn't load %s (%08lx)\n", filename, (long unsigned int)(intptr_t)image);
		return NULL;
	}
	for (y = 0; y < (int)height; y++) {
		png_read_row(png_ptr, (unsigned char *) line, NULL);
		for (x = 0; x < (int)width; x++) {
			unsigned int color = line[x];
			image->data[x + y * image->textureWidth] = color;
		}
	}
	free(line);
	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);
	printf("Loaded %s (%08lx)\n", filename, (long unsigned int)(intptr_t)image);
	return image;
}

#define NTEX 128
int texid[NTEX];
int nextId = 0;

void initImage()
{
#ifndef _PSP
	glGenTextures(NTEX, (GLuint *)texid);      // Hard coded.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif
}

int uploadImage(Image *image)
{
	if(!image) {
		printf("*** Missing image.\n");
		return 0;
	}
	if(nextId >= NTEX) {
		printf("*** Out of texids.\n");
		return 0;
	}
#ifndef _PSP
	int i = nextId++;
	glBindTexture(GL_TEXTURE_2D, texid[i]);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->textureWidth, image->textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->data);
	image->texid = texid[i];
	return texid[i];
#else
	return 0;
#endif
}

void freeImage(Image *image)
{
	if(!image) return;
	if(image->data && image->vram == 0) {
		free(image->data);
		imageRamAlloc -= image->imageHeight * image->textureWidth * 4;
//printf("FREEImage ram usage: %.4f MB\n",imageRamAlloc/(1024.0f*1024.0f));
	} else if(image->data && image->vram) {
		freeVRam(image->data, image->imageHeight * image->textureWidth * 4);
	}
	image->data = 0;
	if(image->texid) {
		// should put it back on the list of available texture ids.
	}
	free(image);
}

void saveImagePng(const char* filename, Color* data, int width, int height, int lineSize, int saveAlpha)
{
	png_structp png_ptr;
	png_infop info_ptr;
	FILE* fp;
	int i, x, y;
	unsigned char* line;

	if ((fp = fopen(filename, "wb")) == NULL) return;
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) return;
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		return;
	}
	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr, width, height, 8,
		saveAlpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	line = (unsigned char *) malloc(width * (saveAlpha ? 4 : 3));
	for (y = 0; y < height; y++) {
		for (i = 0, x = 0; x < width; x++) {
			Color color = data[x + y * lineSize];
			unsigned char r = color & 0xff;
			unsigned char g = (color >> 8) & 0xff;
			unsigned char b = (color >> 16) & 0xff;
			unsigned char a = saveAlpha ? (color >> 24) & 0xff : 0xff;
			line[i++] = r;
			line[i++] = g;
			line[i++] = b;
			if (saveAlpha) line[i++] = a;
		}
		png_write_row(png_ptr, line);
	}
	free(line);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	fclose(fp);
}

int swizzleToVRam = 1;
//unsigned char *nextVRam=(unsigned char *)0x04000000+0x154000;	// after depth buffer
int availableVRam = 0x200000 - 0x154000;
int biggestVRam = 0;
struct VRamBlock {
	struct VRamBlock *next,*prev;
	unsigned char *addr;
	int length;
} *vramBlock = 0;

void reportVRam()
{
	struct VRamBlock *b;
	int i = 0;

	for(b = vramBlock; b != 0; b = b->next) {
		i++;
	}
	printf("There is %d VRAM available in %d blocks (biggest is %d)\n", availableVRam, i, biggestVRam);
}

void resetVRam()
{
	unsigned char *nextVRam = (unsigned char *)0x04000000 + 0x154000;	// after depth buffer
	availableVRam = 0x200000 - 0x154000;
	vramBlock = (struct VRamBlock *)malloc(sizeof(struct VRamBlock));
	vramBlock->next = 0;
	vramBlock->addr = nextVRam;
	vramBlock->length = availableVRam;
	biggestVRam = 0x200000 - 0x154000;
	printf("reset: "); reportVRam();
}

void *allocVRam(int length)
{
	struct VRamBlock *b;
	struct VRamBlock *biggest = NULL, *closest = NULL, *alloc = NULL;
	if(biggestVRam < length) return 0;	// common case is faster.
	for(b = vramBlock; b != NULL; b = b->next) {
		if(biggest ==0 || biggest->length < b->length) biggest = b;
		if(b->length >= length) {
			if(closest == 0 || closest->length > b->length) closest = b;
		}
	}
	if(closest && closest->length == length) {
		// match, so allocate and return.
		alloc = closest;
	}
	if(biggest && biggest->length >= length) {
		alloc = biggest;
	}
	if(alloc) {
		unsigned char *out = (unsigned char*)alloc->addr;
		if(length == alloc->length) {
			// delete node.
			if(alloc->prev) alloc->prev->next = alloc->next;
			if(alloc->next) alloc->next->prev = alloc->prev;
			if(alloc->prev == 0) vramBlock = alloc->next;
			free(alloc);
		} else {
			// alter node
			alloc->length -= length;
			alloc->addr += length;
		}
		biggestVRam = 0;
		availableVRam = 0;
		for(b = vramBlock; b != NULL; b = b->next) {
			if(b->length > biggestVRam) biggestVRam = b->length;
			availableVRam += b->length;
		}
		printf("alloc: "); reportVRam();
		return out;
	}
	printf("ASSERT COULDN'T ALLOC: "); reportVRam();
	return 0;
}

void freeVRam(void *address, int length)
{
	struct VRamBlock *b, *curr = 0;

	for(b = vramBlock; b != 0; b = b->next) {
		if(b->addr + b->length == address) {
			b->length += length;
			curr = b;
			break;
		}
	}
	if(!curr) {
		// new node before the next higher address.
		curr = (struct VRamBlock *)malloc(sizeof(struct VRamBlock));
		curr->next = 0;
		curr->prev = 0;
		curr->addr = (unsigned char *)address;
		curr->length = length;
		for(b = vramBlock; b != 0; b = b->next) {
			if((intptr_t)b->addr>(intptr_t)address) {
				curr->prev = b->prev;
				curr->next = b;
				b->prev = curr;
				if(curr->prev) {
					curr->prev->next = curr;
				} else {
					vramBlock = curr;
				}
			}
		}
		if(vramBlock == 0) vramBlock = curr;
	}
	if(curr->next && curr->next->addr == curr->addr + curr->length) {
		// merge blocks.
		b = curr->next;
		curr->length += b->length;
		curr->next = b->next;
		if(curr->next) curr->next->prev = curr;
		free(b);
	}
	biggestVRam = 0;
	availableVRam = 0;
	for(b = vramBlock; b != NULL; b = b->next) {
		if(b->length>biggestVRam) biggestVRam = b->length;

		availableVRam += b->length;
	}
	printf("free: "); reportVRam();
}

void swizzleFast(Image *source)
{
	if(source == 0) return;
#ifdef _PSP
	unsigned int width = source->textureWidth * 4;
	unsigned int height = source->imageHeight;
	unsigned long* out;

	if(swizzleToVRam && (out = (unsigned long*)allocVRam(width * height))) {
		printf("texture to vram\n");
		source->vram = 1;
	} else {
		out = (unsigned long *)malloc(width * height);
		if(!out) return;	// couldn't do it!
		imageRamAlloc += width * height;
//printf("SWIZ^Image ram usage: %.4f MB\n",imageRamAlloc/(1024.0f*1024.0f));
	}
	unsigned int blockx, blocky;
	int i;

	unsigned int widthBlocks = (width / 16);
	unsigned int heightBlocks = (height / 8);

	unsigned int srcPitch = (width - 16)/4;
	unsigned int srcRow = width * 8;

	const unsigned char* ysrc = (unsigned char *)source->data;;
	unsigned long * dst = out;

	for (blocky = 0; blocky < heightBlocks; ++blocky) {
		const unsigned char* xsrc = ysrc;
		for (blockx = 0; blockx < widthBlocks; ++blockx) {
			const unsigned long* src = (unsigned long*)xsrc;
			for (i = 0; i < 8; i++) {
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				src += srcPitch;
			}
			xsrc += 16;
		}
		ysrc += srcRow;
	}
	free(source->data);
	imageRamAlloc -= width * height;
//printf("SWIZvImage ram usage: %.4f MB\n",imageRamAlloc/(1024.0f*1024.0f));
	source->data=(Color *)out;
	source->isSwizzled = 1;
#endif
}

/* tga header */
typedef struct
{
  unsigned char id_length;          /* size of image id */
  unsigned char colormap_type;      /* 1 is has a colormap */
  unsigned char image_type;         /* compression type */

  short	cm_first_entry;       /* colormap origin */
  short	cm_length;            /* colormap length */
  unsigned char cm_size;            /* colormap size */

  short	x_origin;             /* bottom left x coord origin */
  short	y_origin;             /* bottom left y coord origin */

  short	width;                /* picture width (in pixels) */
  short	height;               /* picture height (in pixels) */

  unsigned char pixel_depth;        /* bits per pixel: 8, 16, 24 or 32 */
  unsigned char image_descriptor;   /* 24 bits = 0x00; 32 bits = 0x80 */

} tga_header_t;

void saveImageTarga(const char* filename, Color* data, int width, int height, int lineSize, int saveAlpha)
{
	FILE* fp;
	int x, y;
	tga_header_t head;

	if ((fp = fopen(filename, "wb")) == NULL) return;

	// Put the header.
	head.id_length = 0;
	fwrite(&head.id_length, 1, 1, fp);
	head.colormap_type = 0;
	fwrite(&head.colormap_type, 1, 1, fp);
	head.image_type = 2;
	fwrite(&head.image_type, 1, 1, fp);
	head.cm_first_entry = 0;
	fwrite(&head.cm_first_entry, 2, 1, fp);
	head.cm_length = 0;
	fwrite(&head.cm_length, 2, 1, fp);
	head.cm_size = 0;
	fwrite(&head.cm_size, 1, 1, fp);
	head.x_origin = 0;
	fwrite(&head.x_origin, 2, 1, fp);
	head.y_origin = 0;
	fwrite(&head.y_origin, 2, 1, fp);
	head.width = width;
	fwrite(&head.width, 2, 1, fp);
	head.height = height;
	fwrite(&head.height, 2, 1, fp);
	head.pixel_depth = 24;
	fwrite(&head.pixel_depth, 1, 1, fp);
	head.image_descriptor = 0x00;
	fwrite(&head.image_descriptor, 1, 1, fp);

	for(y = height - 1; y >= 0; y--) {
		for(x = 0; x < width; x++) {
			fwrite((char *)(data + y * lineSize + x) + 2, 1, 1, fp);
			fwrite((char *)(data + y * lineSize + x) + 1, 1, 1, fp);
			fwrite((char *)(data + y * lineSize + x) + 0, 1, 1, fp);
		}
	}

	fclose(fp);
}

#if 0
struct FastFont {
	Image *texture;
	int height;
	struct Glyph {
		int active;
		int x, y, w, h;
		int kern_left, kern_right, kern_bottom;
	} glyph[256];
} fastFont[6];

int loadFastFont(struct FastFont *font, const char *filename)
{
	font->height = 1;
	printf("Loading font '%s'\n", filename);
	if(font->texture) return 1;	// already loaded.
	FILE *file = fopen(filename, "r");
	if(!file) {
		font->texture = 0;
		return 0;
	}
	font->texture = 0;
	char line[256];
	line[255]=0;
	while( fgets(line, 255, file) ) {
		if(strncmp(line, "Char=", 5) == 0) {
			int ch = 0;
			int x = 0, y = 0, w = 0, h = 0, kern_left = 0, kern_right = 0, kern_bottom = 0;
			// Char="z"; or Char=0020;
			char c = 0;
			int res = sscanf(line, "Char=\"%c\"; %d,%d,%d,%d; %d,%d,%d;", &c, &x, &y, &w, &h, &kern_left, &kern_right, &kern_bottom);
			ch = c;
			if(res < 5) {
				res = sscanf(line, "Char=%x; %d,%d,%d,%d; %d,%d,%d;", &ch, &x, &y, &w, &h, &kern_left, &kern_right, &kern_bottom);
			}
			if(res >= 5) {
				font->glyph[(unsigned int)ch].active = 1;
				font->glyph[(unsigned int)ch].x = x;
				font->glyph[(unsigned int)ch].y = y;
				font->glyph[(unsigned int)ch].w = w;
				font->glyph[(unsigned int)ch].h = h;
				font->glyph[(unsigned int)ch].kern_left = kern_left;
				font->glyph[(unsigned int)ch].kern_right = kern_right;
				font->glyph[(unsigned int)ch].kern_bottom = kern_bottom;
			}
		} else if(strncmp(line, "Height=", 7) == 0) {
			font->height = 1;
			sscanf(line, "Height=%d", &font->height);
		} else if(strncmp(line, "Bitmap=", 7) == 0) {
			char imagePath[256];
			if(strstr(line, "\n")) strstr(line, "\n")[0] = 0;
			if(strstr(line, "\r")) strstr(line, "\r")[0] = 0;
			strcpy(imagePath, line + 7);
			font->texture = loadPng(imagePath);
			if(!font->texture) {
				strcpy(imagePath, "data/");
				strcat(imagePath, line + 7);
				font->texture = loadPng(imagePath);
			}
#ifdef _PSP
			swizzleFast(font->texture);
#endif
			if(font->texture) printf("Loaded %s\n", imagePath);
			else printf("Could not find '%s'\n", imagePath);
		}
	}
	if(!font->texture) return 0;
	printf("Success for font '%s'\n", filename);
	return 1;
}

void initFastFont()
{
	int oldVRam = swizzleToVRam;
	swizzleToVRam = 0;
	loadFastFont(&fastFont[FONT_HEADLINE], "data/Headline32bluered.fnt");
	loadFastFont(&fastFont[FONT_BODY], "data/Body20blue.fnt");
	loadFastFont(&fastFont[FONT_BODYHIGHLIGHT], "data/Body20brown.fnt");
	loadFastFont(&fastFont[FONT_MESSAGE], "data/Body14blue.fnt");
	loadFastFont(&fastFont[FONT_SMALL], "data/Small10red.fnt");
	loadFastFont(&fastFont[FONT_SMALLHIGHLIGHT], "data/Small10black.fnt");
	swizzleToVRam = oldVRam;
}

void extentMessage(int *w,int *h, enum FontId fontId, const char *message)
{
	struct FastFont *font = &fastFont[fontId];
	unsigned int i;
	if(!font->texture) return;
	*w = 0;
	for(i = 0; i < strlen(message); i++) {
		int ch = (unsigned char)message[i];
		if(font->glyph[ch].active) {
			*w += font->glyph[ch].w + font->glyph[ch].kern_right + 1;
		} else {
			printf("Missing font char: '%c'\n", (char)ch);
			*w += font->height;
		}
	}
	*h = fastFont[fontId].height;
}

void drawMessage(int x,int y, enum FontId fontId, const char *message)
{
	struct FastFont *font = &fastFont[fontId];
	unsigned int i;
	if(!font->texture) return;
	//printf("Message '%s'\n",message);
	for(i = 0; i < strlen(message); i++) {
		int ch = (unsigned char)message[i];
		if(font->glyph[ch].active) {
			drawSprite(font->glyph[ch].x,
				font->glyph[ch].y,font->glyph[ch].w,
				font->glyph[ch].h,font->texture,
				x - font->glyph[ch].kern_left,
				y + font->glyph[ch].kern_bottom);
			x += font->glyph[ch].w + font->glyph[ch].kern_right + 1;
		} else {
			printf("Missing font char: '%c'\n", (char)ch);
			x += font->height;
		}
	}
	//printf("Done '%s'\n",message);
}

void drawMessageAlpha(int x, int y, enum FontId fontId, const char *message, int alpha)
{
	struct FastFont *font = &fastFont[fontId];
	unsigned int i;
	if(!font->texture) return;
	//printf("Message '%s'\n",message);
	for(i = 0; i < strlen(message); i++) {
		int ch = (unsigned char)message[i];
		if(font->glyph[ch].active) {
			drawSpriteAlpha(font->glyph[ch].x,
				font->glyph[ch].y, font->glyph[ch].w,
				font->glyph[ch].h, font->texture,
				x + font->glyph[ch].kern_left, alpha);
				y + font->glyph[ch].kern_bottom, alpha); //sleepy changed, was "y+font->glyph[ch].kern_bottom,"
				x += font->glyph[ch].w + font->glyph[ch].kern_right + 1;
		} else {
			printf("Missing font char: '%c'\n", (char)ch);
			x += font->height;
		}
	}
	//printf("Done '%s'\n", message);
}

void drawMessageFormat(int x, int y, enum FontId fontId, const char *message)
{
	char *s = strdup(message);
	char *freeable = s;
	int i;
	int max = strlen(s);
	char *msg = s;
	int line = 0;

	for(i = 0; i < max; i++) {
		if(s[i] == '\n') {
			s[i] = 0;
			drawMessage(x, y + line * fastFont[fontId].height, fontId, msg);
			line++;
			msg = &s[i + 1];
		}
	}
	if(msg[0] != 0) {
		drawMessage(x, y + line * fastFont[fontId].height, fontId, msg);
	}

	free(freeable);
}

struct ImageCache {
	char fname[256];
	Image *image;
} cell[32];

int cellNext = 0;

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272

Image *loadCell(const char *fname)
{
	Image *image = 0;
	int i;

	for(i = 0; i < cellNext; i++) {
		if(strcmp(cell[i].fname, fname) ==0) {
			image = cell[i].image;
			return image;
		}
	}

	if(!image) {
		image = loadPng(fname);
		if(!image) {
			char buf[256];
			strcpy(buf, "data/");
			strcat(buf, fname);
			image = loadPng(buf);
		}
		if(image) {
			strcpy(cell[cellNext].fname, fname);
			cell[cellNext++].image = image;
		}
	}
	if(!image) printf("Could not load cell '%s'\n", fname);
	if(!image) return 0;
	return image;
}

void drawCell(int x, int y, const char *fname)
{
	Image *image = loadCell(fname);
	if(!image) return;
	printf("Drawing cell '%s', loaded successfully\n", fname);
	if(x + image->imageWidth < 0 || y + image->imageHeight < 0 || x > SCREEN_WIDTH || y > SCREEN_HEIGHT) return;

	drawSprite(0, 0, image->imageWidth, image->imageHeight, image, x, y);
}

void freeCells()
{
	int i;
	for(i = 0; i < cellNext; i++) {
		freeImage(cell[i].image);
		cell[i].image = 0;
		cell[i].fname[0] = 0;
	}
	cellNext = 0;
}
#endif


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "font.h"
#include "main.h"

int round2(double x){
	return (int)(x + 0.5);
}

int nextpoweroftwo(int x) {
	double logbase2 = log((float)x) / log(2.0f);
	return round2(pow(2, ceil(logbase2)));
}

CFontManager *CFontManager::m_Singleton = 0;

CFontManager &CFontManager::getManager(void) {

	if (m_Singleton == 0) {
		m_Singleton = new CFontManager;
		TTF_Init();
		atexit(TTF_Quit);

		const char * fontpath = "data/vera.ttf";

        m_Singleton->Font.Add(FONT_HEADLINE, fontpath, 18);
        m_Singleton->Font.Add(FONT_BODY, fontpath, 20);
        m_Singleton->Font.Add(FONT_BODYHIGHLIGHT, fontpath, 20);
        m_Singleton->Font.Add(FONT_MESSAGE, fontpath, 16);
        m_Singleton->Font.Add(FONT_SMALL, fontpath, 12);
        m_Singleton->Font.Add(FONT_SMALLHIGHLIGHT, fontpath, 12);
	}

	return *m_Singleton;
}

void CFontManager::Add(enum FontID fontId, const char *filepath, int size){

     TTF_Font * font = TTF_OpenFont(filepath, size);
     if(!font)
		printf("Error loading font: %s", TTF_GetError());
     else
        printf("Loaded %d size font '%s'\n", size, filepath);

     FontMap[fontId] = font;
}

void CFontManager::drawMessage(const char *text, enum FontID fontId, SDL_Color color, SDL_Rect *location){

	TTF_Font *font = FontMap[fontId];
	SDL_Surface *initial = 0;
	SDL_Surface *intermediary = 0;
	//SDL_Rect rect;
	int w, h;
	int texture;

	/* Use SDL_TTF to render our text */
	initial = TTF_RenderText_Blended(font, text, color);

	/* Convert the rendered text to a known format */
	w = nextpoweroftwo(initial->w);
	h = nextpoweroftwo(initial->h);
	Uint32 rmask, gmask, bmask, amask;

#if 1
	//LITTLE ENDIAN
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
	SDL_SetColorKey(initial, SDL_TRUE, SDL_MapRGB(initial->format, 0, 0, 0));

	intermediary = SDL_CreateRGBSurface(0, w, h, 32,
			rmask, gmask, bmask, amask);

	SDL_BlitSurface(initial, 0, intermediary, 0);
#if 0
	SDL_LockSurface(intermediary);
	int i, j;
	unsigned int raw;
	for(j = 0; j < intermediary->h; j++) {
		for(i = 0; i < intermediary->w; i++) {
			raw = ((unsigned int *)intermediary->pixels)[i + intermediary->pitch / 4 * j];
			SDL_PixelFormat *fmt = intermediary->format;
			/* Get Red component */
			unsigned int temp = raw & fmt->Rmask;  /* Isolate red component */
			temp = temp >> fmt->Rshift; /* Shift it down to 8-bit */
			temp = temp << fmt->Rloss;  /* Expand to a full 8-bit number */
			unsigned int red = (Uint8)temp;
			raw = raw& ~fmt->Amask;
			raw| = red << fmt->Ashift;
			((unsigned int *)intermediary->pixels)[i + intermediary->pitch / 4 * j] = raw;
		}
	}
	SDL_UnlockSurface(intermediary);

	/* Tell GL about our new texture */
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
	glGenTextures(1, (GLuint*)&texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, intermediary->pixels );
#else
	SDL_LockSurface(initial);
	unsigned char *raw = new unsigned char[w * h];
	memset(raw, 0, w * h);
	int i, j;
	for(j = 0; j < initial->h; j++) {
		for(i = 0; i < initial->w; i++) {
			if(initial && initial->format && initial->format->palette && initial->format->palette->colors) {
				raw[i + j * w] = initial->format->palette->colors[((unsigned char *)initial->pixels)[i + initial->pitch * j]].r;
			} else {
				raw[i + j * w] = ((unsigned char *)initial->pixels)[i + initial->pitch * j];
			}
		}
	}
	SDL_UnlockSurface(initial);

	/* Tell GL about our new texture */
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glGenTextures(1, (GLuint*)&texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, 1, w, h, 0, GL_LUMINANCE,
			GL_UNSIGNED_BYTE, raw );
	delete [] raw;
#endif

	/* GL_NEAREST looks horrible, if scaled... */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	/* prepare to render our texture */
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
	glColor3f(1.0f, 1.0f, 1.0f);

	/* Draw a quad at location */
	glBegin(GL_QUADS);
		/* Recall that the origin is in the lower-left corner
		   That is why the TexCoords specify different corners
		   than the Vertex coors seem to. */
		glTexCoord2f(0.0f, 0.0f);
			glVertex2f((float)location->x    , (float)location->y);
			glTexCoord2f(1.0f, 0.0f);
			glVertex2f((float)location->x + w, (float)location->y);
			glTexCoord2f(1.0f, 1.0f);
			glVertex2f((float)location->x + w, (float)location->y + h);
			glTexCoord2f(0.0f, 1.0f);
			glVertex2f((float)location->x    , (float)location->y + h);
	glEnd();

	/* Bad things happen if we delete the texture before it finishes */
	glFinish();

	/* return the deltas in the unused w,h part of the rect */
	location->w = initial->w;
	location->h = initial->h;

	/* Clean up */
	SDL_FreeSurface(initial);
	if(intermediary) SDL_FreeSurface(intermediary);
	glDeleteTextures(1, (GLuint*)&texture);
}

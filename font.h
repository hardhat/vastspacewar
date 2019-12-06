#ifndef PCT_FONT_H
#define PCT_FONT_H

#define Font	CFontManager::getManager()

#include <map>

//#include <SDL/SDL.h>
#include <SDL2/SDL_ttf.h>

#if 1

#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#endif // 0

enum FontID {
	FONT_HEADLINE, FONT_BODY, FONT_BODYHIGHLIGHT, FONT_MESSAGE, FONT_SMALL,
    FONT_SMALLHIGHLIGHT
};

class CFontManager
{
	public:
		static class CFontManager *m_Singleton;
		static CFontManager &getManager(void);

	private:
		std::map < enum FontID, TTF_Font*> FontMap;

	public:
		void Add(enum FontID fontId, const char *filepath, int size);
		void drawMessage(const char *text, enum FontID fontId, SDL_Color color, SDL_Rect *location);
};

#endif

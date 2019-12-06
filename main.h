/* Main */
#ifndef MAIN_H
#define MAIN_H
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef _PSP
#include <pspgum.h>
#else
#ifndef ScePspFVector3
typedef struct ScePspFVector3 { float x, y, z; } ScePspFVector3;
typedef struct ScePspFVector4 { float x, y, z, w; } ScePspFVector4;
#endif
typedef struct ScePspFMatrix4 { ScePspFVector4 x, y, z, w; } ScePspFMatrix4;
#endif

// main.c
struct Vertex3DCNP {
	unsigned long color;
	float nx, ny, nz;
	float x, y, z;
};
struct Vertex3DTNP {
	float u, v;
	float nx, ny, nz;
	float x, y, z;
};
struct Vertex3DCP {
	unsigned long color;
	float x, y, z;
};
struct Vertex3DTCP {
	float u, v;
	unsigned long color;
	float x, y, z;
};
struct Vertex3DTCNP {
	float u, v;
	unsigned long color;
	float nx, ny, nz;
	float x, y, z;
};
struct Vertex3DTP {
	float u, v;
	float x, y, z;
};
struct Vertex3DTPfast {
	short u, v;
	short x, y, z;
};
struct Vertex3DTfast {
	short u, v;
};
struct Vertex3DPfast {
	short x, y, z;
};

class Camera
{
public:
	ScePspFVector3 from, to, up;
	int width, height;
	/// basic camera with default position
	Camera();
	/// send the gl command to do a lookat.
	void reposition();
	/// go into ortho2d mode
	void hudBegin();
	void hudEnd();
private:
	int hudDepth;
};
extern Camera camera;
#define TILESIZE 32

// image.c
typedef unsigned int Color;
typedef struct Image
{
        int textureWidth;  // the real width of data, 2^n with n>=0
        int textureHeight;  // the real height of data, 2^n with n>=0
        int imageWidth;  // the image width
        int imageHeight;	// the image height
        int isSwizzled;	// Is the image swizzled?
        int vram;		// Is the image in vram or not?
        int format;		// default is GU_COLOR_8888
        Color* data;
        Color* palette;	// used for 4 bpp and 8bpp modes.
        int texid;		// the texture ID for OpenGL.
} Image;
Image *loadPng(const char *filename);
int uploadImage(Image *image);
void freeImage(Image *image);
void resetVRam();
void reportVRam();
extern int swizzleToVRam;
void swizzleFast(Image *source);
void saveImagePng(const char* filename, Color* data, int width, int height, int lineSize, int saveAlpha);
void saveImageTarga(const char* filename, Color* data, int width, int height, int lineSize, int saveAlpha);
/*enum FontId {
	FONT_HEADLINE,FONT_BODY,FONT_BODYHIGHLIGHT,FONT_MESSAGE,FONT_SMALL,FONT_SMALLHIGHLIGHT
};
void initFastFont();
void extentMessage(int *w,int *h, enum FontId fontId, const char *message);
void drawMessage(int x,int y, enum FontId fontId, const char *message);
void drawMessageAlpha(int x,int y, enum FontId fontId, const char *message,int alpha);
void drawMessageFormat(int x,int y,enum FontId fontId, const char *message);
*/

Image *loadCell(const char *fname);
void drawCell(int x, int y, const char *fname);
void freeCells();
Image *newImage(int width, int height);
void initImage();

#endif

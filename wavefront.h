
/* Wavefront */
#ifndef WAVEFRONT_H
#define WAVEFRONT_H

// wavefront.c
struct WavefrontModel;
struct WavefrontModel *loadWavefront(const char *fname);
void freeWavefront(struct WavefrontModel *model);
void setWavefrontPos(struct WavefrontModel *model, float x, float y, float z);
void drawWavefront(struct WavefrontModel *model);
void drawWavefrontPartial(struct WavefrontModel *mod, int transparent);	// 0 = solid, 1 = trans, 3 = both
#endif

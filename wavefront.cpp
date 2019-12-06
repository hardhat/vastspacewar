
#include<math.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __APPLE__
#include<OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#ifdef _PSP
#include<pspctrl.h>
#include<pspgu.h>
#include<pspgum.h>
#endif
#include "main.h"
#include "wavefront.h"

struct Vertex3DT {
	float u, v; 
}; 
struct Vertex3DP {
	float x, y, z; 
}; 

struct Material {
	char name[64]; 
	unsigned long ambient; 
	unsigned long color; 	// diffuse
	unsigned long specular; 
	unsigned long emissive; 
	Image *image; 
	int useCount; 
}; 

struct MaterialGroup {
	int first; 	// vertex
	int last;  // vertex
	Image *image; 
	int transparent; 	// transparent things are rendered last.
}; 

struct WavefrontModel {
    char name[64]; 
    float matrix[16]; 
    struct MaterialGroup *group; 
    int groupCount; 
	struct Vertex3DTNP *vert; 
    int vertCount; 
	float min[3]; 
	float max[3]; 	// handy for collision detection.
}; 

struct WavefrontState {
	int face; 
	int faceMax; 
	struct Material *material; 
	int materialCount; 
	struct Material *currentMaterial; 
	struct Vertex3DP *position; 
	int positionCount; 
	int positionMax; 
	struct Vertex3DP *normal; 
	int normalCount; 
	int normalMax; 
	struct Vertex3DT *texture; 
	int textureCount; 
	int textureMax; 
}; 

void scanWavefront(FILE *file, int *vCount, int *tCount, int *nCount, int *fCount, int *gCount)
{
	size_t initialSeek = ftell(file); 
	int vc = 0, tc = 0, nc = 0, fc = 0, gc = 0; 
	char line[256]; 
	line[255] = 0; 
		
	while(fgets(line, 255, file)) {
		char *s = line; 
		while(s[0] == ' ' || s[0] == '\t') s++; 
		if(s[0] == 'f') {
			fc++; 
			int v[4] = {0, 0, 0, 0}, t[4] = {0, 0, 0, 0}, n[4] = {0, 0, 0, 0}; 
			int rc = sscanf(line, "f%d/%d/%d%d/%d/%d%d/%d/%d%d/%d/%d", &v[0], &t[0], &n[0], &v[1], &t[1], &n[1], &v[2], &t[2], &n[3], &v[3], &t[3], &n[3]); 
			if(rc == 12) fc++; 		// found a quad.
		} else if(s[0] == 'v') {
			if(s[1] == 't') tc++; 
			else if(s[1] == 'n') nc++; 
			else if(s[1] == ' ') vc++; 
		} else if(strncmp(s, "usemtl", 6) == 0) gc++; 
	}
	
	*vCount = vc; 
	*tCount = tc; 
	*nCount = nc; 
	*fCount = fc; 
	*gCount = gc; 
	if(gc == 0) *gCount = 1; 
	fseek(file, initialSeek, SEEK_SET); 
}

void loadMaterials(const char *fname, struct Material *material, int maxMaterial, int *materialCount)
{
	char path[256]; 
	//printf("read materials for '%s'\n", fname); 
	sprintf(path, "models/%s/%s.mtl", fname, fname); 
//	gzFile file = gzopen(path, "r"); 
	FILE *file = fopen(path, "r"); 
	*materialCount = 0; 
	int nextMaterial = 0; 
	if(!file) return; 
	
	// Read in the materials.
	char line[256]; 
	line[255] = 0; 
	int mat = nextMaterial; 

	while( fgets(line, 255, file) ) {
		char cmd[64]; 

		cmd[0] = 0; 
		sscanf(line, "%s", cmd); 
		if(strchr(line, '\r')) strchr(line, '\r')[0] = 0; 
		if(strchr(line, '\n')) strchr(line, '\n')[0] = 0; 

		if(strcmp(cmd, "newmtl") == 0 ) {
			mat = nextMaterial; 
			nextMaterial++; 
			strcpy(material[mat].name, line + 7); 
			material[mat].color = 0; 
			material[mat].image = 0; 
			material[mat].useCount = 0; 
			if(nextMaterial >= maxMaterial) {
				printf("*** Maximum materials exceeded.\n"); 
				break; 
			}
			//printf("Located material '%s'\n", material[mat].name); 
		} else if( strcmp(cmd, "Ka") == 0) {
			float rf = 0, gf = 0, bf = 0; 
			unsigned long r, g, b; 
			sscanf(line, "Ka%f%f%f", &rf, &gf, &bf); 
			r = (int)(rf * 255); 
			g = (int)(gf * 255); 
			b = (int)(bf * 255); 
			material[mat].ambient = (r)|(g << 8)|(b << 16)|(255 << 24); 
		} else if( strcmp(cmd, "Ks") == 0) {
			float rf = 0, gf = 0, bf = 0; 
			unsigned long r, g, b; 
			sscanf(line, "Ks%f%f%f", &rf, &gf, &bf); 
			r = (int)(rf * 255); 
			g = (int)(gf * 255); 
			b = (int)(bf * 255); 
			material[mat].specular = (r)|(g << 8)|(b << 16)|(255 << 24); 
		} else if( strcmp(cmd, "Kd") == 0) {
			float rf = 0, gf = 0, bf = 0; 
			unsigned long r, g, b; 
			sscanf(line, "Kd%f%f%f", &rf, &gf, &bf); 
			r = (int)(rf * 255); 
			g = (int)(gf * 255); 
			b = (int)(bf * 255); 
			material[mat].color = (r)|(g << 8)|(b << 16)|(255 << 24); 
		} else if( strcmp(cmd, "Ke") == 0) {
			float rf = 0, gf = 0, bf = 0; 
			unsigned long r, g, b; 
			sscanf(line, "Ke%f%f%f", &rf, &gf, &bf); 
			r = (int)(rf * 255); 
			g = (int)(gf * 255); 
			b = (int)(bf * 255); 
			material[mat].emissive = (r)|(g << 8)|(b << 16)|(255 << 24); 
		} else if( strcmp(cmd, "map_Kd") == 0) {
			char path[256]; 
			char *s = line; 
			while (s[0] == ' ' || s[0] == '\t') s++; 	// skip white space
			s += 6; 
			while(s[0] == ' ' || s[0] == '\t') s++; 		// skip white space
			if(strrchr(s, '\\')) s = strrchr(s, '\\') + 1; 
			if(strrchr(s, '/')) s = strrchr(s, '/') + 1; 
			sprintf(path, "models/%s/%s", fname, s); 
			material[mat].image = loadPng(path); 
			if(material[mat].image) {
				if(material[mat].image->textureWidth > 64 || material[mat].image->textureHeight > 64) swizzleToVRam = 1;  else swizzleToVRam = 0; 
				swizzleFast(material[mat].image); 
			}
			if(!material[mat].image) printf("Couldn't locate '%s'\n", path); 
		}
	}
	fclose(file); 
	printf("read %d materials for %s\n", nextMaterial, fname); 
	*materialCount = nextMaterial; 
}

struct Material *findMaterial(struct Material *material, int materialCount, const char *name)
{
	int i; 
	for(i = 0; i < materialCount; i++) {
		if(strcmp(name, material[i].name) == 0) {
			material[i].useCount++; 
			return material + i; 
		}
	}
	return material; 
}

void fillWavefrontFace(struct WavefrontModel *mod, struct WavefrontState *state, int *v, int *t, int *n)
{
	int i; 
	for(i = 0; i < 3; i++) {
		// do range checking on the parameters.
		if(v[i] < 0 || v[i] >= state->positionCount) {
			printf("*** vertex out of range: %d of %d\n", v[i], state->positionCount); 
			return; 
		}
		if(t[i] < 0 || t[i] >= state->textureCount) {
			printf("*** texture coordinate out of range: %d of %d\n", t[i], state->textureCount); 
			return; 
		}
		if(n[i] < 0 || n[i] >= state->normalCount) {
			printf("*** normal out of range: %d of %d\n", n[i], state->normalCount); 
			return; 
		}
		// fill in the data in the face.
		struct Vertex3DTNP *vert = mod->vert + state->face * 3 + i; 
		vert->u = state->texture[t[i]].u; 
		vert->v = state->texture[t[i]].v; 
		vert->nx = state->normal[n[i]].x; 
		vert->ny = state->normal[n[i]].y; 
		vert->nz = state->normal[n[i]].z; 
		vert->x = state->position[v[i]].x; 
		vert->y = state->position[v[i]].y; 
		vert->z = state->position[v[i]].z; 
		// update min/max
		if(i == 0 && state->face == 0) {
			mod->min[0] = vert->x; 
			mod->min[1] = vert->y; 
			mod->min[2] = vert->z; 
			mod->max[0] = vert->x; 
			mod->max[1] = vert->y; 
			mod->max[2] = vert->z; 
		}
		if(mod->min[0]>vert->x) mod->min[0] = vert->x; 
		if(mod->min[1]>vert->y) mod->min[1] = vert->y; 
		if(mod->min[2]>vert->z)	mod->min[2] = vert->z; 
		if(mod->max[0] < vert->x) mod->max[0] = vert->x; 
		if(mod->max[1] < vert->y) mod->max[1] = vert->y; 
		if(mod->max[2] < vert->z) mod->max[2] = vert->z; 
	}	
	state->face++; 
	if(state->face * 3 > mod->vertCount) printf("face out of range: %d instead of %d\n", state->face, mod->vertCount/3); 
	int g = mod->groupCount - 1; 
	if(g < 0) g = 0; 	// no materials special case.
	if(state->face < 0) { 
		printf("State face invalid!\n");  
		exit(1);  
	}
	mod->group[g].last = state->face * 3; 
}

void loadWavefrontLine(char *line, struct WavefrontModel *mod, struct WavefrontState *state)
{
	while(line[0] == ' ' || line[0] == '\t') line++; 
	if(strchr(line, '\n')) strchr(line, '\n')[0] = 0; 
	if(strchr(line, '\r')) strchr(line, '\r')[0] = 0; 
	if(line[0] == '#') return; 
	if(line[0] == 'f') {
		int v[4] = {0, 0, 0, 0}, t[4] = {0, 0, 0, 0}, n[4] = {0, 0, 0, 0}; 
		int rc = sscanf(line, "f%d/%d/%d%d/%d/%d%d/%d/%d%d/%d/%d", &v[0], &t[0], &n[0], &v[1], &t[1], &n[1], &v[2], &t[2], &n[2], &v[3], &t[3], &n[3]); 
		if(rc == 12) {
			int vq[3] = {v[0], v[2], v[3]}, tq[3] = {t[0], t[2], t[3]}, nq[3] = {n[0], n[2], n[3]}; 
			int i; 
			for(i = 0; i < 3; i++) { vq[i]--;  tq[i]--;  nq[i]--;  }
			fillWavefrontFace(mod, state, vq, tq, nq); 
		}
		sscanf(line, "f%d/%d/%d%d/%d/%d%d/%d/%d", &v[0], &t[0], &n[0], &v[1], &t[1], &n[1], &v[2], &t[2], &n[2]); 
		int i; 
		for(i = 0; i < 3; i++) { v[i]--;  t[i]--;  n[i]--;  }
		fillWavefrontFace(mod, state, v, t, n); 
	} else if(line[0] == 'v') {
		if(line[1] == 't') {
			float u = 0, v = 0; 
			sscanf(line, "vt%f%f", &u, &v); 
			state->texture[state->textureCount].u = u; 
			state->texture[state->textureCount++].v = 1 - v; 
			if(state->textureCount >= state->textureMax) printf("Texture count at %d of %d\n", state->textureCount, state->textureMax); 
		} else if(line[1] == 'n') {
			float x = 0, y = 0, z = 0; 
			sscanf(line, "vn%f%f%f", &x, &y, &z); 
			state->normal[state->normalCount].x = x; 
			state->normal[state->normalCount].y = y; 
			state->normal[state->normalCount++].z = z; 
			if(state->normalCount >= state->normalMax) printf("Normal count at %d of %d\n", state->normalCount, state->normalMax); 
		} else if(line[1] == ' ') {
			float x = 0, y = 0, z = 0; 
			sscanf(line, "v%f%f%f", &x, &y, &z); 
			state->position[state->positionCount].x = x; 
			state->position[state->positionCount].y = y; 
			state->position[state->positionCount++].z = z; 
			if(state->positionCount >= state->positionMax) printf("Position count at %d of %d\n", state->positionCount, state->positionMax); 
		}
	} else if(strncmp(line, "usemtl", 6) == 0) {
		state->currentMaterial = findMaterial(state->material, state->materialCount, line + 7); 
		mod->groupCount++; 
		int g = mod->groupCount-1; 
		mod->group[g].first = state->face * 3; 
		mod->group[g].last = state->face * 3; 
		mod->group[g].image = state->currentMaterial->image; 
		mod->group[g].transparent = strcmp(state->currentMaterial->name, "Iceglass") == 0?1:0; 
		//printf("usemtl '%s' -> image '%s' for vert %d on\n", state->currentMaterial->name, state->currentMaterial->image->filename, state->face * 3); 
	}
}

struct WavefrontModel *loadWavefront(const char *fname)
{
	struct WavefrontModel *mod = (struct WavefrontModel *)calloc(sizeof(struct WavefrontModel), 1); 
	strcpy(mod->name, fname); 
	char path[256]; 
	struct Material material[64]; 	// at most 64 materials.  not sure why.
	memset(material, 0, sizeof(material)); 
	int materialCount = 0; 
	FILE *file; 
	struct WavefrontState state; 

	loadMaterials(fname, material + 0, 64, &materialCount); 

	printf("read obj for '%s'\n", fname); 
	sprintf(path, "models/%s/%s.obj", fname, fname); 
	file = fopen(path, "r"); 
	if(!file) {
		printf("Couldn't find %s\n", path); 
		return 0; 
	}
	// Count the verts.
	int vCount, vtCount, vnCount, fCount, gCount; 
	scanWavefront(file, &vCount, &vtCount, &vnCount, &fCount, &gCount); 
	printf("Found %d image verts,  %d texture verts,  %d normal verts,  %d groups and %d faces.\n", vCount, vtCount, vnCount, gCount, fCount); 
	strcpy(mod->name, fname); 
	mod->groupCount = 0; 
	mod->group = (struct MaterialGroup *)calloc(sizeof(struct MaterialGroup), gCount); 
	mod->vertCount = fCount * 3; 
	mod->vert = (struct Vertex3DTNP *)calloc(sizeof(struct Vertex3DTNP), fCount * 3); 
	
	state.face = 0; 
	state.faceMax = fCount; 
	state.position = (struct Vertex3DP *)calloc(sizeof(struct Vertex3DP), vCount); 
	state.positionCount = 0; 
	state.positionMax = vCount; 
	state.normal = (struct Vertex3DP *)calloc(sizeof(struct Vertex3DP), vnCount); 
	state.normalCount = 0; 
	state.normalMax = vnCount; 
	state.texture = (struct Vertex3DT *)calloc(sizeof(struct Vertex3DT), vtCount); 
	state.textureCount = 0; 
	state.textureMax = vtCount; 
	state.material = material; 
	state.materialCount = materialCount; 
	state.currentMaterial = 0; 
	
	//printf("item[nextItem].vert = %08x\n", (int)item[nextItem].vert); 
	int i; 
	for(i = 0; i < 16; i++) {
		mod->matrix[i] = (i % 4) == (i/4)?1.0f:0; 
	}

	char line[256]; 
	while(fgets(line, 255, file)) {
		loadWavefrontLine(line, mod, &state); 
	}
	if(state.texture) free(state.texture); 
	state.texture = 0; 
	if(state.normal) free(state.normal); 
	state.normal = 0; 
	if(state.position) free(state.position); 
	state.position = 0; 
	fclose(file); 
	// now clean up the materials that weren't used,  if any
	for(i = 0; i < materialCount; i++) {
		if(material[i].useCount == 0) {
			printf("*** Material '%s' unused!\n", material[i].name); 
			if(material[i].image) freeImage(material[i].image); 
			material[i].image = 0; 
		}
	}
	
	return mod; 
}

int cmpMaterialGroupImage(const void *one, const void *two)
{
	const struct MaterialGroup *a = (struct MaterialGroup *)one, *b = (struct MaterialGroup *)two; 
	
	return 	a->image < b->image?-1:a->image>b->image?1:0; 
}

void freeWavefront(struct WavefrontModel *mod)
{
	qsort(mod->group, mod->groupCount, sizeof(struct MaterialGroup), cmpMaterialGroupImage); 
	Image *last = 0; 
	int j; 
	for(j = 0; j < mod->groupCount; j++) {
		struct MaterialGroup *g = mod->group + j; 
		if(g->image != 0 && g->image != last) {
			freeImage(g->image); 
			last = g->image; 
			g->image = 0; 
		}
	}

	if(mod->group) free(mod->group); 
	mod->group = 0; 
	if(mod->vert) free(mod->vert); 
	mod->vert = 0; 
	free(mod);
}

void drawWavefrontPartial(struct WavefrontModel *mod, int transparent)
{
	if(!mod) return; 
#ifdef _PSP
	//printf("Rendering item %d\n", i); 
	sceGumMatrixMode(GU_MODEL); 
	sceGumPushMatrix(); 
	sceGumMultMatrix((ScePspFMatrix4 *)mod->matrix); 
	int j = 0; 
	int g; 
	int jCount; 
	//printf("item image: %08x\n", (int)item[i].image); 
	sceGuEnable(GU_TEXTURE_2D); 
	sceGuColor(0xffffffff); 
	sceGuFrontFace(GU_CCW); 

	sceGuTexScale(1.0f, 1.0f); 
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA); 
	sceGuEnable(GU_LIGHTING); 

	for(g = 0; g < mod->groupCount; g++) {
		if(transparent == 0 && mod->group[g].transparent) continue; 
		if(transparent == 1 && !mod->group[g].transparent) continue; 
		if(mod->group[g].image) {
			Image *source = mod->group[g].image; 
			sceGuTexMode(GU_PSM_8888,  0,  0,  source->isSwizzled); 
			sceGuTexImage(0,  source->textureWidth,  source->textureHeight,  source->textureWidth,  source->data); 
		}
		j = mod->group[g].first; 
		jCount = mod->group[g].last; 
		if(j >= jCount) printf("wavefrontmodel %s: skipping group %d of %d,  vert %d-%d\n", mod->name, g, mod->groupCount, j, jCount); 
		while(j < jCount) {
			int count = 30720; 
			if(j + count > jCount) count = jCount - j; 
			if(count <= 0) { printf("lost my place.\n");  continue;  }
			if(mod->vert) sceGumDrawArray(GU_TRIANGLES, GU_VERTEX_32BITF|GU_NORMAL_32BITF|GU_TEXTURE_32BITF|GU_TRANSFORM_3D, count, 0, mod->vert + j); 
			j = j + count; 
		}
	}
	sceGuFrontFace(GU_CW); 
	sceGumPopMatrix(); 
#else
	//printf("Rendering item %d\n", i); 
	glMatrixMode(GL_MODELVIEW); 
	glPushMatrix(); 
	glMultMatrixf((float *)mod->matrix); 
	int j = 0; 
	int g; 
	int jCount; 
	//printf("item image: %08x\n", (int)item[i].image); 
	glEnable(GL_TEXTURE_2D); 
	glFrontFace(GL_CCW); 
	glColor3f(1, 1, 1); 

	glEnable(GL_LIGHTING); 
    glBlendFunc(GL_SRC_ALPHA,  GL_ONE_MINUS_SRC_ALPHA); 
    glEnable(GL_BLEND); 
    glAlphaFunc(GL_GREATER, 0); 
    glEnable(GL_ALPHA_TEST); 
    
	for(g = 0; g < mod->groupCount; g++) {
		if(transparent == 0 && mod->group[g].transparent) continue; 
		if(transparent == 1 && !mod->group[g].transparent) continue; 
		if(mod->group[g].image) {
			Image *source = mod->group[g].image; 
			if(source->texid == 0) uploadImage(source); 
			glBindTexture(GL_TEXTURE_2D, source->texid); 
		}
		j = mod->group[g].first; 
		jCount = mod->group[g].last; 
		if(j >= jCount) printf("wavefrontmodel %s: skipping group %d of %d,  vert %d-%d\n", mod->name, g, mod->groupCount, j, jCount); 
		glBegin(GL_TRIANGLES); 
		for(j = mod->group[g].first; j < jCount; j++) {
			if(mod->vert) {
                struct Vertex3DTNP *v = mod->vert + j; 
                glTexCoord2f(v->u, v->v); 
                glNormal3f(v->nx, v->ny, v->nz); 
                glVertex3f(v->x, v->y, v->z); 
            }
		}
		glEnd(); 
	}
    glDisable(GL_ALPHA_TEST); 
	glFrontFace(GL_CW); 
	glPopMatrix(); 
#endif
}
void drawWavefront(struct WavefrontModel *mod)
{
	drawWavefrontPartial(mod, 3); 
}

void setWavefrontPos(struct WavefrontModel *model, float x, float y, float z)
{
	if(model == 0) return; 
	model->matrix[12] = x; 
	model->matrix[13] = y; 
	model->matrix[14] = z; 
	model->matrix[0] = 1;  //6; 
	model->matrix[5] = 1;  //6; 
	model->matrix[10] = 1;  //6; 
}

float *getWavefrontMin(struct WavefrontModel *model)
{
	if(!model) return 0; 
	return model->min; 
}

float *getWavefrontMax(struct WavefrontModel *model)
{
	if(!model) return 0; 
	return model->max; 
}

#ifndef MAP_GLOBALS_H
#define MAP_GLOBALS_H

#include "map_types.h"
#include "../../math3d.h" // pour Vec3

// Variables globales accessibles par tous
extern int p[512];
extern int lastChunkX, lastChunkY;
extern int mode_fps;
extern Vec3 cam_pos;
extern float cam_zaw, cam_pitch;
extern char* worldName;
extern char* seedString;
extern char** worldNames;
extern int worldCount;
extern chunk* c;
extern chunk* loadedChunks[2 * RENDER_DISTANCE + 1][2 * RENDER_DISTANCE + 1];
extern rocks rocks_place[NB_ROCKS];
extern tree forest[NB_TREES];
extern clouds cloud_fields[NB_CLOUDS];
extern int count_tick;

#endif

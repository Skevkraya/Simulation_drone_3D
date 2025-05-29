#ifndef PERLIN_H
#define PERLIN_H

#include "../main/map_types.h"

void init_perlin(void);
void generate_chunk(chunk* chunk, int cx, int cy);
void generate_heightmap(void);
float perlin2D(float x, float y);

#endif

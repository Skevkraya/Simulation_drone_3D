#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "../main/map_types.h"
#include "../main/map_generals.h"
#include "perlin.h"

unsigned int hash_seed_string(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }
    return hash;
}

void init_perlin() {
    int world_seed = hash_seed_string(seedString);
    srand(world_seed);

    int perm[256];
    for (int i = 0; i < 256; i++) perm[i] = i;
    for (int i = 255; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = perm[i];
        perm[i] = perm[j];
        perm[j] = tmp;
    }
    for (int i = 0; i < 256; i++) {
        p[i] = p[i + 256] = perm[i];
    }
}


float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

float grad(int hash, float x, float y) {
    int h = hash & 7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float perlin2D(float x, float y) {
    int xi = (int)floor(x) & 255;
    int yi = (int)floor(y) & 255;
    float xf = x - floor(x);
    float yf = y - floor(y);
    float u = fade(xf);
    float v = fade(yf);

    int aa = p[p[xi] + yi];
    int ab = p[p[xi] + yi + 1];
    int ba = p[p[xi + 1] + yi];
    int bb = p[p[xi + 1] + yi + 1];

    float x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1, yf), u);
    float x2 = lerp(grad(ab, xf, yf - 1), grad(bb, xf - 1, yf - 1), u);
    return (lerp(x1, x2, v) + 1.0f) / 2.0f;
}

void generate_chunk(chunk* chunk, int cx, int cy) {
    chunk->chunkX = cx;
    chunk->chunkY = cy;

    for (int y = 0; y <= CHUNK_SIZE; y++) {
        for (int x = 0; x <= CHUNK_SIZE; x++) {
            float worldX = (cx * CHUNK_SIZE + x) * SCALE;
            float worldY = (cy * CHUNK_SIZE + y) * SCALE;
            chunk->heightmap[x][y] = perlin2D(worldX, worldY);
        }
    }
}

void generate_heightmap() {
    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            float noise = perlin2D(x * SCALE, y * SCALE);
            c->heightmap[x][y] = noise;
        }
    }
}
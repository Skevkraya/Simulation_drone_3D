#include <GL/gl.h>
#include <math.h>
#include "../main/map_generals.h"
#include "../main/map_types.h"
#include "draw_chunk.h"

void set_color(float h) {
    if (h < 0.2f) glColor3f(0.2f, 0.4f, 1.0f);
    else if (h < 0.5f) glColor3f(0.2f, 0.8f, 0.2f);
    else if (h < 0.7f) glColor3f(0.6f, 0.4f, 0.2f); 
    else glColor3f(1.0f, 1.0f, 1.0f);
}

void draw_chunk(const chunk* c) {
    int baseX = c->chunkX * CHUNK_SIZE;
    int baseZ = c->chunkY * CHUNK_SIZE;

    for (int z = 0; z < CHUNK_SIZE; z++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            float h1 = c->heightmap[x][z] * AMPLITUDE;
            float h2 = c->heightmap[x+1][z] * AMPLITUDE;
            float h3 = c->heightmap[x][z+1] * AMPLITUDE;
            float h4 = c->heightmap[x+1][z+1] * AMPLITUDE;

            float wx = x + baseX;
            float wz = z + baseZ;

            glBegin(GL_TRIANGLES);

            set_color(c->heightmap[x][z]);
            glVertex3f(wx, h1, wz);

            set_color(c->heightmap[x+1][z]);
            glVertex3f(wx + 1, h2, wz);

            set_color(c->heightmap[x][z+1]);
            glVertex3f(wx, h3, wz + 1);

            set_color(c->heightmap[x+1][z]);
            glVertex3f(wx + 1, h2, wz);

            set_color(c->heightmap[x+1][z+1]);
            glVertex3f(wx + 1, h4, wz + 1);

            set_color(c->heightmap[x][z+1]);
            glVertex3f(wx, h3, wz + 1);

            glEnd();
        }
    }
}



void draw_all_chunks(cameraState cam) {

    for (int i = 0; i < 2 * RENDER_DISTANCE + 1; i++) {
        for (int j = 0; j < 2 * RENDER_DISTANCE + 1; j++) {
            chunk* ch = loadedChunks[i][j];
            if (!ch) continue;

            int dx = ch->chunkX * CHUNK_SIZE + CHUNK_SIZE / 2 - (int)cam.x;
            int dy = ch->chunkY * CHUNK_SIZE + CHUNK_SIZE / 2 - (int)cam.y;
            int dist_sq = dx * dx + dy * dy;

            if (dist_sq < 200 * 200) {
                draw_chunk(ch);
            }
        }
    }
}
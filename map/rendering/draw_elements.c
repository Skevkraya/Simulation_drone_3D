#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include "../main/map_generals.h"
#include "../main/map_types.h"
#include "../generation/elements.h"
#include "draw_elements.h"

void draw_forest() {
    for (int i = 0; i < 2 * RENDER_DISTANCE + 1; i++) {
        for (int j = 0; j < 2 * RENDER_DISTANCE + 1; j++) {
            chunk* ch = loadedChunks[i][j];
            if (!ch) continue;

            int baseX = ch->chunkX * CHUNK_SIZE;
            int baseZ = ch->chunkY * CHUNK_SIZE;

            for (int k = 0; k < NB_TREES; k++) {
                int lx = forest[k].x;
                int lz = forest[k].z;          
                int wx = baseX + lx;
                int wz = baseZ + lz;

                if ((wx / CHUNK_SIZE) != ch->chunkX || (wz / CHUNK_SIZE) != ch->chunkY)
                    continue;

                float h = ch->heightmap[lx][lz];

                if (h > WATER_LEVEL && h < MOUNTAIN_LEVEL) {
                    float height_tree = forest[k].height_tree;
                    float y = h * AMPLITUDE;

                    glPushMatrix();
                    glTranslatef(wx + 0.5f, y, wz + 0.5f);
                    GLUquadric* quad = gluNewQuadric();

                    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                    glColor3f(0.4f, 0.2f, 0.0f);
                    gluCylinder(quad, 0.2f, 0.2f, height_tree, 8, 1);

                    glTranslatef(0, 0, height_tree);
                    glColor3f(0.0f, 0.6f, 0.0f);
                    gluSphere(quad, 1.6f, 8, 8);

                    gluDeleteQuadric(quad);
                    glPopMatrix();
                }
            }
        }
    }
}

                

void draw_rock() {
    for (int i = 0; i < 2 * RENDER_DISTANCE + 1; i++) {
        for (int j = 0; j < 2 * RENDER_DISTANCE + 1; j++) {
            chunk* c = loadedChunks[i][j];
            if (!c) continue;

            int baseX = c->chunkX * CHUNK_SIZE;
            int baseZ = c->chunkY * CHUNK_SIZE;

            for (int k = 0; k < NB_ROCKS; k++) {
                int lx = rocks_place[k].x;
                int lz = rocks_place[k].z;
                int wx = baseX + lx;
                int wz = baseZ + lz;
                if ((wx / CHUNK_SIZE) != c->chunkX || (wz / CHUNK_SIZE) != c->chunkY)
                    continue;

                float h = c->heightmap[lx][lz];
                if (h > WATER_LEVEL && h > MOUNTAIN_LEVEL) {
                    glPushMatrix();
                    glTranslatef(wx + 0.5f,  h * AMPLITUDE - 0.1f , wz + 0.2f);

                    GLUquadric* quad = gluNewQuadric();
                    float r = rocks_place[k].r;
                    glColor3f(r, r - 0.05f, r - 0.1f);
                    gluSphere(quad, 0.6, 8, 8);
                    gluDeleteQuadric(quad);
                    glPopMatrix();
                }
            }
        }
    }
}

void draw_water(float level, float time) {
    time = time / 3600.0f;
    float waterHeight = level * AMPLITUDE;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);
    glColor4f(0.2f, 0.4f, 1.0f, 0.5f);

    for (int i = 0; i < 2 * RENDER_DISTANCE + 1; i++) {
        for (int j = 0; j < 2 * RENDER_DISTANCE + 1; j++) {
            chunk* c = loadedChunks[i][j];
            if (!c) continue;

            int baseX = c->chunkX * CHUNK_SIZE;
            int baseZ = c->chunkY * CHUNK_SIZE;

            for (int z = 0; z < CHUNK_SIZE; z++) {
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    if (c->heightmap[x][z] < level || c->heightmap[x+1][z] < level ||
                        c->heightmap[x][z+1] < level || c->heightmap[x+1][z+1] < level) {

                        float y1 = waterHeight + 0.1f * sinf(0.3f * x + 0.2f * z + time);
                        float y2 = waterHeight + 0.1f * sinf(0.3f * (x + 1) + 0.2f * z + time);
                        float y3 = waterHeight + 0.1f * sinf(0.3f * x + 0.2f * (z + 1) + time);
                        float y4 = waterHeight + 0.1f * sinf(0.3f * (x + 1) + 0.2f * (z + 1) + time);

                        glBegin(GL_TRIANGLES);
                        glVertex3f(baseX + x, y1 , baseZ + z);
                        glVertex3f(baseX + x + 1, y2, baseZ + z);
                        glVertex3f(baseX + x, y3, baseZ + z + 1);

                        glVertex3f(baseX + x + 1, y2, baseZ + z);
                        glVertex3f(baseX + x + 1, y4, baseZ + z + 1);
                        glVertex3f(baseX + x, y3, baseZ + z + 1);
                        glEnd();
                    }
                }
            }
        }
    }

    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
}


void draw_clouds(float time) {
  
    for (int i = 0; i < 2 * RENDER_DISTANCE + 1; i++) {
        for (int j = 0; j < 2 * RENDER_DISTANCE + 1; j++) {
                chunk* c = loadedChunks[i][j];
                if (!c) continue;
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDisable(GL_LIGHTING);
                glColor4f(1.0f, 1.0f, 1.0f, 0.4f);  
                int baseX = c->chunkX * CHUNK_SIZE;
                int baseZ = c->chunkY * CHUNK_SIZE;

                for (int k = 0; k < NB_CLOUDS; k++) {
                    float cx = cloud_fields[k].x + sinf(time * 0.001f + i) * 2.0f;
                    float cz = cloud_fields[k].z;
                    float cy = AMPLITUDE + 10.0f + sinf(time * 0.002f + i);

                    glPushMatrix();
                    glTranslatef(cx, cy, cz);
                    GLUquadric* quad = gluNewQuadric();
                    gluSphere(quad, cloud_fields[i].r, 12, 12);
                    gluDeleteQuadric(quad);
                    glPopMatrix();
                }

                glDisable(GL_BLEND);
                glEnable(GL_LIGHTING);
            }
        }
}
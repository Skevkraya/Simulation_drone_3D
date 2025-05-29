#include "../lib.h"
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <SDL/SDL_ttf.h>
#include <sys/stat.h>
#include "map_perlin.h"
#include "../drone.h"
#include "../input.h"
#include "../pid.h"
#include "../math3d.h"


// gcc main.c drone.c map/map_perlin.c input.c math3d.c pid.c -o droneSim -lmingw32 -lSDLmain -lSDL -lSDL_gfx -lopengl32 -lglu32 -lm -lSDL_ttf
// sudo apt install libsdl-ttf2.0-dev
int p[512];
int lastChunkX = -9999, lastChunkY = -9999;
extern int mode_fps;
Vec3 cam_pos = {50.0f, 20.f, 50.0f};
float cam_zaw = 0.0f, cam_pitch = 0.0f;
char* worldName = NULL;
char* seedString = NULL;
char** worldNames = NULL;
int worldCount = 0;
chunk* c = NULL;
chunk* loadedChunks[2 * RENDER_DISTANCE + 1][2 * RENDER_DISTANCE + 1];
rocks rocks_place[NB_ROCKS];
tree forest[NB_TREES];
clouds cloud_fields[NB_CLOUDS];
int count_tick = 0;

int search(Node* root, int x, int z) {
    if (root == NULL) return 0;
    if (x == root->x && z == root->z) return 1;
    if (x < root->x || (x == root->x && z < root->z))
        return search(root->left, x, z);
    else
        return search(root->right, x, z);
}

Node* insert(Node* root, int x, int z) {
    if (root == NULL) {
        Node* node = malloc(sizeof(Node));
        node->x = x;
        node->z = z;
        node->left = node->right = NULL;
        return node;
    }
    if (x < root->x || (x == root->x && z < root->z))
        root->left = insert(root->left, x, z);
    else if (x > root->x || (x == root->x && z > root->z))
        root->right = insert(root->right, x, z);

    return root;
}

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

// void updatec(void) {
//     cameraState cam = get_current_camera_position();
//     int cx = (int)(cam.x) / CHUNK_SIZE;
//     int cy = (int)(cam.y) / CHUNK_SIZE;

//     if (cx != lastChunkX || cy != lastChunkY) {
//         if (c == NULL) c = malloc(sizeof(chunk));
//         if (!load_chunk(c, cx, cy)) {
//             generate_chunk(c, cx, cy);
//             save_chunk(c);
//         }
//         lastChunkX = cx;
//         lastChunkY = cy;
//     }
// }
void updateChunks() {
    cameraState cam = get_current_camera_position();
    // printf("[updateChunks] Position : %.2f %.2f\n", cam.x, cam.y);
    int cx = (int)(cam.x) / CHUNK_SIZE;
    int cy = (int)(cam.y) / CHUNK_SIZE;

    if (lastChunkX == -9999 && lastChunkY == -9999) {
        lastChunkX = cx;
        lastChunkY = cy;
    }

    const int SIZE = 2 * RENDER_DISTANCE + 1;
    chunk* newChunks[SIZE][SIZE];
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            newChunks[i][j] = NULL;

    // Compteurs debug
    int reusedCount = 0;
    int generatedCount = 0;
    int freedCount = 0;

    // Génère ou déplace les chunks
    for (int dx = -RENDER_DISTANCE; dx <= RENDER_DISTANCE; dx++) {
        for (int dy = -RENDER_DISTANCE; dy <= RENDER_DISTANCE; dy++) {
            int chunkX = cx + dx;
            int chunkY = cy + dy;
            int newIx = dx + RENDER_DISTANCE;
            int newIy = dy + RENDER_DISTANCE;
            int oldIx = chunkX - lastChunkX + RENDER_DISTANCE;
            int oldIy = chunkY - lastChunkY + RENDER_DISTANCE;

            if (oldIx >= 0 && oldIx < SIZE &&
                oldIy >= 0 && oldIy < SIZE &&
                loadedChunks[oldIx][oldIy]) {
                
                newChunks[newIx][newIy] = loadedChunks[oldIx][oldIy];
                loadedChunks[oldIx][oldIy] = NULL;
                reusedCount++;
            } else {
                chunk* ch = malloc(sizeof(chunk));
                if (!ch) {
                    fprintf(stderr, "Erreur malloc pour chunk (%d, %d)\n", chunkX, chunkY);
                    exit(EXIT_FAILURE);
                }

                if (!load_chunk(ch, chunkX, chunkY)) {
                    generate_chunk(ch, chunkX, chunkY);
                    save_chunk(ch);
                    generatedCount++;
                }
                newChunks[newIx][newIy] = ch;
            }
        }
    }

    // Libère les anciens chunks non utilisés
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            chunk* ch = loadedChunks[i][j];
            if (ch) {
                // Vérifie s’il a été déplacé dans newChunks
                int moved = 0;
                for (int ni = 0; ni < SIZE && !moved; ni++) {
                    for (int nj = 0; nj < SIZE && !moved; nj++) {
                        if (newChunks[ni][nj] == ch)
                            moved = 1;
                    }
                }

                if (!moved) {
                    save_chunk(ch);
                    free(ch);
                    freedCount++;
                }

                loadedChunks[i][j] = NULL;
            }
        }
    }

    // Copie dans loadedChunks
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            loadedChunks[i][j] = newChunks[i][j];
        }
    }

    lastChunkX = cx;
    lastChunkY = cy;
    
    // printf("Chunks → réutilisés : %d | générés : %d | libérés : %d\n", reusedCount, generatedCount, freedCount);
}



chunk* get_chunk_at_world(int x, int y) {
    int chunkX = x / CHUNK_SIZE;
    int chunkY = y / CHUNK_SIZE;

    int localX = x % CHUNK_SIZE;
    int localY = y % CHUNK_SIZE;

    if (localX < 0) { chunkX--; localX += CHUNK_SIZE; }
    if (localY < 0) { chunkY--; localY += CHUNK_SIZE; }

    int idxX = chunkX - (lastChunkX - RENDER_DISTANCE);
    int idxY = chunkY - (lastChunkY - RENDER_DISTANCE);

    if (idxX < 0 || idxX >= 2 * RENDER_DISTANCE + 1 ||
        idxY < 0 || idxY >= 2 * RENDER_DISTANCE + 1) {
        fprintf(stderr, "[get_chunk_at_world] Chunk (%d,%d) hors de portée (%d,%d)\n", chunkX, chunkY, idxX, idxY);
        return NULL;
    }

    return loadedChunks[idxX][idxY];
}


void save_chunk(chunk* chunk) {
    char folder[256];
    snprintf(folder, sizeof(folder), "chunks/%s", worldName);
    #ifdef _WIN32
    mkdir("chunks");
    mkdir(folder);
    #else 
    mkdir("chunks", 0777);
    mkdir(folder, 0777);
    #endif

    char path[512];
    snprintf(path, sizeof(path), "%s/chunk_%d_%d.bin", folder, chunk->chunkX, chunk->chunkY);

    FILE* f = fopen(path, "wb");
    if (f) {
        fwrite(chunk, sizeof(*chunk), 1, f);
        fclose(f);
    }
}


int load_chunk(chunk* chunk, int cx, int cy) {
    char path[255];
    sprintf(path, "chunks/%s/chunk_%d_%d.bin", worldName, cx, cy);
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    fread(chunk, sizeof(*chunk), 1, f);
    fclose(f);
    return 1;
}
void save_player_state(Vec3 pos) {
    char path[128];
    sprintf(path, "chunks/%s/metadata.txt", worldName);
    FILE* f = fopen(path, "w");
    if (f) {
        fprintf(f, "%s\n", seedString); // Ligne 1 : seed
        fprintf(f, "%lf %lf %lf\n", pos.x, pos.y, pos.z); // Ligne 2 : position
        fclose(f);
    }
}


Vec3 load_player_state() {
    Vec3 pos = {CHUNK_SIZE / 2, CHUNK_SIZE / 2, 10};
    char path[128];
    sprintf(path, "chunks/%s/metadata.txt", worldName);

    FILE* f = fopen(path, "r");
    if (f) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), f)) {
            buffer[strcspn(buffer, "\n")] = '\0';  // Enlève le \n
            seedString = strdup(buffer);
        }

        fscanf(f, "%lf %lf %lf", &pos.x, &pos.y, &pos.z);
        fclose(f);
    } else {
        fprintf(stderr, "[WARN] metadata.txt introuvable pour %s\n", worldName);
        seedString = strdup("default");
    }

    return pos;
}

void generate_heightmap() {
    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            float noise = perlin2D(x * SCALE, y * SCALE);
            c->heightmap[x][y] = noise;
        }
    }
}
void set_color(float h) {
    if (h < 0.2f) glColor3f(0.2f, 0.4f, 1.0f);       // eau
    else if (h < 0.5f) glColor3f(0.2f, 0.8f, 0.2f);  // herbe
    else if (h < 0.7f) glColor3f(0.6f, 0.4f, 0.2f);  // terre
    else glColor3f(1.0f, 1.0f, 1.0f);               // neige
}

void generate_forest() {
    Node* root = NULL;
    int count = 0;
    for (int i = 0; i < NB_TREES; i++) {
        int x = rand() % CHUNK_SIZE;
        int z = rand() % CHUNK_SIZE;
        forest[i].height_tree = 4.0f + ((float)rand() / RAND_MAX) * 6.0f;
        if(!search(root, x, z)){
            root = insert (root, x, z);
            forest[count].x = rand() % CHUNK_SIZE;
            forest[count].z = rand() % CHUNK_SIZE;
            count++;
        }
    }
}

void generate_rock() {
    Node* root = NULL;
    int count = 0;
    for (int i = 0; i < NB_TREES; i++) {
        int x = rand() % CHUNK_SIZE;
        int z = rand() % CHUNK_SIZE;
        rocks_place[i].r = 0.45f + (rand() % 10) / 100.0f;
        if(!search(root, x, z)){
            root = insert (root, x, z);
            rocks_place[count].x = rand() % CHUNK_SIZE;
            rocks_place[count].z = rand() % CHUNK_SIZE;
            count++;
        }
    }
}
void generate_clouds() {
    for (int i = 0; i < NB_CLOUDS; i++) {
        cloud_fields[i].x = rand() % CHUNK_SIZE;
        cloud_fields[i].z = rand() % CHUNK_SIZE;
        cloud_fields[i].r = 2.0f + ((float)rand() / RAND_MAX) * 2.0f;
    }
}

const tree* get_forest() {
    return forest;
}
const rocks* get_rocks() {
    return rocks_place;
}
// void draw_terrain_cube(int x, int y, float h) {
//     float h_top = h * AMPLITUDE;
//     float h_bottom = 0.0f; // sol bas
//     set_color(h);
//     glBegin(GL_QUADS);
//     glVertex3f(x,     y,     h_top);
//     glVertex3f(x + 1, y,     h_top);
//     glVertex3f(x + 1, y + 1, h_top);
//     glVertex3f(x,     y + 1, h_top);
//     glEnd();
//     glColor3f(0.1f, 0.1f, 0.1f);
//     glBegin(GL_QUADS);
//     glVertex3f(x,     y,     h_bottom);
//     glVertex3f(x + 1, y,     h_bottom);
//     glVertex3f(x + 1, y + 1, h_bottom);
//     glVertex3f(x,     y + 1, h_bottom);
//     glEnd();
//     // Faces latérales : Nord, Sud, Est, Ouest
//     float neighbor;
//     // N (y+1)
//     if (y + 1 >= CHUNK_SIZE || c->heightmap[x][y + 1] * AMPLITUDE < h_top) {
//         glColor3f(0.15f, 0.15f, 0.15f);
//         glBegin(GL_QUADS);
//         glVertex3f(x,     y + 1, h_bottom);
//         glVertex3f(x + 1, y + 1, h_bottom);
//         glVertex3f(x + 1, y + 1, h_top);
//         glVertex3f(x,     y + 1, h_top);
//         glEnd();
//     }
//     // S (y-1)
//     if (y == 0 || c->heightmap[x][y - 1] * AMPLITUDE < h_top) {
//         glColor3f(0.15f, 0.15f, 0.15f);
//         glBegin(GL_QUADS);
//         glVertex3f(x,     y, h_bottom);
//         glVertex3f(x + 1, y, h_bottom);
//         glVertex3f(x + 1, y, h_top);
//         glVertex3f(x,     y, h_top);
//         glEnd();
//     }
//     // E (x+1)
//     if (x + 1 >= CHUNK_SIZE || c->heightmap[x + 1][y] * AMPLITUDE < h_top) {
//         glColor3f(0.2f, 0.2f, 0.2f);
//         glBegin(GL_QUADS);
//         glVertex3f(x + 1, y,     h_bottom);
//         glVertex3f(x + 1, y + 1, h_bottom);
//         glVertex3f(x + 1, y + 1, h_top);
//         glVertex3f(x + 1, y,     h_top);
//         glEnd();
//     }
//     // O (x-1)
//     if (x == 0 || c->heightmap[x - 1][y] * AMPLITUDE < h_top) {
//         glColor3f(0.2f, 0.2f, 0.2f);
//         glBegin(GL_QUADS);
//         glVertex3f(x, y,     h_bottom);
//         glVertex3f(x, y + 1, h_bottom);
//         glVertex3f(x, y + 1, h_top);
//         glVertex3f(x, y,     h_top);
//         glEnd();
//     }
// }
// void draw_terrain_minecraft() {
//     for (int y = 0; y < CHUNK_SIZE; y++) {
//         for (int x = 0; x < CHUNK_SIZE; x++) {
//             draw_terrain_cube(x, y, c->heightmap[x][y]);
//         }
//     }
// }
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING); // Optionnel, à voir si tu veux un style non affecté par lumière
    glColor4f(1.0f, 1.0f, 1.0f, 0.4f); // Blanc translucide

    for (int i = 0; i < NB_CLOUDS; i++) {
        float cx = cloud_fields[i].x + sinf(time * 0.001f + i) * 2.0f;
        float cz = cloud_fields[i].z;
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


float get_ground_height_at(float x, float z) {
    int xi = (int)floor(x);
    int zi = (int)floor(z);

    chunk* ch = get_chunk_at_world(xi, zi);
    if (!ch) {
        fprintf(stderr, "[get_ground_height_at] Aucun chunk trouvé pour (%d, %d), retourne 0.0\n", xi, zi);
        return 0.0f;
    }

    int lx = xi % CHUNK_SIZE;
    int lz = zi % CHUNK_SIZE;
    if (lx < 0) lx += CHUNK_SIZE;
    if (lz < 0) lz += CHUNK_SIZE;

    if (lx >= CHUNK_SIZE - 1 || lz >= CHUNK_SIZE - 1) {
        fprintf(stderr, "[get_ground_height_at] Coordonnées locales hors bornes (%d,%d)\n", lx, lz);
        return 0.0f;
    }

    float dx = x - xi;
    float dz = z - zi;

    float h00 = ch->heightmap[lx][lz] * AMPLITUDE;
    float h10 = ch->heightmap[lx + 1][lz] * AMPLITUDE;
    float h01 = ch->heightmap[lx][lz + 1] * AMPLITUDE;
    float h11 = ch->heightmap[lx + 1][lz + 1] * AMPLITUDE;

    float h;
    if (dx + dz < 1.0f) {
        h = h00 * (1 - dx - dz) + h10 * dx + h01 * dz;
    } else {
        float dx1 = 1 - dx;
        float dz1 = 1 - dz;
        h = h11 * (dx + dz - 1) + h10 * dx1 + h01 * dz1;
    }

    return h;
}


Vec3 get_normal_at(float x, float z) {
    int xi = (int)x, zi = (int)z;
    float hL = get_ground_height_at(xi - 1, zi);
    float hR = get_ground_height_at(xi + 1, zi);
    float hD = get_ground_height_at(xi, zi - 1);
    float hU = get_ground_height_at(xi, zi + 1);
    
    Vec3 normal = {
        hL - hR, // pente Est-Ouest
        2.0f, // pente Nord-Sud
        hD - hU   // poids du Z pour garder une normale "vers le haut"
    };

    // Normalisation
    float mag = sqrt(normal.x*normal.x + normal.z*normal.z + normal.y*normal.y);
    normal.x /= mag; normal.z /= mag; normal.z /= mag;
    return normal;
}

void render_drone(Drone *drone, float armLength, float armRadius, float diskRadius) {
    // Sauve l'état de la matrice
    glPushMatrix();

    // 1) Déplace le drone à sa position
    glTranslatef(drone->pos.x,
                 drone->pos.y,
                 drone->pos.z);

    // 2) Applique les rotations dans l'ordre yaw → pitch → roll
    //    pour qu'on tourne d'abord autour de l'axe Y (lacet),
    //    puis X (tangage), puis Z (roulis).
    glRotatef(drone->rot[1], 0.0f, 1.0f, 0.0f);  // yaw
    glRotatef(drone->rot[0], 1.0f, 0.0f, 0.0f);  // pitch
    glRotatef(drone->rot[2], 0.0f, 0.0f, 1.0f);  // roll

    glRotatef(45.0f, 0.0f, 1.0f, 0.0f);

    // 3) On dessine les bras et les disques aux extrémités
    GLUquadric *q = gluNewQuadric();

    // -- Bras sur l’axe X
    glColor3f(0.6f, 0.6f, 0.6f);
    glPushMatrix();
      glTranslatef(-armLength/2, 0.0f, 0.0f);
      glRotatef(90.0f, 0, 1, 0);
      gluCylinder(q, armRadius, armRadius, armLength, 16, 1);
    glPopMatrix();

    // -- Bras sur l’axe Z
    glPushMatrix();
      glTranslatef(0.0f, 0.0f, -armLength/2);
      glRotatef(90.0f, 0, 0, 1);
      gluCylinder(q, armRadius, armRadius, armLength, 16, 1);
    glPopMatrix();

    // -- 4 disques (hélices) aux extrémités
    glColor3f(1.0f, 0.2f, 0.2f);
    const float half = armLength / 2;
    for (int i = 0; i < 4; ++i) {
        // positions : ( +X, 0, 0 ), ( -X, 0, 0 ), (0, 0, +Z), (0, 0, -Z)
        float x = (i==0 ?  half : (i==1 ? -half : 0.0f));
        float z = (i>=2 ? (i==2 ?  half : -half) : 0.0f);

        glPushMatrix();
          glTranslatef(x, 0.1f, z);
          glRotatef(-90.0f, 1, 0, 0);
          gluDisk(q, 0.0, diskRadius, 16, 1);
        glPopMatrix();
    }

    gluDeleteQuadric(q);

    // 4) Restaure la matrice
    glPopMatrix();
}


// void draw_velocity(const Drone* drone) {
//     Vec3 v = drone->velocity;
//     float scale = 0.2f; // adapter selon longueur désirée
//     glColor3f(0.0f, 0.5f, 1.0f);
//     glBegin(GL_LINES);
//         glVertex3f(0, 0, 0);
//         glVertex3f(v.x * scale, v.y * scale, v.z * scale);
//     glEnd();
// }
void load_world_names(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) return;
    worldNames = NULL;
    worldCount = 0;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), f)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
        char* name = malloc(len);
        if (!name) continue;
        strcpy(name, buffer);
        worldNames = realloc(worldNames, (worldCount + 1) * sizeof(char*));
        worldNames[worldCount] = name;
        worldCount++;
    }
    fclose(f);
}
void add_world_name(const char* filename, const char* name) {
    FILE* f = fopen(filename, "a");
    if (f) {
        fprintf(f, "%s\n", name);
        fclose(f);
    }
    char* copy = malloc(strlen(name) + 1);
    if (!copy) return;
    strcpy(copy, name);
    worldNames = realloc(worldNames, (worldCount + 1) * sizeof(char*));
    worldNames[worldCount++] = copy;
}

void free_world_names() {
    for (int i = 0; i < worldCount; i++) {
        free(worldNames[i]);
    }
    free(worldNames);
    worldNames = NULL;
    worldCount = 0;
}

void exitGame() {
    free_world_names();
    if (worldName) {
        free(worldName);
        worldName = NULL;
    }

    if (seedString) {
        free(seedString);
        seedString = NULL;
    }
    exit(EXIT_SUCCESS);
}


void drawText(int x, int y, const char* text, TTF_Font* font, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) return;

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    SDL_Surface* formatted = SDL_CreateRGBSurface(0, surface->w, surface->h, 32,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);  // RGBA
    SDL_BlitSurface(surface, NULL, formatted, NULL);
    SDL_FreeSurface(surface);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, formatted->w, formatted->h,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, formatted->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, WIDTH, HEIGHT, 0, -1, 1);  // Coord écran

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1, 1, 1);  // blanc
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2i(x, y);
        glTexCoord2f(1, 0); glVertex2i(x + formatted->w, y);
        glTexCoord2f(1, 1); glVertex2i(x + formatted->w, y + formatted->h);
        glTexCoord2f(0, 1); glVertex2i(x, y + formatted->h);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    SDL_FreeSurface(formatted);
    glDeleteTextures(1, &texture);
}

void getTextInput(const char* prompt, char* buffer, int maxLen) {
    SDL_Event e;
    int done = 0;
    int len = 0;
    buffer[0] = '\0';
    SDL_EnableUNICODE(1);
    TTF_Font* font = TTF_OpenFont("./arial.ttf", 24);
    SDL_Color white = {255, 255, 255, 255};
    
    while (!done) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) exit(0);
            if (e.type == SDL_KEYDOWN) {
                SDLKey key = e.key.keysym.sym;
                if (key == SDLK_RETURN) {
                    done = 1;
                }
                else if (key == SDLK_ESCAPE) {
                    buffer[0] = '\0';
                    done = 1;
                }
                else if (key == SDLK_BACKSPACE && len > 0) {
                    buffer[--len] = '\0';
                }
                else {
                    char ch = e.key.keysym.unicode & 0x7F;
                    if (ch >= 32 && ch < 127 && len < maxLen - 1) {
                        buffer[len++] = ch;
                        buffer[len] = '\0';
                    }
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawText(100, 100, prompt, font, white);
        drawText(100, 140, buffer, font, white);
        SDL_GL_SwapBuffers();
        SDL_Delay(10);
    }

    TTF_CloseFont(font);
    SDL_EnableUNICODE(0);
}


void create_new_world(TTF_Font* font, SDL_Color color) {
    char bufferSeed[64] = "";
    char bufferName[128] = "";

    getTextInput("Entrez une seed :", bufferSeed, sizeof(bufferSeed));
    getTextInput("Entrez un nom pour le monde :", bufferName, sizeof(bufferName));

    // Allocation dynamique globale
    free(seedString);
    seedString = malloc(strlen(bufferSeed) + 1);
    strcpy(seedString, bufferSeed);

    free(worldName);
    worldName = malloc(strlen(bufferName) + 1);
    strcpy(worldName, bufferName);

    init_perlin();
    add_world_name("worlds.txt", worldName);

    if (!c) c = malloc(sizeof(chunk));
    generate_chunk(c, 50, 50);
    save_chunk(c);
    generate_forest();
    generate_rock();
    generate_clouds();

    Vec3 pos = {
        CHUNK_SIZE,
        CHUNK_SIZE,
        get_ground_height_at(CHUNK_SIZE, CHUNK_SIZE) + 2.5f
    };
    save_player_state(pos);

    if (mode_fps)
        cam_pos = pos;
    else{
        // drone.position = pos;
    }
}

int displayWorldSelection(TTF_Font* font, SDL_Color color) {
    if (worldCount == 0) {
        printf("Aucun monde trouvé.\n");
        return -1;
    }
    int selected = 0;
    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) exit(EXIT_SUCCESS);
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_DOWN:
                        selected = (selected + 1) % worldCount;
                        break;
                    case SDLK_UP:
                        selected = (selected - 1 + worldCount) % worldCount;
                        break;
                    case SDLK_RETURN:
                        running = 0;
                        break;
                    case SDLK_ESCAPE:
                        return -1;
                }
            }
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawText(100, 50, "=== Selectionnez un monde ===", font, color);

        for (int i = 0; i < worldCount; i++) {
            if (!worldNames[i]) {
                printf("Erreur : worldNames[%d] est NULL\n", i);
                continue;
            }
            char line[256];
            snprintf(line, sizeof(line), "%s %s", (i == selected ? "->" : "  "), worldNames[i]);
            drawText(120, 100 + i * 30, line, font, color);
        }
        SDL_GL_SwapBuffers();
        SDL_Delay(10);
    }
    return selected;
}



int menu() {
    if (TTF_Init() < 0) {
        fprintf(stderr, "Erreur TTF_Init : %s\n", TTF_GetError());
        return 0;
    }

    TTF_Font* font = TTF_OpenFont("./arial.ttf", 24);
    if (!font) {
        fprintf(stderr, "Erreur TTF_OpenFont : %s\n", TTF_GetError());
        TTF_Quit();
        return 0;
    }

    SDL_Color white = {255, 255, 255, 255};
    int choix = 0;
    int running = 1;

    // === Menu principal ===
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) exit(EXIT_SUCCESS);
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_n: choix = 1; running = 0; break;
                    case SDLK_c: choix = 2; running = 0; break;
                    case SDLK_q: exitGame();
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawText(100, 100, "=== MENU ===", font, white);
        drawText(100, 140, "n. Nouvelle partie", font, white);
        drawText(100, 180, "c. Charger partie", font, white);
        drawText(100, 220, "q. Quitter", font, white);
        SDL_GL_SwapBuffers();
        SDL_Delay(10);
    }

    if (choix == 1) {
        create_new_world(font, white);
    }

    if (choix == 2) {
        load_world_names("worlds.txt");
        int selected = displayWorldSelection(font, white);
        if (selected >= 0 && selected < worldCount) {
        if (worldName) free(worldName); // sécurité si déjà alloué
        worldName = malloc(strlen(worldNames[selected]) + 1);
        if (!worldName) {
            fprintf(stderr, "Erreur d'allocation mémoire pour worldName\n");
            exit(EXIT_FAILURE);
        }
        strcpy(worldName, worldNames[selected]);
        Vec3 pos = load_player_state();  // Charge avant
        int chunkX = (int)(pos.x) / CHUNK_SIZE;
        int chunkY = (int)(pos.y) / CHUNK_SIZE;
        if (!c) c = malloc(sizeof(chunk));
        if (!load_chunk(c, chunkX, chunkY)) {
            generate_chunk(c, chunkX, chunkY);
            save_chunk(c);
            generate_forest();
            generate_rock();
            generate_clouds();
            }
        if (mode_fps)
            cam_pos = pos;
        else{
            // drone.position = pos;
        }
        }
    }

    updateChunks();
    float cx = CHUNK_SIZE;
    float cy = CHUNK_SIZE;
    float cz = get_ground_height_at(cx, cz) + 2.5f;
    if (mode_fps) {
        cam_pos.x = cx;
        cam_pos.y = cy;
        cam_pos.z = cz;
        cam_zaw = 0.0f;
        cam_pitch = 0.0f;
    } else {
        // drone.position.x = cx;
        // drone.position.y = cy;
        // drone.position.z = cz;
    }
    TTF_CloseFont(font);
    TTF_Quit();

    return (choix == 1 || choix == 2);
}



void update_camera_fps(const Uint8* keystate, double deltaTime) {
    float speed = 200.0f * deltaTime;
    Vec3 dir = {
        cosf(cam_pitch) * cosf(cam_zaw),
        sinf(cam_pitch),
        cosf(cam_pitch) * sinf(cam_zaw)
    };

    Vec3 right = {
        -sinf(cam_zaw),
        0.0f,
        cosf(cam_zaw)
    };

    Vec3 up = {0.0f, 1.0f, 0.0f};

    if (keystate[SDLK_z]) { cam_pos.x += dir.x * speed; cam_pos.z += dir.y * speed; cam_pos.z += dir.z * speed; }
    if (keystate[SDLK_s]) { cam_pos.x -= dir.x * speed; cam_pos.z -= dir.y * speed; cam_pos.z -= dir.z * speed; }
    if (keystate[SDLK_q]) { cam_pos.x -= right.x * speed; cam_pos.z -= right.y * speed; }
    if (keystate[SDLK_d]) { cam_pos.x += right.x * speed; cam_pos.z += right.y * speed; }
    if (keystate[SDLK_SPACE]) { cam_pos.z += speed; }
    if (keystate[SDLK_LCTRL]) { cam_pos.z -= speed; }
    if (keystate[SDLK_a]) cam_zaw -= 40.f * deltaTime;
    if (keystate[SDLK_e]) cam_zaw += 40.f * deltaTime;
    float ground = get_ground_height_at(cam_pos.x, cam_pos.z);
    if (cam_pos.z < ground + 2.0f) cam_pos.z = ground + 2.0f;
}

cameraState get_current_camera_position(void) {
    cameraState cam;
    if (mode_fps) {
        cam.x = cam_pos.x;
        cam.y = cam_pos.z;
        cam.z = cam_pos.z;
    } else {
        // cam.x = drone.position.x;
        // cam.y = drone.position.y;
        // cam.z = drone.position.z;
    }
    return cam;
}

void affichage(Drone* drone, float camRadius, float camPitch, float camZaw) {
    glViewport(0, 0, WIDTH, HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(70.0, (double)WIDTH / HEIGHT, 0.1, 1000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    float time = SDL_GetTicks();
    cameraState cam = get_current_camera_position();

    if (mode_fps) {
        Vec3 dir = {
            cosf(cam_pitch) * cosf(cam_zaw),
            sinf(cam_pitch),
            cosf(cam_pitch) * sinf(cam_zaw)
        };
        Vec3 up = {0.0f, 1.0f, 0.0f};

        gluLookAt(
            cam_pos.x, cam_pos.y, cam_pos.z,
            cam_pos.x + dir.x, cam_pos.y+ dir.y, cam_pos.z + dir.z,
            up.x, up.y, up.z
        );
    } else if (drone != NULL) {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // 2. calcul du yaw en radians
        float zaw_rad = drone->rot[1] * (M_PI / 180.0f);
        float fx = sinf(zaw_rad), fz = cosf(zaw_rad);

        // 3. position de la caméra
        float camX = drone->pos.x - fx * 1.f;
        float camY = drone->pos.y + 2.f;
        float camZ = drone->pos.z - fz * 2.f;

        // 4. point visé (on ajoute target_offset_y, typiquement négatif)
        float tgtX = drone->pos.x + fx;
        float tgtY = drone->pos.y + 0.2f;
        float tgtZ = drone->pos.z + fz;

        // 5. on appelle gluLookAt
        gluLookAt(
            camX,    camY,    camZ,
            tgtX,    tgtY,    tgtZ,
            0.0f,    1.0f,    0.0f
        );
    }

    draw_all_chunks(cam);
    draw_forest();
    draw_rock();
    draw_water(0.3f, time);
    draw_clouds(time);

    if (!mode_fps && drone != NULL) {
    // draw_velocity(Drone); 
        render_drone(drone, 3.0f, 0.1f, 0.5f); 
    }

    SDL_GL_SwapBuffers();
}


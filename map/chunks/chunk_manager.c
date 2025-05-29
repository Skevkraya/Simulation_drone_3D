#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../main/map_main.h"
#include "../main/map_types.h"
#include "../main/map_generals.h"
#include "../generation/perlin.h"
#include "chunk_manager.h"

void updateChunks(Drone* drone) {
    cameraState cam = get_current_camera_position(drone);
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

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            chunk* ch = loadedChunks[i][j];
            if (ch) {
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
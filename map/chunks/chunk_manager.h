#ifndef CHUNK_MANAGER_H
#define CHUNK_MANAGER_H

#include "../main/map_types.h"

int load_chunk(chunk* chunk, int cx, int cy);
void save_chunk(chunk* chunk);
void updateChunks(Drone* drone);
chunk* get_chunk_at_world(int x, int y);

#endif

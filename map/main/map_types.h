#ifndef MAP_TYPES_H
#define MAP_TYPES_H

// Constantes
#define WIDTH 900
#define HEIGHT 900
#define RENDER_DISTANCE 2
#define CHUNK_SIZE 64
#define MAP_WIDTH 200
#define MAP_HEIGHT 200
#define SCALE 0.04f
#define AMPLITUDE 20.0f
#define NB_TREES 100
#define NB_ROCKS 1000
#define NB_CLOUDS 1000
#define MOUNTAIN_LEVEL 0.6f
#define WATER_LEVEL 0.4f
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
typedef struct chunk chunk;
typedef struct tree tree;
typedef struct rocks rocks;
typedef struct clouds clouds;
typedef struct Node Node;
typedef struct cameraState cameraState;
struct chunk {
    int chunkX, chunkY;
    float heightmap[CHUNK_SIZE + 1][CHUNK_SIZE + 1];
};

struct tree {
    int x, z;
    float height_tree;
};

struct rocks {
    int x, z;
    float r;
};

struct clouds {
    int x, z;
    float r;
};

struct Node {
    int x, z;
    struct Node* left;
    struct Node* right;
};

struct cameraState {
    float x, y, z;
};

#endif

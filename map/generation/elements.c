#include <stdlib.h>
#include "../main/map_generals.h"
#include "../main/map_types.h"
#include "elements.h"

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
    Node* root = NULL;
    int count = 0;
    for (int i = 0; i < NB_CLOUDS; i++) {
        int x = rand() % CHUNK_SIZE;
        int z = rand() % CHUNK_SIZE;
        cloud_fields[i].r = 0.45f + (rand() % 10) / 100.0f;
        if(!search(root, x, z)){
            root = insert (root, x, z);
            cloud_fields[count].x = rand() % CHUNK_SIZE;
            cloud_fields[count].z = rand() % CHUNK_SIZE;
            count++;
        }
    }
}

const tree* get_forest() {
    return forest;
}
const rocks* get_rocks() {
    return rocks_place;
}
const clouds* get_clouds() {
    return cloud_fields;
}
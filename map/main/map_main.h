#ifndef MAP_MAIN_H
#define MAP_MAIN_H

#include "map_types.h"
#include "map_generals.h"
#include "../../drone.h"
#include <SDL/SDL.h>
#include "../chunks/chunk_manager.h"
#include "../generation/elements.h"
#include "../generation/perlin.h"
#include "../rendering/draw_chunk.h"
#include "../rendering/draw_elements.h"

cameraState get_current_camera_position(Drone* drone);
void update_camera_fps(const Uint8* keys, double dt);
void init_SDL(void);
void affichage(Drone* drone, float camRadius, float camPitch, float camYaw);
int menu(Drone* drone);
void load_world_names(const char* filename);
void add_world_name(const char* filename, const char* name);
void free_world_names();
Vec3 load_player_state();
void save_player_state(Vec3 pos);

#endif

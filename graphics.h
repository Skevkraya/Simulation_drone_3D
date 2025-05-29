#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "drone.h"

void init_SDL();
void render_drone(Drone *drone, float armLength, float armRadius, float diskRadius);
void render_ground();
void affichage(Drone * drone);

#endif  // GRAPHICS_H
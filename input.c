#include "lib.h"
#include "input.h"
#include "globals.h"
#include "./map/map_perlin.h"
#include "./math3d.h"

Control * control;

//       [0]         [1]
//      rotor0     rotor1
//        \         /
//         \       /
//          \     /
//           \   /
//            [X]
//           /   \
//          /     \
//     rotor3     rotor2
//     [3]         [2]
int mode_fps = 0;


void control_init() {
    control = malloc(sizeof(Control));
    if (!control) {
        fprintf(stderr, "Erreur d'allocation mémoire pour control\n");
        exit(EXIT_FAILURE);
    }

    control->pitch_setpoint = 0.0f;
    control->roll_setpoint = 0.0f;
    control->yaw_setpoint = 0.0f;
    control->altitude_setpoint = 0.0f;
    control->altitude_hold = 0;
}

void SDL_event_listener(Drone * drone, double dt) {
    SDL_Event event;
    const Uint8* keystates = SDL_GetKeyState(NULL);
    while(SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                // ─── PITCH ───────────────────────────────────────────────
                case SDLK_w:  // avancer = pitch avant
                    control->pitch_setpoint    = +30.0f;                       
                    break;
                case SDLK_s:  // reculer = pitch arrière
                    control->pitch_setpoint    = -30.0f;                
                    break;

                // ─── ROLL ────────────────────────────────────────────────
                case SDLK_a:  // roll gauche
                    control->roll_setpoint     = -20.0f;                
                    break;
                case SDLK_d:  // roll droite
                    control->roll_setpoint     = +20.0f;                
                    break;

                // ─── YAW ─────────────────────────────────────────────────
                case SDLK_q:  // yaw gauche
                    control->yaw_setpoint      += 15.0f;                
                    break;
                case SDLK_e:  // yaw droite
                    control->yaw_setpoint      -= 15.0f;                
                    break;

                // ─── ALTITUDE ────────────────────────────────────────────
                case SDLK_SPACE:  // monter
                    control->altitude_hold     = 1;                
                    control->altitude_setpoint = drone->pos.y + 0.9f;  
                    break;
                case SDLK_LCTRL:  // descendre
                    control->altitude_hold     = 1;                
                    control->altitude_setpoint = drone->pos.y - 0.9f;  
                    break;
                case SDLK_o: // Décollage
                        drone->rotorSpeed[0] = 0;
                    break;
                case SDLK_l:
                    mode_fps = !mode_fps;
                    SDL_ShowCursor(mode_fps ? SDL_DISABLE : SDL_ENABLE);
                    break;
                case SDLK_ESCAPE: running = 0; break;

            }
        } else if (event.type == SDL_KEYUP) {
            switch (event.key.keysym.sym) {
                case SDLK_w:
                case SDLK_s:
                    control->pitch_setpoint = 0.0f; 
                    break;
                case SDLK_a:
                case SDLK_d:
                    control->roll_setpoint = 0.0f;
                    break;
            }
        }
        if(event.type == SDL_QUIT) {
            running = 0;    
        }
    }
    if (mode_fps){
        update_camera_fps(keystates, dt);
        affichage(NULL, 0, 0, 0); // drone inutile ici
    }
    else{
        affichage(drone, 0, 0, -10.0);
    }
    // On clamp les vitesses des rotors entre 0 et 1
    clamp_rotorSpeed(drone);
}
#ifndef INPUT_H
#define INPUT_H

#include "drone.h"

#define ROTOR_ADJUST 0.01f

typedef struct {
    float pitch_setpoint;       // en % d’inclinaison avant/arrière (−1…+1)
    float roll_setpoint;        // en % d’inclinaison avant/arrière (−1…+1)
    float yaw_setpoint;         // en % d’inclinaison avant/arrière (−1…+1)
    int  altitude_hold;         // true pour maintenir l’altitude
    float altitude_setpoint;    // altitude cible (en m)
} Control;

extern Control * control;

void control_init();
void SDL_event_listener(Drone * drone, double dt);


#endif  // INPUT_H
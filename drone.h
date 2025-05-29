#ifndef DRONE_H
#define DRONE_H

#include "math3d.h"


typedef struct {
    Vec3 pos;         // Position (x, y, z)
    Vec3 vel;     // Vitesse
    Vec3 acc;     // Accélération
    Vec3 angularAcc;// Accélération angulaire
    Vec3 angularVel;    // Vitesse angulaire
    float angularDampingCoefficient;
    float rot[3];     // Rotation (pitch, yaw, roll)
    Vec3 torque;  // Couple angulaire
    float rotorSpeed[4];  // vitesses des 4 rotors (0 à 1 ou en tours/min)
    float rotorMaxThrust; // poussée maximale d’un rotor
    Vec3 force;    // Force linéaire totale (N)
    float mass;
    float momentOfInertia;
    float linearDragCoefficient;
} Drone;

// FONCTIONS
// Drone 
void drone_update(Drone * drone, double dt);
void init_drone(Drone * drone);
void clamp_rotorSpeed(Drone * drone);

// Calcul des forces
void apply_force(Drone* drone, float fx, float fy, float fz);
void apply_force_local(Drone* drone, float fx, float fy, float fz);

#endif  // DRONE_H
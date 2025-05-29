#include "lib.h"
#include "drone.h"
#include "input.h"
#include "pid.h"
#include "math3d.h"

PID pid_pitch, pid_roll, pid_yaw, pid_alt;

void init_drone(Drone * drone) {
    memset(drone, 0, sizeof(Drone));
    drone->pos.x = 0.0f;
    drone->pos.y = 0.0f;
    drone->pos.z = -5.0f;  // pour qu'on voie le drone au démarrage

    drone->mass = 1.0f;

    drone->rotorMaxThrust = 10.0f;

    drone->momentOfInertia = 10.0f;

    drone->linearDragCoefficient = 0.5f;
    drone->angularDampingCoefficient = 0.5f;
}

void clamp_rotorSpeed(Drone * drone) {
    for (int i = 0; i < 4; i++) {
        if (drone->rotorSpeed[i] < 0.0f) drone->rotorSpeed[i] = 0.0f;
        if (drone->rotorSpeed[i] > 1.0f) drone->rotorSpeed[i] = 1.0f;
    }
}

void apply_force(Drone* drone, float fx, float fy, float fz) {
    drone->force.x += fx;
    drone->force.y += fy;
    drone->force.z += fz;
}

void apply_force_local(Drone* drone, float fx, float fy, float fz) {
    float pitch_rad = drone->rot[0] * (M_PI / 180.0f);
    float yaw_rad   = drone->rot[1] * (M_PI / 180.0f);
    float roll_rad  = drone->rot[2] * (M_PI / 180.0f);

    // 1) start in local coords
    float rx = fx, ry = fy, rz = fz;
    float tmp;

    // 2) pitch around X
    {
      float c = cosf(pitch_rad), s = sinf(pitch_rad);
      tmp = ry*c - rz*s;
      rz  = ry*s + rz*c;
      ry  = tmp;
    }

    // 3) roll around Z
    {
      float c = cosf(roll_rad), s = sinf(roll_rad);
      tmp = rx*c - ry*s;
      ry  = rx*s + ry*c;
      rx  = tmp;
    }

    // 4) yaw around Y (last!)
    {
      float c = cosf(yaw_rad), s = sinf(yaw_rad);
      tmp = rx*c + rz*s;
      rz  = -rx*s + rz*c;
      rx  = tmp;
    }

    apply_force(drone, rx, ry, rz);
}

void drone_update(Drone * drone, double dt) { 
         // ─── 1) CALCUL DES EFFORTS PAR PID ────────────────────────────────────────
    // Pitch (tangage) autour de X
    float tau_p = pid_compute(&pid_pitch,
                              control->pitch_setpoint,   // consigne (−1…+1)
                              drone->rot[0],             // mesure actuelle (degrés)
                              dt);



    // Roll (roulis) autour de Z
    float tau_r = pid_compute(&pid_roll,
                              control->roll_setpoint,    // consigne (−1…+1)
                              drone->rot[2],             // mesure actuelle (degrés)
                              dt);

    // Yaw (lacet) autour de Y
    float tau_y = pid_compute(&pid_yaw,
                              control->yaw_setpoint,     // consigne (−1…+1)
                              drone->rot[1],             // mesure actuelle (degrés)
                              dt);

    // Altitude (force verticale)
    float Fz_pid = 0.0f;
    if (control->altitude_hold) {
        // PI(D) pour maintenir altitude
        Fz_pid = pid_compute(&pid_alt,
                             control->altitude_setpoint,  // consigne en m
                             drone->pos.y,               // hauteur actuelle
                             dt);
    }
    // on ajoute le poids pour compenser la gravité
    float Fz = Fz_pid + drone->mass * 9.81f;

    // ─── 2) MIXAGE ROTORS ─────────────────────────────────────────────────────
    // base = part d'altitude uniforme
    float base = Fz / (4.0f * drone->rotorMaxThrust);

    // chaque rotor reçoit altitude + contributions des couples
    drone->rotorSpeed[0] = base -  tau_p +  tau_r -  tau_y; // Decrease front-left for forward pitch
    drone->rotorSpeed[1] = base -  tau_p -  tau_r +  tau_y; // Decrease front-right for forward pitch
    drone->rotorSpeed[2] = base +  tau_p -  tau_r -  tau_y; // Increase rear-left for forward pitch
    drone->rotorSpeed[3] = base +  tau_p +  tau_r +  tau_y; // Increase rear-right for forward pitch

    // on borne bien entre 0 et 1
    clamp_rotorSpeed(drone);

    // On calcule le thrust
    float thrust[4];
    for (int i = 0; i < 4; i++) {
        thrust[i] = drone->rotorSpeed[i] * drone->rotorMaxThrust;
    }
    

    // Force totale de poussée 
    float totalThrust = thrust[0] + thrust[1] + thrust[2] + thrust[3];

    // On applique la force de poussée localement 
    apply_force_local(drone, 0.0f, totalThrust, 0.0f);

    // On applique les forces
    drone->torque.x = (thrust[2] + thrust[3]) - (thrust[0] + thrust[1]);
    drone->torque.y = -thrust[0] + thrust[1] - thrust[2] + thrust[3];
    drone->torque.z = (thrust[0] + thrust[3]) - (thrust[1] + thrust[2]); // (FL + RL) - (FR + RR)

    // TODO : Appliquer forces de torque
    drone->angularAcc.x = drone->torque.x / drone->momentOfInertia; // For pitch
    drone->angularAcc.y = drone->torque.y / drone->momentOfInertia; // For yaw
    drone->angularAcc.z = drone->torque.z / drone->momentOfInertia; // For roll

    drone->angularVel.x += drone->angularAcc.x * dt;
    drone->angularVel.y += drone->angularAcc.y * dt;
    drone->angularVel.z += drone->angularAcc.z * dt;

    // Atténuations angulaires
    drone->angularVel.x *= (1.0f - drone->angularDampingCoefficient * dt);
    drone->angularVel.y *= (1.0f - drone->angularDampingCoefficient * dt);
    drone->angularVel.z *= (1.0f - drone->angularDampingCoefficient * dt);

    // Integrate angular velocity into rotation (convert degrees to radians for sin/cos)
    drone->rot[0] += drone->angularVel.x * dt * (180.0f / M_PI); // Convert back to degrees
    drone->rot[1] += drone->angularVel.y * dt * (180.0f / M_PI);
    drone->rot[2] += drone->angularVel.z * dt * (180.0f / M_PI);

    for (int i = 0; i < 3; i++) {
        while (drone->rot[i] > 180.0f) {
            drone->rot[i] -= 360.0f;
        }
        while (drone->rot[i] < -180.0f) {
            drone->rot[i] += 360.0f;
        }
    }

    // Gravité
    apply_force(drone, 0.0f, -drone->mass * 9.81f, 0.0f);

    // Force de trainée
    float speedSq = drone->vel.x*drone->vel.x + drone->vel.y*drone->vel.y + drone->vel.z*drone->vel.z;
    float speed = sqrtf(speedSq);
    if (speed > 0.001f) { // Avoid division by zero with very small speeds
        float dragMagnitude = drone->linearDragCoefficient * speedSq;
        drone->force.x -= (drone->vel.x / speed) * dragMagnitude;
        drone->force.y -= (drone->vel.y / speed) * dragMagnitude;
        drone->force.z -= (drone->vel.z / speed) * dragMagnitude;
    }

    // Intégration physique
    // Calcul de l'accélération 
    drone->acc.x= drone->force.x / drone->mass;
    drone->acc.y = drone->force.y / drone->mass;
    drone->acc.z = drone->force.z / drone->mass;

    // On ajoute l'accélération à la vitesse
    drone->vel = vec3_add(drone->vel, vec3_scale(drone->acc, dt));
    // On ajoute la vitesse à la position
    drone->pos = vec3_add(drone->pos, vec3_scale(drone->vel, dt));


    // Vérifie la collision avec le sol
    if(drone->pos.y<=1)drone->pos.y=1;


    // On reset les forces pour la prochaine frame
    drone->force.x = 0;drone->force.y = 0;drone->force.z = 0;
    drone->torque.x = 0;drone->torque.y = 0;drone->torque.z = 0;
}
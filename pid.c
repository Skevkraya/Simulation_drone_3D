#include "pid.h"
#include "lib.h"


void init_pids() {
    // pour les essais, tu peux commencer par :
    // Kp = 1.0f, Ki = 0.1f, Kd = 0.01f puis ajuster
    pid_init(&pid_pitch, 0.01f, 0.00f, 0.005f);
    pid_init(&pid_roll,  0.01f, 0.000f, 0.005f);
    pid_init(&pid_yaw,   0.01f, 0.00f, 0.005f);
    pid_init(&pid_alt,   2.0f, 0.0f,  0.001f);  // plus agressif sur l'altitude
}

void pid_init(PID* pid, float Kp, float Ki, float Kd) {
    pid->Kp       = Kp;
    pid->Ki       = Ki;
    pid->Kd       = Kd;
    pid->integral = 0.0f;
    pid->prevError= 0.0f;
}

float pid_compute(PID* pid, float setpoint, float measured, float dt) {
    float error = setpoint - measured;

    // On ramène sur [-180, 180]
    error= fmodf(error, 360.0f);
    if (error >  180.0f) error -= 360.0f;
    if (error < -180.0f) error += 360.0f;

    pid->integral += error * dt;
    float derivative = (error - pid->prevError) / dt;
    pid->prevError = error;
    return pid->Kp * error
         + pid->Ki * pid->integral
         + pid->Kd * derivative;
}


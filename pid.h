#ifndef PID_H
#define PID_H

#include "pid.h"

typedef struct {
    float Kp, Ki, Kd;
    float integral, prevError;
} PID;

extern PID pid_pitch, pid_roll, pid_yaw, pid_alt;

void init_pids();
void pid_init(PID* pid, float Kp, float Ki, float Kd);
float pid_compute(PID* pid, float setpoint, float measured, float dt);

#endif  // PID_H
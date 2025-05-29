#ifndef MATH3D_H
#define MATH3D_H

typedef struct { double x, y, z; } Vec3;
typedef struct { double w, x, y, z; } Quaternion;

Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_scale(Vec3 v, float s);


#endif
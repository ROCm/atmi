#ifndef SPHERE_H
#define SPHERE_H

#define INF         2e10f

#define random(min, max)    (((double)rand() / ((size_t)RAND_MAX + 1)) * (max - min + 1) + min)
#define rnd(max)            random(0, max)

typedef struct _Color {
    double r, g, b;
} Color;

typedef struct _Material {
    double ns; // shininess
    double transp; // transparency
    double reflt; // reflection
    Color ambient;
    Color diffuse;
    Color specular;
} Material;

typedef struct _Sphere {
    double   x,y,z;
    double   radius,radius2;
    double   r,b,g;
    long idx;
    Material m;
} Sphere;

double hit(Sphere *s, double ox, double oy, double *n);
bool containsPoint(Sphere *s, double ox, double oy, double oz);

#endif

#include <cmath>
#include "Sphere.h"

double hit(Sphere *s, double ox, double oy, double *n) {
    double dx = ox - s->x;
    double dy = oy - s->y;
    double radius2 = s->radius2;
    if (dx*dx + dy*dy < radius2) {
        double dz = sqrtf((float)(radius2 - dx*dx - dy*dy));
        *n = dz / s->radius;
        return dz + s->z;
    }
    return (double)-INF;
}

bool containsPoint(Sphere *s, double ox, double oy, double oz) {
    double dx = s->x - ox;
    double dy = s->y - oy;
    //double dz = s-> - oz;
    double dz = 0;
    double radius2 = s->radius2;
    if((dx*dx + dy*dy + dz*dz) <= radius2)
        return true;
    else
        return false;
}

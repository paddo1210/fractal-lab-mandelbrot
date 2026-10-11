#pragma once
#include "Engine.h"
namespace fractal {
// Square complex-plane pixels, with imaginary axis increasing upwards.
struct Viewport {
    double x, y, unitsPerPixel;
    int width, height;
    Coordinate at(double px, double py) const noexcept {
        return {x + (px - width * 0.5) * unitsPerPixel,
                y - (py - height * 0.5) * unitsPerPixel};
    }
    Coordinate pixel(Coordinate c) const noexcept {
        return {width * 0.5 + (c.x-x)/unitsPerPixel,
                height * 0.5 - (c.y-y)/unitsPerPixel};
    }
};
inline Viewport viewport(const Settings& s, int w, int h) noexcept {
    return {s.x,s.y,3.0/(s.zoom*std::max(1,h)),w,h};
}
inline int escapeIterations(Coordinate c, int limit) noexcept {
    double x=0,y=0;
    for(int n=1;n<=limit;++n) {
        const double nx=x*x-y*y+c.x;
        y=2*x*y+c.y; x=nx;
        if(x*x+y*y>4) return n;
    }
    return 0; // unresolved at finite iteration budget
}
}

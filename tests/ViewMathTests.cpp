#include "ViewMath.h"
#include <iostream>
#include <stdexcept>
void check(bool value) { if(!value) throw std::runtime_error("View math check failed"); }
int main() {
    fractal::Settings s;
    check(fractal::escapeIterations({0,0},512)==0);
    check(fractal::escapeIterations({-1,0},512)==0);
    check(fractal::escapeIterations({2,0},64)==2);
    check(fractal::escapeIterations({3,0},64)==1);
    for(double zoom : {1.,10.,10000.}) {
        s.zoom=zoom;
        auto v=fractal::viewport(s,600,220);
        auto center=v.at(300,110);
        check(std::abs(center.x-s.x)<1e-12 && std::abs(center.y-s.y)<1e-12);
        for(int sampler : {0,1}) {
            s.sampler=sampler;
            for(int i=0;i<16;++i) {
                auto c=fractal::coordinate(s,i), p=v.pixel(c), restored=v.at(p.x,p.y);
                check(std::abs(c.x-restored.x)<1e-12 && std::abs(c.y-restored.y)<1e-12);
                if(sampler==0) check(std::abs(std::hypot(c.x-s.x,c.y-s.y)-s.radius/zoom)<1e-12);
                else check(c.y==s.y);
            }
        }
        check(v.at(0,0).y>v.at(0,220).y);
    }
    std::cout << "ViewMath: escape, axis orientation, aspect ratio and all 16 sample roundtrips passed\n";
}

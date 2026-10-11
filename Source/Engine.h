#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace fractal {
constexpr int steps=16;
inline double bounded(double v,double lo,double hi,double fallback) noexcept { return std::isfinite(v) ? std::clamp(v,lo,hi) : fallback; }
struct Settings {
    bool enabled=true,gen=true;
    double x=-0.6,y=0,zoom=1,radius=0.37;
    int sampler=0,iterations=128,root=48,scale=1,low=36,high=84;
    int pitchSource=0,velocitySource=1;
    double pitchAmount=0.5,rhythmAmount=0,velocityAmount=0.5,threshold=0.5,gate=0.65;
    int pitchSpan=7,velocitySpan=30,velocity=100;
    std::array<int,steps> degrees {0,2,4,6,4,2,1,3,0,2,5,6,5,3,2,1};
    std::array<int,steps> velocities {100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100};
    std::array<bool,steps> on {true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true};
};
inline Settings sanitize(Settings s) noexcept {
    s.x=bounded(s.x,-2,1,-0.6); s.y=bounded(s.y,-1.5,1.5,0);
    s.zoom=bounded(s.zoom,1,10000,1); s.radius=bounded(s.radius,0,1,0.37);
    s.iterations=std::clamp(s.iterations,64,512); s.sampler=std::clamp(s.sampler,0,1);
    s.root=std::clamp(s.root,0,127); s.scale=std::clamp(s.scale,0,3);
    s.low=std::clamp(s.low,0,127); s.high=std::clamp(s.high,s.low,127);
    s.pitchSource=std::clamp(s.pitchSource,0,3); s.velocitySource=std::clamp(s.velocitySource,0,2);
    s.pitchAmount=bounded(s.pitchAmount,0,1,0); s.rhythmAmount=bounded(s.rhythmAmount,0,1,0);
    s.velocityAmount=bounded(s.velocityAmount,0,1,0); s.threshold=bounded(s.threshold,0,1,0.5);
    s.gate=bounded(s.gate,0.05,0.95,0.65);
    s.pitchSpan=std::clamp(s.pitchSpan,0,14); s.velocitySpan=std::clamp(s.velocitySpan,0,63); s.velocity=std::clamp(s.velocity,1,127);
    for(int i=0;i<steps;++i) { s.degrees[i]=std::clamp(s.degrees[i],-14,28); s.velocities[i]=std::clamp(s.velocities[i],1,127); }
    return s;
}
struct Coordinate { double x,y; };
inline Coordinate coordinate(const Settings& s,int step) noexcept {
    const double r=s.radius/s.zoom;
    if(s.sampler==1) return {s.x+r*(-1+2.0*step/15),s.y};
    const double angle=6.283185307179586*step/16;
    return {s.x+r*std::cos(angle),s.y+r*std::sin(angle)};
}
struct Feature {
    double magnitude=0,movement=0,contrast=0,smooth=0;
    int escapeIndex=0,observed=0;
    bool escaped=false,smoothValid=false;
};
inline Feature analyze(Coordinate c,int limit) noexcept {
    Feature f; double x=0,y=0;
    limit=std::clamp(limit,1,512);
    if(!std::isfinite(c.x) || !std::isfinite(c.y) || std::abs(c.x)>4 || std::abs(c.y)>4) return f;
    for(int n=1;n<=limit;++n) {
        const double nx=x*x-y*y+c.x,ny=2*x*y+c.y;
        f.magnitude+=std::min((nx*nx+ny*ny)/4,1.0);
        f.movement+=std::min(std::hypot(nx-x,ny-y)/4,1.0);
        x=nx; y=ny; f.observed=n;
        if(x*x+y*y>4) { f.escaped=true; f.escapeIndex=n; break; }
    }
    f.magnitude/=f.observed; f.movement/=f.observed;
    if(f.escaped) {
        for(int i=0;i<2;++i) { const double nx=x*x-y*y+c.x; y=2*x*y+c.y; x=nx; }
        f.smooth=f.escapeIndex+3-std::log(std::log(std::hypot(x,y)))/std::log(2.0);
        f.smoothValid=std::isfinite(f.smooth);
    }
    return f;
}
inline int scalePitch(int root,int scale,int degree) noexcept {
    constexpr int intervals[4][7] {{0,1,2,3,4,5,6},{0,2,4,5,7,9,11},{0,2,3,5,7,8,10},{0,3,5,7,10,0,0}};
    const int n=scale==0 ? 12 : (scale==3 ? 5 : 7);
    int octave=degree/n,index=degree%n;
    if(index<0) { index+=n; --octave; }
    return root+octave*12+(scale==0 ? index : intervals[scale][index]);
}
inline bool inScale(int note,int root,int scale) noexcept {
    for(int d=0;d<(scale==0 ? 12 : (scale==3 ? 5 : 7));++d)
        if((note-root-scalePitch(0,scale,d))%12==0) return true;
    return false;
}
inline double pitchFeature(const Feature& f,int source,int limit) noexcept {
    if(source==0) return f.magnitude;
    if(source==1) return f.movement;
    if(source==2) return f.smoothValid ? std::clamp(std::log1p(std::max(f.smooth,0.0))/std::log(65.0),0.0,1.0) : 1.0;
    return f.escaped ? double(f.escapeIndex)/limit : 1.0;
}
struct Step { int pitch=48,velocity=100; bool on=false,baseProjected=false; };
struct Pattern {
    std::array<Step,steps> notes {};
    std::array<Feature,steps> features {};
    bool validRange=true,gen=true;
    double gate=0.65;
};
inline Pattern mapFeatures(Settings raw,const std::array<Feature,16>& features) noexcept {
    const auto s=sanitize(raw); Pattern p; p.gen=s.gen; p.gate=s.gate;
    std::array<int,128> allowed {}; int count=0;
    for(int note=s.low;note<=s.high;++note) if(inScale(note,s.root,s.scale)) allowed[count++]=note;
    if(count==0) { p.validRange=false; return p; }
    p.features=features;
    for(int i=0;i<steps;++i) {
        auto& f=p.features[i];
        f.contrast=(s.sampler==1 && i==0) ? 0 : std::min(std::abs(f.magnitude-p.features[(i+15)%16].magnitude)/0.05,1.0);
        auto& note=p.notes[i];
        const int base=s.gen ? s.root : scalePitch(s.root,s.scale,s.degrees[i]);
        int nearest=0;
        for(int j=1;j<count;++j) if(std::abs(allowed[j]-base)<std::abs(allowed[nearest]-base)) nearest=j;
        note.baseProjected=allowed[nearest]!=base;
        const double fPitch=pitchFeature(f,s.pitchSource,s.iterations);
        int idx;
        if(s.gen) idx=int(std::lround(nearest+s.pitchAmount*(std::lround((count-1)*fPitch)-nearest)));
        else idx=nearest+int(std::lround(s.pitchAmount*s.pitchSpan*(2*fPitch-1)));
        note.pitch=allowed[std::clamp(idx,0,count-1)];
        const double fVelocity=s.velocitySource==0 ? f.magnitude : (s.velocitySource==1 ? f.movement : f.contrast);
        note.velocity=std::clamp((s.gen ? s.velocity : s.velocities[i])+int(std::lround(s.velocityAmount*s.velocitySpan*(2*fVelocity-1))),1,127);
        note.on=s.on[i] && f.contrast>=s.rhythmAmount*s.threshold;
    }
    return p;
}
inline Pattern makePattern(Settings raw) noexcept {
    const auto s=sanitize(raw); std::array<Feature,16> features;
    for(int i=0;i<16;++i) features[i]=analyze(coordinate(s,i),s.iterations);
    return mapFeatures(s,features);
}
class Engine {
public:
    const Pattern& pattern() const noexcept { return current; }
    void requestReset() noexcept { resetRequested=true; }
    template<class Emit> void process(int samples,double rate,bool playing,double ppq,double bpm,const Settings& s,Emit emit) noexcept {
        processPrepared(samples,rate,playing,ppq,bpm,s,emit,[&](double){ return makePattern(s); });
    }
    template<class Emit,class Provider> void processPrepared(int samples,double rate,bool playing,double ppq,double bpm,const Settings& s,Emit emit,Provider provider) noexcept {
        if(samples<=0) return;
        const double delta=bpm/(60*rate);
        if(resetRequested) { off(0,emit); running=false; resetRequested=false; }
        if(!playing || !s.enabled || !std::isfinite(rate) || rate<=0 || !std::isfinite(bpm) || bpm<=0
           || !std::isfinite(ppq) || std::abs(ppq)>1e12 || !std::isfinite(delta) || delta<=0) {
            off(0,emit); running=false; return;
        }
        const bool jump=!running || rate!=previousRate || std::abs(ppq-expected)>std::max(2*delta,1e-7);
        if(jump) { off(0,emit); lastStep=std::numeric_limits<std::int64_t>::min(); current=provider(ppq); }
        running=true; previousRate=rate;
        for(int i=0;i<samples;++i) {
            const double beat=ppq+i*delta;
            if(active>=0 && beat+1e-10>=offBeat) off(i,emit);
            const auto absStep=static_cast<std::int64_t>(std::floor((beat+1e-10)*4));
            if(absStep==lastStep) continue;
            lastStep=absStep; off(i,emit);
            const int step=int((absStep%16+16)%16);
            if(step==0 && !(jump && i==0)) current=provider(beat);
            if(jump && i==0 && std::abs(beat-absStep*.25)>delta*.51) continue;
            const auto& note=current.notes[step];
            if(!note.on) continue;
            active=note.pitch; emit(true,active,note.velocity,i);
            offBeat=absStep*.25+.25*current.gate;
        }
        expected=ppq+samples*delta;
    }
private:
    template<class Emit> void off(int offset,Emit emit) noexcept { if(active>=0) emit(false,active,0,offset); active=-1; }
    Pattern current;
    bool running=false,resetRequested=false;
    int active=-1;
    double offBeat=0,expected=0,previousRate=0;
    std::int64_t lastStep=std::numeric_limits<std::int64_t>::min();
};
}

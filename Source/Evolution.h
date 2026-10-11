#pragma once
#include "Engine.h"
#include <atomic>
#include <memory>
#include <thread>

namespace fractal {
// Two fixed slots. Readers protect one slot; audio publishers never wait.
template<class T> class Exchange {
public:
    Exchange() : slots(new T[2]) {}
    template<class Fn> bool read(Fn fn) const noexcept {
        for(int attempt=0;attempt<3;++attempt) {
            int i=active.load(); readers[i].fetch_add(1);
            if(i!=active.load()) { readers[i].fetch_sub(1); continue; }
            fn(slots[i]); readers[i].fetch_sub(1); return true;
        }
        return false;
    }
    bool publish(const T& value) noexcept {
        int i=1-active.load(); if(readers[i].load()!=0) return false;
        slots[i]=value; active.store(i); return true;
    }
    // Single non-realtime writer only. Never call from processBlock.
    template<class Fn> void prepare(Fn fn) {
        int i=1-active.load(); while(readers[i].load()!=0) std::this_thread::yield();
        fn(slots[i]); active.store(i);
    }
private:
    std::unique_ptr<T[]> slots;
    mutable std::atomic<int> readers[2] {{0},{0}};
    std::atomic<int> active {0};
};

constexpr int flightFrames=2048;
constexpr double flightBeats=256.0, maxFlightZoom=10000.0;
struct FlightConfig { double driftX=.25,driftY=.1; int direction=1; };
inline Settings flightPosition(Settings base,double phase,FlightConfig cfg) noexcept {
    phase-=std::floor(phase);
    const double a=6.283185307179586*phase, wave=.5-.5*std::cos(a);
    const double initial=std::log10(std::clamp(base.zoom,1.0,maxFlightZoom));
    base.x+=.1*cfg.driftX/base.zoom*std::sin(a);
    base.y+=.1*cfg.driftY/base.zoom*(1-std::cos(a));
    base.zoom=std::pow(10.0,cfg.direction>0 ? initial+(4-initial)*wave : initial*(1-wave));
    base.x=std::clamp(base.x,-2.0,1.0); base.y=std::clamp(base.y,-1.5,1.5);
    return base;
}
struct FlightFrame { Settings math; Pattern candidate; };
struct FlightBank {
    std::array<FlightFrame,flightFrames> frames;
    Settings origin;
    FlightConfig config;
    uint64_t generation=0;
    void build(Settings s,FlightConfig cfg,uint64_t gen) noexcept {
        origin=sanitize(s); config=cfg; generation=gen;
        for(int i=0;i<flightFrames;++i) {
            frames[i].math=flightPosition(origin,double(i)/flightFrames,cfg);
            frames[i].candidate=makePattern(frames[i].math);
        }
    }
    const FlightFrame& frame(double phase) const noexcept {
        phase-=std::floor(phase);
        return frames[std::clamp(int(phase*flightFrames),0,flightFrames-1)];
    }
};
struct EvolutionControls {
    bool flight=false,mathMode=true,freeze=false,hold=false,evolve=true;
    double speed=1;
    int rate=8,depth=1;
};
struct FlightClock {
    double phase=0,lastBeat=0,lastSpeed=1,anchorBeat=0,anchorPhase=0;
    bool initialized=false,lastMoving=false;
    double at(double beat,const EvolutionControls& c) noexcept {
        const bool moving=c.flight && !c.freeze;
        if(!initialized) {
            phase=c.flight ? beat*c.speed/flightBeats : 0;anchorPhase=phase;anchorBeat=beat;initialized=true;
        } else if(beat<lastBeat-1e-7 || beat-lastBeat>4.0001) {
            if(moving) phase=beat*c.speed/flightBeats;
            anchorBeat=beat;anchorPhase=phase;
        } else {
            if(lastMoving) phase=anchorPhase+(beat-anchorBeat)*lastSpeed/flightBeats;
            if(moving!=lastMoving || c.speed!=lastSpeed) {anchorPhase=phase;anchorBeat=beat;}
        }
        lastBeat=beat;lastSpeed=c.speed;lastMoving=moving;
        phase-=std::floor(phase); return c.mathMode ? phase : 0;
    }
    void reset(double beat) noexcept { phase=anchorPhase=0;lastBeat=anchorBeat=beat;initialized=true; }
};
struct PatternState {
    Pattern pattern;
    Settings math;
    std::array<Coordinate,16> points {};
};
inline PatternState candidateState(const FlightFrame& frame,Settings musical) noexcept {
    musical.x=frame.math.x; musical.y=frame.math.y; musical.zoom=frame.math.zoom;
    musical.radius=frame.math.radius;musical.iterations=frame.math.iterations;musical.sampler=frame.math.sampler;
    PatternState p; p.math=musical;
    p.pattern=mapFeatures(musical,frame.candidate.features);
    for(int i=0;i<16;++i) p.points[i]=coordinate(musical,i);
    return p;
}
inline bool stepDifferent(const Step& a,const Step& b) noexcept {
    return a.pitch!=b.pitch || a.velocity!=b.velocity || a.on!=b.on;
}
inline int mutate(PatternState& current,const PatternState& candidate,int maximum) noexcept {
    std::array<int,16> order {}; std::array<double,16> scores {};
    for(int i=0;i<16;++i) {
        order[i]=i; auto a=current.pattern.features[i],b=candidate.pattern.features[i];
        scores[i]=std::abs(a.magnitude-b.magnitude)+std::abs(a.movement-b.movement)
            +std::abs(a.contrast-b.contrast)+std::abs(double(a.escapeIndex-b.escapeIndex))/512.;
    }
    // Stable insertion sort: equal scores favour the lower step index.
    for(int i=1;i<16;++i) { int j=i; while(j>0 && scores[order[j]]>scores[order[j-1]]) { std::swap(order[j],order[j-1]);--j; } }
    int changed=0;
    for(int idx:order) {
        if(changed>=maximum) break;
        if(!stepDifferent(current.pattern.notes[idx],candidate.pattern.notes[idx])) continue;
        current.pattern.notes[idx]=candidate.pattern.notes[idx];
        current.pattern.features[idx]=candidate.pattern.features[idx];
        current.points[idx]=candidate.points[idx]; ++changed;
    }
    current.math=candidate.math; return changed;
}
struct MemoryState { PatternState current,original,captured; bool hasCaptured=false,valid=false,clockMoving=false; double phase=0,clockBeat=0,clockSpeed=1,anchorBeat=0,anchorPhase=0; int64_t evolutionInterval=std::numeric_limits<int64_t>::min(); };
class Evolution {
public:
    MemoryState memory;
    FlightClock clock;
    int changed=0;
    double nextBeat=0;
    uint64_t generation=0;
    bool capturePending=false,recallPending=false,resetPending=false,flightResetPending=false;
    void capture() noexcept {
        if(initialized) { memory.captured=memory.current;memory.hasCaptured=true; }
        else capturePending=true;
    }
    void recall() noexcept { recallPending=true; }
    void reset() noexcept { resetPending=true; }
    void restore(MemoryState s) noexcept { memory=s;initialized=s.valid;restoredState=s.valid;clock.phase=s.phase;clock.lastBeat=s.clockBeat;clock.lastSpeed=s.clockSpeed;clock.lastMoving=s.clockMoving;clock.anchorBeat=s.anchorBeat;clock.anchorPhase=s.anchorPhase;clock.initialized=s.valid;previousBeat=s.clockBeat;lastInterval=s.evolutionInterval; }
    const Pattern& cycle(double beat,Settings musical,const EvolutionControls& c,const FlightBank& bank) noexcept {
        const double grid=std::round(beat*4)/4;
        if(std::abs(beat-grid)<1e-7) beat=grid;
        if(!initialized || generation!=bank.generation) {
            const bool preserve=restoredState;
            if(!preserve) memory.current=memory.original=candidateState(bank.frames[0],musical);
            restoredState=false;memory.valid=true;
            generation=bank.generation; initialized=true; if(!preserve) lastInterval=std::numeric_limits<int64_t>::min();
        }
        if(beat<previousBeat-1e-7 || beat-previousBeat>4.0001) memory.current=memory.hasCaptured ? memory.captured : memory.original;
        previousBeat=beat;
        if(flightResetPending) { clock.reset(beat);flightResetPending=false; }
        const double phase=clock.at(beat,c);
        if(capturePending) { memory.captured=memory.current; memory.hasCaptured=true;capturePending=false; }
        if(resetPending) { memory.current=memory.original;resetPending=false; }
        if(recallPending) { if(memory.hasCaptured) memory.current=memory.captured;recallPending=false; }
        const int64_t interval=int64_t(std::floor((beat+1e-8)/(4*c.rate)));
        nextBeat=(interval+1)*4.0*c.rate;
        const bool boundary=std::abs(beat-interval*4.0*c.rate)<1e-7;
        if(c.hold || !c.evolve) {
            if(c.hold && memory.hasCaptured) memory.current=memory.captured;
        } else if(!c.freeze && boundary && interval!=lastInterval) {
            auto candidate=candidateState(bank.frame(phase),musical);
            changed=mutate(memory.current,candidate,c.depth);
            // Gate follows only approved boundaries and OFF really changes nothing.
            if(c.depth>0) memory.current.pattern.gate=candidate.pattern.gate;
        }
        if(boundary) lastInterval=interval;
        memory.evolutionInterval=lastInterval;
        return memory.current.pattern;
    }
private:
    bool initialized=false,restoredState=false;
    int64_t lastInterval=std::numeric_limits<int64_t>::min();
    double previousBeat=0;
};
}

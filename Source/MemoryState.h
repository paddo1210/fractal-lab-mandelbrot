#pragma once
#include "Evolution.h"
#include <juce_data_structures/juce_data_structures.h>
namespace fractal {
inline juce::ValueTree savePattern(const char* name,const PatternState& p) {
    juce::ValueTree t(name);t.setProperty("gate",p.pattern.gate,nullptr);t.setProperty("valid",p.pattern.validRange,nullptr);t.setProperty("gen",p.pattern.gen,nullptr);
    auto s=p.math;juce::ValueTree m("Math");
#define SAVE(field) m.setProperty(#field,s.field,nullptr)
    SAVE(x);SAVE(y);SAVE(zoom);SAVE(radius);SAVE(iterations);SAVE(sampler);SAVE(root);SAVE(scale);SAVE(low);SAVE(high);SAVE(gen);SAVE(enabled);SAVE(pitchSource);SAVE(velocitySource);SAVE(pitchAmount);SAVE(rhythmAmount);SAVE(velocityAmount);SAVE(threshold);SAVE(gate);SAVE(pitchSpan);SAVE(velocitySpan);SAVE(velocity);
#undef SAVE
    t.addChild(m,-1,nullptr);
    for(int i=0;i<16;++i) {
        juce::ValueTree n("Step");auto note=p.pattern.notes[i];auto f=p.pattern.features[i];
        n.setProperty("pitch",note.pitch,nullptr);n.setProperty("velocity",note.velocity,nullptr);n.setProperty("on",note.on,nullptr);n.setProperty("projected",note.baseProjected,nullptr);
        n.setProperty("cx",p.points[i].x,nullptr);n.setProperty("cy",p.points[i].y,nullptr);
        n.setProperty("degree",s.degrees[i],nullptr);n.setProperty("baseVelocity",s.velocities[i],nullptr);n.setProperty("baseOn",s.on[i],nullptr);
#define SAVE(field) n.setProperty(#field,f.field,nullptr)
        SAVE(magnitude);SAVE(movement);SAVE(contrast);SAVE(smooth);SAVE(escapeIndex);SAVE(observed);SAVE(escaped);SAVE(smoothValid);
#undef SAVE
        t.addChild(n,-1,nullptr);
    }
    return t;
}
inline PatternState loadPattern(juce::ValueTree t) {
    PatternState p;auto& s=p.math;auto m=t.getChildWithName("Math");
#define LOAD(field,type) s.field=type(m.getProperty(#field,s.field))
    LOAD(x,double);LOAD(y,double);LOAD(zoom,double);LOAD(radius,double);LOAD(iterations,int);LOAD(sampler,int);LOAD(root,int);LOAD(scale,int);LOAD(low,int);LOAD(high,int);LOAD(gen,bool);LOAD(enabled,bool);LOAD(pitchSource,int);LOAD(velocitySource,int);LOAD(pitchAmount,double);LOAD(rhythmAmount,double);LOAD(velocityAmount,double);LOAD(threshold,double);LOAD(gate,double);LOAD(pitchSpan,int);LOAD(velocitySpan,int);LOAD(velocity,int);
#undef LOAD
    p.pattern.gate=bounded(double(t.getProperty("gate",.65)),.05,.95,.65);p.pattern.validRange=bool(t.getProperty("valid",true));p.pattern.gen=bool(t.getProperty("gen",true));
    int index=0;
    for(auto n:t) { if(!n.hasType("Step")) continue;if(index>=16) break;int i=index++;
        auto& note=p.pattern.notes[i];auto& f=p.pattern.features[i];
        note.pitch=std::clamp(int(n.getProperty("pitch",48)),0,127);note.velocity=std::clamp(int(n.getProperty("velocity",100)),1,127);note.on=bool(n.getProperty("on",false));note.baseProjected=bool(n.getProperty("projected",false));
        p.points[i]={bounded(double(n.getProperty("cx",0)),-4,4,0),bounded(double(n.getProperty("cy",0)),-4,4,0)};
        s.degrees[i]=int(n.getProperty("degree",0));s.velocities[i]=int(n.getProperty("baseVelocity",100));s.on[i]=bool(n.getProperty("baseOn",true));
        f.magnitude=bounded(double(n.getProperty("magnitude",0)),0,1,0);f.movement=bounded(double(n.getProperty("movement",0)),0,1,0);f.contrast=bounded(double(n.getProperty("contrast",0)),0,1,0);
        f.smooth=bounded(double(n.getProperty("smooth",0)),-1000,1000,0);f.escapeIndex=std::clamp(int(n.getProperty("escapeIndex",0)),0,512);f.observed=std::clamp(int(n.getProperty("observed",0)),0,512);f.escaped=bool(n.getProperty("escaped",false));f.smoothValid=bool(n.getProperty("smoothValid",false));
    }
    p.math=sanitize(s);return p;
}
inline juce::ValueTree saveMemory(const MemoryState& s) {
    juce::ValueTree t("PatternMemory");t.setProperty("hasCaptured",s.hasCaptured,nullptr);t.setProperty("valid",s.valid,nullptr);t.setProperty("phase",s.phase,nullptr);t.setProperty("clockBeat",s.clockBeat,nullptr);
    t.addChild(savePattern("Current",s.current),-1,nullptr);t.addChild(savePattern("Original",s.original),-1,nullptr);t.addChild(savePattern("Captured",s.captured),-1,nullptr);return t;
}
inline MemoryState loadMemory(juce::ValueTree t) {
    MemoryState s;s.hasCaptured=bool(t.getProperty("hasCaptured",false));s.valid=bool(t.getProperty("valid",false));s.phase=bounded(double(t.getProperty("phase",0)),0,1,0);s.clockBeat=bounded(double(t.getProperty("clockBeat",0)),-1e12,1e12,0);s.current=loadPattern(t.getChildWithName("Current"));s.original=loadPattern(t.getChildWithName("Original"));s.captured=loadPattern(t.getChildWithName("Captured"));return s;
}
}

#pragma once
#include "Evolution.h"
#include <juce_data_structures/juce_data_structures.h>
#include <cstring>
namespace fractal {
// JUCE XML numeric formatting does not guarantee an exact double roundtrip.
// Explicit eight-byte encodings are used only during non-realtime state I/O.
inline void saveDouble(juce::ValueTree& t,const char* name,double value) {
    juce::MemoryBlock bytes(&value,sizeof(value));t.setProperty(name,bytes.toBase64Encoding(),nullptr);
}
inline double loadDouble(juce::ValueTree t,const char* name,double fallback=0) {
    auto property=t.getProperty(name);if(property.isVoid()) return fallback;
    if(!property.isString()) return double(property);
    juce::MemoryBlock bytes;if(!bytes.fromBase64Encoding(property.toString()) || bytes.getSize()!=sizeof(double)) return fallback;
    double value;std::memcpy(&value,bytes.getData(),sizeof(value));return value;
}
inline juce::ValueTree savePattern(const char* name,const PatternState& p) {
    juce::ValueTree t(name);saveDouble(t,"gate",p.pattern.gate);t.setProperty("valid",p.pattern.validRange,nullptr);t.setProperty("gen",p.pattern.gen,nullptr);
    auto s=p.math;juce::ValueTree m("Math");
#define SAVE(field) m.setProperty(#field,s.field,nullptr)
    SAVE(iterations);SAVE(sampler);SAVE(root);SAVE(scale);SAVE(low);SAVE(high);SAVE(gen);SAVE(enabled);SAVE(pitchSource);SAVE(velocitySource);SAVE(pitchSpan);SAVE(velocitySpan);SAVE(velocity);
#undef SAVE
#define SAVE(field) saveDouble(m,#field,s.field)
    SAVE(x);SAVE(y);SAVE(zoom);SAVE(radius);SAVE(pitchAmount);SAVE(rhythmAmount);SAVE(velocityAmount);SAVE(threshold);SAVE(gate);
#undef SAVE
    t.addChild(m,-1,nullptr);
    for(int i=0;i<16;++i) {
        juce::ValueTree n("Step");auto note=p.pattern.notes[i];auto f=p.pattern.features[i];
        n.setProperty("pitch",note.pitch,nullptr);n.setProperty("velocity",note.velocity,nullptr);n.setProperty("on",note.on,nullptr);n.setProperty("projected",note.baseProjected,nullptr);
        saveDouble(n,"cx",p.points[i].x);saveDouble(n,"cy",p.points[i].y);
        n.setProperty("degree",s.degrees[i],nullptr);n.setProperty("baseVelocity",s.velocities[i],nullptr);n.setProperty("baseOn",s.on[i],nullptr);
#define SAVE(field) n.setProperty(#field,f.field,nullptr)
        SAVE(escapeIndex);SAVE(observed);SAVE(escaped);SAVE(smoothValid);
#undef SAVE
#define SAVE(field) saveDouble(n,#field,f.field)
        SAVE(magnitude);SAVE(movement);SAVE(contrast);SAVE(smooth);
#undef SAVE
        t.addChild(n,-1,nullptr);
    }
    return t;
}
inline PatternState loadPattern(juce::ValueTree t) {
    PatternState p;auto& s=p.math;auto m=t.getChildWithName("Math");
#define LOAD(field,type) s.field=type(m.getProperty(#field,s.field))
    LOAD(iterations,int);LOAD(sampler,int);LOAD(root,int);LOAD(scale,int);LOAD(low,int);LOAD(high,int);LOAD(gen,bool);LOAD(enabled,bool);LOAD(pitchSource,int);LOAD(velocitySource,int);LOAD(pitchSpan,int);LOAD(velocitySpan,int);LOAD(velocity,int);
#undef LOAD
#define LOAD(field) s.field=loadDouble(m,#field,s.field)
    LOAD(x);LOAD(y);LOAD(zoom);LOAD(radius);LOAD(pitchAmount);LOAD(rhythmAmount);LOAD(velocityAmount);LOAD(threshold);LOAD(gate);
#undef LOAD
    p.pattern.gate=bounded(loadDouble(t,"gate",.65),.05,.95,.65);p.pattern.validRange=bool(t.getProperty("valid",true));p.pattern.gen=bool(t.getProperty("gen",true));
    int index=0;
    for(auto n:t) { if(!n.hasType("Step")) continue;if(index>=16) break;int i=index++;
        auto& note=p.pattern.notes[i];auto& f=p.pattern.features[i];
        note.pitch=std::clamp(int(n.getProperty("pitch",48)),0,127);note.velocity=std::clamp(int(n.getProperty("velocity",100)),1,127);note.on=bool(n.getProperty("on",false));note.baseProjected=bool(n.getProperty("projected",false));
        p.points[i]={bounded(loadDouble(n,"cx"),-4,4,0),bounded(loadDouble(n,"cy"),-4,4,0)};
        s.degrees[i]=int(n.getProperty("degree",0));s.velocities[i]=int(n.getProperty("baseVelocity",100));s.on[i]=bool(n.getProperty("baseOn",true));
        f.magnitude=bounded(loadDouble(n,"magnitude"),0,1,0);f.movement=bounded(loadDouble(n,"movement"),0,1,0);f.contrast=bounded(loadDouble(n,"contrast"),0,1,0);
        f.smooth=bounded(loadDouble(n,"smooth"),-1000,1000,0);f.escapeIndex=std::clamp(int(n.getProperty("escapeIndex",0)),0,512);f.observed=std::clamp(int(n.getProperty("observed",0)),0,512);f.escaped=bool(n.getProperty("escaped",false));f.smoothValid=bool(n.getProperty("smoothValid",false));
    }
    p.math=sanitize(s);return p;
}
inline juce::ValueTree saveMemory(const MemoryState& s) {
    juce::ValueTree t("PatternMemory");t.setProperty("hasCaptured",s.hasCaptured,nullptr);t.setProperty("valid",s.valid,nullptr);saveDouble(t,"phase",s.phase);saveDouble(t,"clockBeat",s.clockBeat);
    saveDouble(t,"clockSpeed",s.clockSpeed);t.setProperty("clockMoving",s.clockMoving,nullptr);t.setProperty("evolutionInterval",juce::int64(s.evolutionInterval),nullptr);
    saveDouble(t,"anchorBeat",s.anchorBeat);saveDouble(t,"anchorPhase",s.anchorPhase);
    t.addChild(savePattern("Current",s.current),-1,nullptr);t.addChild(savePattern("Original",s.original),-1,nullptr);t.addChild(savePattern("Captured",s.captured),-1,nullptr);return t;
}
inline MemoryState loadMemory(juce::ValueTree t) {
    MemoryState s;s.hasCaptured=bool(t.getProperty("hasCaptured",false));s.valid=bool(t.getProperty("valid",false));s.phase=bounded(loadDouble(t,"phase"),0,1,0);s.clockBeat=bounded(loadDouble(t,"clockBeat"),-1e12,1e12,0);s.clockSpeed=bounded(loadDouble(t,"clockSpeed",1),.05,4,1);s.clockMoving=bool(t.getProperty("clockMoving",false));s.anchorBeat=bounded(loadDouble(t,"anchorBeat"),-1e12,1e12,0);s.anchorPhase=bounded(loadDouble(t,"anchorPhase"),-1e12,1e12,0);s.evolutionInterval=juce::int64(t.getProperty("evolutionInterval",juce::int64(std::numeric_limits<int64_t>::min())));s.current=loadPattern(t.getChildWithName("Current"));s.original=loadPattern(t.getChildWithName("Original"));s.captured=loadPattern(t.getChildWithName("Captured"));return s;
}
}

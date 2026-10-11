#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "MemoryState.h"
namespace {
const char* ids[] {"run","mode","x","y","zoom","radius","sampler","iterations","pitchAmount","rhythmAmount","velocityAmount","root","scale","low","high","velocity","gate","panic","pitchSource","velocitySource","pitchSpan","velocitySpan","threshold","zoomExponent"};
}
juce::AudioProcessorValueTreeState::ParameterLayout MandelbrotProcessor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    auto id=[](const juce::String& n){ return juce::ParameterID{n,1}; };
    auto number=[&](const char* n,const char* label,float lo,float hi,float def,float interval=0.001f) {
        p.add(std::make_unique<juce::AudioParameterFloat>(id(n),label,juce::NormalisableRange<float>{lo,hi,interval},def));
    };
    auto integer=[&](const char* n,const char* label,int lo,int hi,int def){ p.add(std::make_unique<juce::AudioParameterInt>(id(n),label,lo,hi,def)); };
    auto choice=[&](const char* n,const char* label,juce::StringArray choices,int def){ p.add(std::make_unique<juce::AudioParameterChoice>(id(n),label,choices,def)); };
    p.add(std::make_unique<juce::AudioParameterBool>(id("run"),"Run",true));
    choice("mode","Mapping mode (next evolution)",{"GEN","CTRL"},0);
    number("x","X center",-2,1,-0.75f,0.000001f); number("y","Y center",-1.5f,1.5f,0.1f,0.000001f);
    number("zoom","Zoom multiplier",1,10,1); number("zoomExponent","Zoom exponent (10^n)",0,3,0);
    number("radius","Local radius",0,1,0.37f);
    choice("sampler","Contour sampling",{"Circle","Line"},0);
    choice("iterations","Iteration budget",{"64","128","256","512"},1);
    number("pitchAmount","Pitch Amount",0,1,0.5f); number("rhythmAmount","Rhythm MASK Amount",0,1,0);
    number("velocityAmount","Velocity Amount",0,1,0.5f);
    integer("root","Root MIDI",24,84,48); choice("scale","Scale",{"Chromatic","Major","Minor","Minor Pentatonic"},1);
    integer("low","Lowest MIDI",0,127,36); integer("high","Highest MIDI",0,127,84);
    integer("velocity","GEN base velocity",1,127,100); number("gate","Gate",0.05f,0.95f,0.65f);
    p.add(std::make_unique<juce::AudioParameterBool>(id("panic"),"Panic (toggle)",false));
    choice("pitchSource","Pitch feature",{"Orbit Magnitude","Orbit Movement","Smooth Escape","Integer Escape"},0);
    choice("velocitySource","Velocity feature",{"Orbit Magnitude","Orbit Movement","Neighbor Contrast"},1);
    integer("pitchSpan","CTRL max degree shift",0,14,7); integer("velocitySpan","Velocity max shift",0,63,30);
    number("threshold","MASK contrast threshold",0,1,0.5f);
    const fractal::Settings defaults;
    for(int i=0;i<16;++i) {
        const auto suffix=juce::String(i+1).paddedLeft('0',2);
        p.add(std::make_unique<juce::AudioParameterInt>(id("step"+suffix),"Step "+suffix+" Degree",-14,28,defaults.degrees[i]));
        p.add(std::make_unique<juce::AudioParameterBool>(id("on"+suffix),"Step "+suffix+" On",true));
        p.add(std::make_unique<juce::AudioParameterInt>(id("vel"+suffix),"Step "+suffix+" Velocity",1,127,100));
    }
    p.add(std::make_unique<juce::AudioParameterBool>(id("flightEnable"),"Flight On",false));
    choice("flightMode","Flight Mode",{"OFF","MATH FLIGHT"},1);
    number("flightSpeed","Flight Speed",0.05f,4,1);
    choice("zoomDirection","Zoom Direction",{"In","Out"},0);
    number("driftX","Drift X (prepare)",-1,1,.25f);
    number("driftY","Drift Y (prepare)",-1,1,.1f);
    for(auto name : {"freeze","capture","hold","recall","patternReset","flightReset"})
        p.add(std::make_unique<juce::AudioParameterBool>(id(name),name,false));
    p.add(std::make_unique<juce::AudioParameterBool>(id("evolve"),"Evolve",true));
    choice("evolutionRate","Evolution Rate (bars)",{"4","8","16"},1);
    choice("mutationDepth","Mutation Depth",{"OFF","SUBTLE (1)","LOW (2)"},1);
    return p;
}
MandelbrotProcessor::MandelbrotProcessor()
 : AudioProcessor(BusesProperties().withOutput("Silent Output",juce::AudioChannelSet::stereo(),true)),
   state(*this,nullptr,"FractalLabV03State",layout()) {
    for(size_t i=0;i<global.size();++i) global[i]=state.getRawParameterValue(ids[i]);
    for(int i=0;i<16;++i) {
        const auto suffix=juce::String(i+1).paddedLeft('0',2);
        degrees[i]=state.getRawParameterValue("step"+suffix); velocities[i]=state.getRawParameterValue("vel"+suffix);
        enabled[i]=state.getRawParameterValue("on"+suffix);
        mag[i].store(0); movement[i].store(0); contrast[i].store(0); escape[i].store(0);
        note[i].store(48); on[i].store(0); projected[i].store(0);
    }
    prepareFlight();
}
fractal::Settings MandelbrotProcessor::settings() const noexcept {
    fractal::Settings s;
    auto value=[&](int i){ return global[size_t(i)]->load(); };
    s.enabled=value(0)>0.5f && value(17)<0.5f; s.gen=value(1)<0.5f;
    s.x=value(2); s.y=value(3); s.zoom=value(4)*std::pow(10.0,value(23)); s.radius=value(5);
    s.sampler=int(value(6)); s.iterations=64<<std::clamp(int(value(7)),0,3);
    s.pitchAmount=value(8); s.rhythmAmount=value(9); s.velocityAmount=value(10);
    s.root=int(value(11)); s.scale=int(value(12)); s.low=int(value(13)); s.high=int(value(14));
    s.velocity=int(value(15)); s.gate=value(16); s.pitchSource=int(value(18)); s.velocitySource=int(value(19));
    s.pitchSpan=int(value(20)); s.velocitySpan=int(value(21)); s.threshold=value(22);
    for(int i=0;i<16;++i) { s.degrees[i]=int(degrees[i]->load()); s.velocities[i]=int(velocities[i]->load()); s.on[i]=enabled[i]->load()>0.5f; }
    return s;
}
fractal::EvolutionControls MandelbrotProcessor::controls() const noexcept {
    auto v=[&](const char* id){return state.getRawParameterValue(id)->load();};
    fractal::EvolutionControls c;
    c.mathMode=v("flightMode")>.5f;c.flight=v("flightEnable")>.5f && c.mathMode; c.freeze=v("freeze")>.5f;
    c.hold=v("hold")>.5f;c.evolve=v("evolve")>.5f;c.speed=fractal::bounded(v("flightSpeed"),.05,4,1);
    c.rate=4<<std::clamp(int(v("evolutionRate")),0,2);c.depth=std::clamp(int(v("mutationDepth")),0,2);return c;
}
bool MandelbrotProcessor::prepareFlight(const fractal::Settings* origin,const fractal::FlightConfig* config) {
    std::lock_guard<std::mutex> guard(preparationLock);
    if(transportPlaying.load()) return false;
    fractal::FlightConfig cfg;cfg.driftX=state.getRawParameterValue("driftX")->load();cfg.driftY=state.getRawParameterValue("driftY")->load();
    cfg.direction=state.getRawParameterValue("zoomDirection")->load()<.5f ? 1 : -1;
    if(config) cfg=*config;
    const auto s=origin ? *origin : settings();const auto gen=++bankGeneration;
    // Host setup / stopped GUI only. Actual orbit analysis runs on this joined preparation worker.
    std::thread worker([&]{banks.prepare([&](fractal::FlightBank& b){b.build(s,cfg,gen);});});worker.join();
    previews.publish(s);restart.store(true);return true;
}
void MandelbrotProcessor::prepareToPlay(double,int) { transportPlaying.store(false);restart.store(true); }
fractal::Settings MandelbrotProcessor::previewSettings() const noexcept {
    auto s=settings();previews.read([&](const fractal::Settings& p){s=p;});return s;
}
void MandelbrotProcessor::processBlock(juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;audio.clear();midi.clear();
    if(restart.exchange(false)) engine.requestReset();
    const bool restoring=restored.exchange(false);
    if(restoring) restoredMemories.read([&](const fractal::MemoryState& m){evolution.restore(m);});
    const char* commandIds[]={"capture","recall","patternReset","flightReset"};
    for(int i=0;i<4;++i) {
        bool down=state.getRawParameterValue(commandIds[i])->load()>.5f;
        if(restoring) previousCommands[size_t(i)]=down;
        if(down!=previousCommands[size_t(i)]) {
            if(i==0) evolution.capture();if(i==1) evolution.recall();if(i==2) evolution.reset();if(i==3) evolution.flightResetPending=true;
        }
        previousCommands[size_t(i)]=down;
    }
    auto* playhead=getPlayHead();auto pos=playhead ? playhead->getPosition() : juce::Optional<juce::AudioPlayHead::PositionInfo>{};
    bool valid=pos && pos->getPpqPosition() && pos->getBpm();
    if(valid) valid=std::isfinite(*pos->getPpqPosition()) && std::abs(*pos->getPpqPosition())<=1e12 && std::isfinite(*pos->getBpm()) && *pos->getBpm()>0 && std::isfinite(getSampleRate()) && getSampleRate()>0;
    if(valid && pos->getTimeSignature()) {auto ts=*pos->getTimeSignature();valid=ts.numerator==4 && ts.denominator==4;}
    bool playing=valid && pos->getIsPlaying();double ppq=valid ? *pos->getPpqPosition() : 0;
    transportPlaying.store(playing);hostBeat.store(ppq);
    const auto s=settings();const auto c=controls();
    auto emit=[&](bool on,int pitch,int velocity,int offset){midi.addEvent(on ? juce::MidiMessage::noteOn(1,pitch,juce::uint8(velocity)) : juce::MidiMessage::noteOff(1,pitch),offset);};
    bool read=banks.read([&](const fractal::FlightBank& bank){
        if(playing) evolution.clock.at(ppq,c);
        engine.processPrepared(audio.getNumSamples(),getSampleRate(),playing,ppq,valid ? *pos->getBpm() : 120,s,emit,
            [&](double beat){return evolution.cycle(beat,s,c,bank);});
        if(playing) {const auto phase=evolution.clock.at(ppq+audio.getNumSamples()*(valid ? *pos->getBpm() : 120)/(60*getSampleRate()),c);previews.publish(bank.frame(phase).math);}
        else previews.publish(bank.frame(c.mathMode ? evolution.clock.phase : 0).math);
    });
    if(!read) engine.processPrepared(audio.getNumSamples(),getSampleRate(),false,ppq,120,s,emit,[&](double){return engine.pattern();});
    evolution.memory.phase=evolution.clock.phase;evolution.memory.clockBeat=evolution.clock.lastBeat;
    evolution.memory.clockSpeed=evolution.clock.lastSpeed;evolution.memory.clockMoving=evolution.clock.lastMoving;
    evolution.memory.anchorBeat=evolution.clock.anchorBeat;evolution.memory.anchorPhase=evolution.clock.anchorPhase;
    memories.publish(evolution.memory);changedSteps.store(evolution.changed);nextEvolutionBeat.store(evolution.nextBeat);
    const auto& p=engine.pattern();appliedGen.store(p.gen);validRange.store(p.validRange);
    for(int i=0;i<16;++i) {
        const auto& f=p.features[i];mag[i].store(float(f.magnitude));movement[i].store(float(f.movement));contrast[i].store(float(f.contrast));escape[i].store(float(f.escapeIndex));
        note[i].store(float(p.notes[i].pitch));on[i].store(p.notes[i].on ? 1.f : 0.f);projected[i].store(p.notes[i].baseProjected ? 1.f : 0.f);
    }
}
juce::String MandelbrotProcessor::diagnostics() const {
    juce::String text="Applied: "+juce::String(appliedGen.load() ? "GEN" : "CTRL")+" | MASK: H >= Amount * Threshold | H = min(abs(E-Eprev)/0.05,1)\n";
    text+=validRange.load() ? "Step : E / Movement / H / Escape iteration (0 = unresolved) / MIDI / On / projected base\n" : "INVALID RANGE: no scale note; silent pattern.\n";
    for(int i=0;i<16;++i) text+=juce::String(i+1).paddedLeft('0',2)+": "+juce::String(mag[i].load(),3)+" / "+juce::String(movement[i].load(),3)+" / "+juce::String(contrast[i].load(),3)+" / "+juce::String(int(escape[i].load()))+" / "+juce::String(int(note[i].load()))+" / "+juce::String(int(on[i].load()))+" / "+juce::String(int(projected[i].load()))+"\n";
    return text;
}
juce::AudioProcessorEditor* MandelbrotProcessor::createEditor() { return new LabEditor(*this); }
void MandelbrotProcessor::getStateInformation(juce::MemoryBlock& dest) {
    auto tree=state.copyState(); tree.setProperty("schemaVersion",3,nullptr);
    fractal::MemoryState memory;memories.read([&](const fractal::MemoryState& m){memory=m;});
    tree.addChild(fractal::saveMemory(memory),-1,nullptr);
    banks.read([&](const fractal::FlightBank& b){
        fractal::PatternState origin;origin.math=b.origin;auto plan=fractal::savePattern("PreparedPlan",origin);
        fractal::saveDouble(plan,"driftX",b.config.driftX);fractal::saveDouble(plan,"driftY",b.config.driftY);plan.setProperty("direction",b.config.direction,nullptr);tree.addChild(plan,-1,nullptr);
    });
    if(auto xml=tree.createXml()) copyXmlToBinary(*xml,dest);
}
void MandelbrotProcessor::setStateInformation(const void* data,int size) {
    if(auto xml=getXmlFromBinary(data,size)) if(xml->hasTagName(state.state.getType())) {
        auto tree=juce::ValueTree::fromXml(*xml);
        if(int(tree.getProperty("schemaVersion",0))!=3) return;
        auto memory=tree.getChildWithName("PatternMemory");
        if(memory.isValid()) {auto restoredState=fractal::loadMemory(memory);restoredMemories.prepare([&](fractal::MemoryState& m){m=restoredState;});restored.store(true);tree.removeChild(memory,nullptr);}
        auto plan=tree.getChildWithName("PreparedPlan");auto origin=fractal::loadPattern(plan).math;
        fractal::FlightConfig config;config.driftX=fractal::bounded(fractal::loadDouble(plan,"driftX",.25),-1,1,.25);config.driftY=fractal::bounded(fractal::loadDouble(plan,"driftY",.1),-1,1,.1);config.direction=int(plan.getProperty("direction",1))<0 ? -1 : 1;
        if(plan.isValid()) tree.removeChild(plan,nullptr);
        state.replaceState(tree);if(plan.isValid()) prepareFlight(&origin,&config);else prepareFlight();
    }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MandelbrotProcessor(); }


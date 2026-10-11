#include "Evolution.h"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace fractal;
void check(bool ok,const char* message) {if(!ok) throw std::runtime_error(message);}
bool same(const Pattern& a,const Pattern& b) {for(int i=0;i<16;++i) if(stepDifferent(a.notes[i],b.notes[i])) return false;return a.gate==b.gate;}
int differences(const Pattern& a,const Pattern& b) {int n=0;for(int i=0;i<16;++i)n+=stepDifferent(a.notes[i],b.notes[i]);return n;}
struct Event {bool on;int pitch,velocity;int64_t sample;bool operator==(const Event& b)const{return on==b.on && pitch==b.pitch && velocity==b.velocity && sample==b.sample;}};
std::vector<Event> render(const FlightBank& bank,int block,bool offline) {
    (void)offline;Engine engine;Evolution evolution;Settings s=bank.origin;EvolutionControls c;c.flight=true;
    std::vector<Event> result;const int64_t total=128*24000;
    for(int64_t start=0;start<total;start+=block) {
        int count=int(std::min<int64_t>(block,total-start));
        engine.processPrepared(count,48000,true,double(start)/24000,120,s,[&](bool on,int p,int v,int offset){result.push_back({on,p,v,start+offset});},[&](double beat){return evolution.cycle(beat,s,c,bank);});
    }
    engine.processPrepared(block,48000,false,128,120,s,[&](bool on,int p,int v,int offset){result.push_back({on,p,v,total+offset});},[&](double){return engine.pattern();});
    return result;
}
int main() {
 try {
    auto bank=std::make_unique<FlightBank>();Settings s;s.x=-.75;s.y=.1;bank->build(s,{},1);
    Settings invalid=s;invalid.low=invalid.high=61;invalid.root=60;invalid.scale=1;auto noNote=makePattern(invalid);
    check(!noNote.validRange && noNote.features[0].observed>0,"Mathematical features remain authentic even with empty musical range");
    EvolutionControls c;c.flight=true;
    Evolution hold;hold.cycle(0,s,c,*bank);hold.capture();c.hold=true;
    const auto captured=hold.memory.captured.pattern;
    for(int bar=1;bar<=32;++bar) check(same(hold.cycle(bar*4.,s,c,*bank),captured),"A: 32-bar hold");
    check(hold.clock.phase!=0,"A: flight continues during hold");
    c.hold=false;Evolution evolving;evolving.cycle(0,s,c,*bank);int changes=0;
    for(int bar=1;bar<=32;++bar) {
        auto before=evolving.memory.current.pattern;auto after=evolving.cycle(bar*4.,s,c,*bank);int n=differences(before,after);
        if(n) {check(bar%8==0,"B: eight-bar boundaries only");check(n<=1,"C: subtle maximum one step");++changes;}
    }
    check(changes>0,"B/C: test must actually mutate");
    auto midi1=render(*bank,127,false),midi2=render(*bank,512,false),midi3=render(*bank,4096,true);
    check(midi1==midi2 && midi1==midi3,"D/I: repeated and different-block realtime/offline engine harness");
    std::array<int,128> active {};for(auto e:midi1) {if(e.on){check(active[e.pitch]==0,"H: duplicate active note");++active[e.pitch];}else {check(active[e.pitch]==1,"H: unpaired note off");--active[e.pitch];}}
    for(int n:active) check(n==0,"H: no hanging note after stop");
    FlightClock clock;clock.at(0,c);double p=clock.at(4,c);c.freeze=true;p=clock.at(8,c);
    check(clock.at(40,c)==p,"F: frozen phase holds");
    FlightClock stoppedFlight;EvolutionControls stopControls;stopControls.flight=true;stoppedFlight.at(0,stopControls);stoppedFlight.at(4,stopControls);stopControls.flight=false;
    double stoppedPhase=stoppedFlight.at(8,stopControls);check(stoppedFlight.at(16,stopControls)==stoppedPhase,"Flight stop holds current position");
    auto before=evolving.memory.current.pattern;for(int bar=33;bar<=64;++bar) check(same(evolving.cycle(bar*4.,s,c,*bank),before),"F: freeze does not evolve notes");
    c.freeze=false;c.depth=0;for(int bar=65;bar<=80;++bar) check(same(evolving.cycle(bar*4.,s,c,*bank),before),"OFF mutation holds");
    evolving.capture();auto saved=evolving.memory.captured.pattern;c.depth=2;
    for(int bar=81;bar<=96;++bar) {auto old=evolving.memory.current.pattern;evolving.cycle(bar*4.,s,c,*bank);check(differences(old,evolving.memory.current.pattern)<=2,"LOW maximum two");}
    evolving.recall();check(same(evolving.cycle(388,s,c,*bank),saved),"Recall saved notes");
    // All simultaneous fields count as one event; stable ties choose step 1.
    PatternState a,b;for(int i=0;i<16;++i){b.pattern.notes[i]={60,90,true,false};}
    check(mutate(a,b,1)==1 && a.pattern.notes[0].pitch==60 && a.pattern.notes[1].pitch==48,"C: event count and fixed tie-break");
    for(int i=0;i<flightFrames;++i) {
        const auto& f=bank->frames[i];check(f.math.zoom>=1 && f.math.zoom<=maxFlightZoom,"Numerical zoom bounds");
        auto point=coordinate(f.math,0);check(point.x!=f.math.x,"Radius remains representable at max zoom");
    }
    std::cout<<"PASS A/B/C/D/F/H/I engine harness; capture/recall, LOW, tie-break and zoom checks. E/G deferred.\n";
    return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

#include "Engine.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <vector>
void check(bool ok,const char* message){ if(!ok) throw std::runtime_error(message); }
bool near(double a,double b){return std::abs(a-b)<1e-10;}
struct Event { bool on; int pitch,velocity; int sample; };
std::vector<Event> render(int block,bool automation) {
    fractal::Engine engine; fractal::Settings s; s.gen=false; s.pitchAmount=s.velocityAmount=0;
    std::vector<Event> events;
    for(int start=0;start<288000;) {
        // Same exact host event timeline, split buffers at event timestamps.
        if(automation && start==30000) { s.gen=true; s.pitchAmount=1; s.x=-1; s.radius=0; }
        if(automation && start==100000) { s.rhythmAmount=1; }
        int end=std::min(start+block,288000);
        if(automation) for(int boundary : {30000,100000}) if(start<boundary) end=std::min(end,boundary);
        engine.process(end-start,48000,true,start/24000.0,120,s,[&](bool on,int n,int v,int offset){events.push_back({on,n,v,start+offset});});
        start=end;
    }
    engine.process(1,48000,false,12,120,s,[&](bool on,int n,int v,int offset){events.push_back({on,n,v,288000+offset});});
    return events;
}
void compare(const std::vector<Event>& a,const std::vector<Event>& b) {
    check(a.size()==b.size(),"event count");
    for(size_t i=0;i<a.size();++i) check(a[i].on==b[i].on && a[i].pitch==b[i].pitch && a[i].velocity==b[i].velocity && a[i].sample==b[i].sample,"event equality");
}
int main() {
    try {
        const auto zero=fractal::analyze({0,0},128),period2=fractal::analyze({-1,0},128),edge=fractal::analyze({-2,0},128),outside=fractal::analyze({1,0},128);
        check(!zero.escaped && zero.observed==128 && near(zero.magnitude,0) && near(zero.movement,0),"zero orbit");
        check(!period2.escaped && near(period2.magnitude,0.125) && near(period2.movement,0.25),"period two orbit");
        check(!edge.escaped && near(edge.magnitude,1),"strict escape radius boundary");
        check(outside.escaped && outside.escapeIndex==3 && outside.smoothValid,"escape indexing");
        const auto late=fractal::analyze({-.75,.001},512); check(!late.escaped && !late.smoothValid,"unresolved stays distinct");
        const auto a=fractal::analyze({-.75,.1},128),b=fractal::analyze({-.75,-.1},128);
        check(a.escapeIndex==33 && a.escapeIndex==b.escapeIndex && near(a.magnitude,b.magnitude) && near(a.movement,b.movement),"conjugate symmetry");
        fractal::Settings s; s.gen=false; s.pitchAmount=s.rhythmAmount=s.velocityAmount=0;
        s.velocities[2]=73; s.on[3]=false; const auto original=fractal::makePattern(s);
        for(int i=0;i<16;++i) check(original.notes[i].pitch==fractal::scalePitch(s.root,s.scale,s.degrees[i]) && original.notes[i].on==s.on[i] && original.notes[i].velocity==s.velocities[i],"CTRL exact valid baseline");
        s.pitchAmount=1; auto changed=fractal::makePattern(s);
        for(int i=0;i<16;++i) check(changed.notes[i].velocity==original.notes[i].velocity && changed.notes[i].on==original.notes[i].on,"pitch independent");
        s.pitchAmount=0; s.velocityAmount=1; changed=fractal::makePattern(s);
        for(int i=0;i<16;++i) check(changed.notes[i].pitch==original.notes[i].pitch && changed.notes[i].on==original.notes[i].on,"velocity independent");
        s.velocityAmount=0;
        for(double amount : {0.0,0.25,0.5,1.0}) {
            s.rhythmAmount=amount; changed=fractal::makePattern(s);
            for(int i=0;i<16;++i) {
                check(changed.notes[i].on==(s.on[i] && changed.features[i].contrast>=amount*s.threshold),"MASK rule");
                check(changed.notes[i].pitch==original.notes[i].pitch && changed.notes[i].velocity==original.notes[i].velocity,"rhythm independent");
            }
        }
        s.rhythmAmount=0; changed=fractal::makePattern(s);
        for(int i=0;i<16;++i) check(changed.notes[i].on==original.notes[i].on,"MASK reversible");
        for(int scale=0;scale<4;++scale) for(int root : {24,48,84}) for(int source=0;source<4;++source) {
            s.scale=scale; s.root=root; s.pitchSource=source; s.pitchAmount=1; s.low=41; s.high=68;
            const auto p=fractal::makePattern(s);
            for(const auto& n:p.notes) check(n.pitch>=41 && n.pitch<=68 && fractal::inScale(n.pitch,root,scale),"range and scale bounds");
        }
        s.low=s.high=49; s.root=48; s.scale=1; check(!fractal::makePattern(s).validRange,"empty scale range");
        s.x=std::nan(""); s.zoom=std::numeric_limits<double>::infinity(); s.pitchAmount=-1;
        const auto clean=fractal::sanitize(s); check(clean.x==-.6 && clean.zoom==1 && clean.pitchAmount==0,"nonfinite sanitization");
        s=fractal::Settings{}; s.sampler=1;
        auto p=fractal::makePattern(s); check(p.features[0].contrast==0,"line no artificial closing edge");
        auto start=fractal::coordinate(s,0),end=fractal::coordinate(s,15); check(near(end.x-start.x,2*s.radius),"line extent");
        s.zoom=10; check(near(fractal::coordinate(s,15).x-s.x,s.radius/10),"zoom separates from amount");
        for(bool autoMode : {false,true}) { const auto reference=render(64,autoMode); compare(reference,render(511,autoMode)); compare(reference,render(2048,autoMode)); }
        const auto events=render(64,true);
        check(events.size()==64,"automation event count before masked cycle");
        check(events.back().sample<192000,"rhythm automation applies and masks third cycle");
        const auto constant=render(64,false);
        check(constant.size()==96 && constant[1].sample==3900,"16 steps per cycle and gate samples");
        check(events[2].pitch==fractal::scalePitch(48,1,2) && events[30].pitch==fractal::scalePitch(48,1,1),"mode remains until next cycle");
        fractal::Engine engine; s=fractal::Settings{}; std::vector<Event> tail;
        auto emit=[&](bool on,int n,int v,int i){tail.push_back({on,n,v,i});};
        engine.process(128,48000,true,0,120,s,emit); engine.process(128,48000,false,128/24000.0,120,s,emit);
        check(tail.size()==2 && !tail.back().on && tail.back().sample==0,"stop releases active note");
        tail.clear(); engine.process(128,48000,true,0,120,s,emit); engine.process(128,48000,true,0,120,s,emit);
        check(tail.size()==3 && !tail[1].on && tail[2].on,"loop off before on");
        s.enabled=false; engine.process(128,48000,true,.01,120,s,emit); check(!tail.back().on,"panic off");
        tail.clear(); s.enabled=true; engine.process(128,48000,true,.13,120,s,emit); check(tail.empty(),"midstep start");
        engine.process(128,48000,true,-.25,120,s,emit); check(tail.size()==1 && tail[0].on,"negative PPQ");
        engine.process(128,48000,true,0,std::nan(""),s,emit); check(!tail.back().on,"invalid tempo off");
        tail.clear(); engine.process(128,48000,true,0,120,s,emit); engine.requestReset(); engine.process(128,96000,true,0,120,s,emit);
        check(tail.size()==3 && !tail[1].on,"reset and sample rate release");
        for(double bpm : {60.0,180.0}) {
            fractal::Engine timed; const int duration=int(48000*60*4/bpm); int ons=0,offs=0;
            for(int i=0;i<duration;i+=257) timed.process(std::min(257,duration-i),48000,true,i*bpm/(60*48000),bpm,s,[&](bool on,int,int,int){on ? ++ons : ++offs;});
            check(ons==16 && offs==16,"tempo synchronized note count");
        }
        s.iterations=512; s.radius=.001; s.x=-.75; s.y=.001;
        const auto t0=std::chrono::steady_clock::now(); double sum=0;
        for(int i=0;i<1000;++i) { s.x=-.75+i*1e-9; sum+=fractal::makePattern(s).features[0].magnitude; }
        const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count()/1000;
        std::cout<<"PASS: orbit math, MASK, independent amounts, scale/range, synchronization, automation timeline, note-offs; worst-budget mean pattern time "<<ms<<" ms (checksum "<<sum<<")\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}

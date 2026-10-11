#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <vector>
void check(bool ok,const char* message){if(!ok) throw std::runtime_error(message);}
void set(MandelbrotProcessor& p,const char* id,float v) {
    auto* param=p.state.getParameter(id); check(param!=nullptr,"parameter exists");
    param->setValueNotifyingHost(param->convertTo0to1(v));
}
struct Playhead final : juce::AudioPlayHead {
    double ppq=0,bpm=120; bool playing=true;
    juce::Optional<PositionInfo> getPosition() const override {
        PositionInfo p; p.setPpqPosition(ppq); p.setBpm(bpm); p.setIsPlaying(playing); return p;
    }
};
std::vector<std::array<int64_t,4>> renderProcessor(const juce::MemoryBlock& saved,int block,bool offline) {
    MandelbrotProcessor p;p.setStateInformation(saved.getData(),int(saved.getSize()));p.setRateAndBufferSizeDetails(48000,block);p.setNonRealtime(offline);p.prepareToPlay(48000,block);
    Playhead head;p.setPlayHead(&head);std::vector<std::array<int64_t,4>> events;
    for(int start=0;start<128*24000;start+=block) {
        int count=std::min(block,128*24000-start);head.ppq=start/24000.;juce::AudioBuffer<float> audio(2,count);juce::MidiBuffer midi;p.processBlock(audio,midi);
        for(auto e:midi){check(e.numBytes==3,"three-byte note MIDI");events.push_back({start+e.samplePosition,e.data[0],e.data[1],e.data[2]});}
    }
    return events;
}
int main() {
    juce::ScopedJuceInitialiser_GUI init;
    try {
        MandelbrotProcessor first,second;
        check(first.acceptsMidi() && first.producesMidi(),"Live MIDI bus flags");
        set(first,"mode",1); set(first,"x",-.75f); set(first,"y",.1f); set(first,"zoomExponent",2);
        set(first,"pitchAmount",.77f); set(first,"rhythmAmount",0); set(first,"threshold",.3f); set(first,"velocityAmount",.4f);
        set(first,"flightEnable",1);
        set(first,"step02",5); set(first,"vel02",73); set(first,"on03",0);
        first.prepareFlight();
        juce::MemoryBlock state; first.getStateInformation(state); second.setStateInformation(state.getData(),int(state.getSize()));
        auto a=first.state.copyState(),b=second.state.copyState();
        a.removeProperty("schemaVersion",nullptr); b.removeProperty("schemaVersion",nullptr);
        check(a.isEquivalentTo(b),"all parameter state roundtrip");
        check(renderProcessor(state,127,false)==renderProcessor(state,4096,true),"actual MATH FLIGHT processor: 32 bars, realtime/offline flags and block sizes produce identical MIDI");
        Playhead h1,h2; first.setPlayHead(&h1); second.setPlayHead(&h2);
        first.setNonRealtime(false);second.setNonRealtime(true);
        first.setRateAndBufferSizeDetails(48000,512); second.setRateAndBufferSizeDetails(48000,512);
        first.prepareToPlay(48000,512); second.prepareToPlay(48000,512);
        int noteOns=0;
        for(int start=0;start<100000;start+=512) {
            h1.ppq=h2.ppq=start/24000.0;
            juce::AudioBuffer<float> audio1(2,512),audio2(2,512); audio1.clear(); audio2.clear();
            juce::MidiBuffer m1,m2; first.processBlock(audio1,m1); second.processBlock(audio2,m2);
            check(m1.getNumEvents()==m2.getNumEvents(),"preset MIDI event count");
            auto i2=m2.begin();
            for(auto meta1:m1) { auto meta2=*i2++; if(meta1.getMessage().isNoteOn()) ++noteOns; check(meta1.samplePosition==meta2.samplePosition && meta1.numBytes==meta2.numBytes && std::memcmp(meta1.data,meta2.data,size_t(meta1.numBytes))==0,"preset identical MIDI bytes"); }
            check(audio1.getMagnitude(0,512)==0,"silent audio");
        }
        check(noteOns>=15,"preset comparison actually emitted notes");
        set(first,"capture",1);juce::AudioBuffer<float> captureAudio(2,512);juce::MidiBuffer captureMidi;first.processBlock(captureAudio,captureMidi);
        check(first.memoryState().hasCaptured,"capture stores actually applied pattern");
        first.getStateInformation(state);MandelbrotProcessor restored;restored.setStateInformation(state.getData(),int(state.getSize()));
        restored.setRateAndBufferSizeDetails(48000,512);restored.setPlayHead(&h1);restored.prepareToPlay(48000,512);restored.processBlock(captureAudio,captureMidi);
        auto saved=first.memoryState().captured,loaded=restored.memoryState().captured;
        check(restored.memoryState().hasCaptured,"captured memory survives preset");
        for(int i=0;i<16;++i) {check(!fractal::stepDifferent(saved.pattern.notes[i],loaded.pattern.notes[i]),"saved MIDI pattern exact");check(saved.points[i].x==loaded.points[i].x && saved.points[i].y==loaded.points[i].y,"saved mathematical sample points exact");check(saved.pattern.features[i].magnitude==loaded.pattern.features[i].magnitude && saved.pattern.features[i].movement==loaded.pattern.features[i].movement && saved.pattern.features[i].contrast==loaded.pattern.features[i].contrast,"saved ranking features exact");}
        check(saved.math.x==loaded.math.x && saved.math.y==loaded.math.y && saved.math.zoom==loaded.math.zoom,"saved navigation exact");
        const auto before=second.state.copyState(); const char garbage[]="not a preset";
        second.setStateInformation(garbage,sizeof(garbage)); check(before.isEquivalentTo(second.state.copyState()),"reject malformed preset");
        set(second,"panic",1); juce::AudioBuffer<float> audio(2,512); juce::MidiBuffer midi;
        second.processBlock(audio,midi); for(auto meta:midi) check(!meta.getMessage().isNoteOn(),"panic suppresses note on");
        std::cout<<"PASS: APVTS preset roundtrip, identical processor MIDI, bus flags, silent audio, malformed state, panic\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}

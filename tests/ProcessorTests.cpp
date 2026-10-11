#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
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
int main() {
    juce::ScopedJuceInitialiser_GUI init;
    try {
        MandelbrotProcessor first,second;
        check(first.acceptsMidi() && first.producesMidi(),"Live MIDI bus flags");
        set(first,"mode",1); set(first,"x",-.75f); set(first,"y",.1f); set(first,"zoomExponent",2);
        set(first,"pitchAmount",.77f); set(first,"rhythmAmount",0); set(first,"threshold",.3f); set(first,"velocityAmount",.4f);
        set(first,"step02",5); set(first,"vel02",73); set(first,"on03",0);
        juce::MemoryBlock state; first.getStateInformation(state); second.setStateInformation(state.getData(),int(state.getSize()));
        auto a=first.state.copyState(),b=second.state.copyState();
        a.removeProperty("schemaVersion",nullptr); b.removeProperty("schemaVersion",nullptr);
        check(a.isEquivalentTo(b),"all parameter state roundtrip");
        Playhead h1,h2; first.setPlayHead(&h1); second.setPlayHead(&h2);
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
        const auto before=second.state.copyState(); const char garbage[]="not a preset";
        second.setStateInformation(garbage,sizeof(garbage)); check(before.isEquivalentTo(second.state.copyState()),"reject malformed preset");
        set(second,"panic",1); juce::AudioBuffer<float> audio(2,512); juce::MidiBuffer midi;
        second.processBlock(audio,midi); for(auto meta:midi) check(!meta.getMessage().isNoteOn(),"panic suppresses note on");
        std::cout<<"PASS: APVTS preset roundtrip, identical processor MIDI, bus flags, silent audio, malformed state, panic\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}

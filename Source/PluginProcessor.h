#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Engine.h"
class MandelbrotProcessor final : public juce::AudioProcessor {
public:
    MandelbrotProcessor();
    void prepareToPlay(double,int) override { restart.store(true); }
    void releaseResources() override { restart.store(true); }
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout& l) const override { return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo(); }
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "FRACTAL LAB MANDELBROT v0.2b"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    fractal::Settings settings() const noexcept;
    juce::String diagnostics() const;
    juce::AudioProcessorValueTreeState state;
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    std::array<std::atomic<float>*,24> global {};
    std::array<std::atomic<float>*,16> degrees {},velocities {},enabled {};
    std::array<std::atomic<float>,16> mag {},movement {},contrast {},escape {},note {},on {},projected {};
    std::atomic<bool> appliedGen {true},validRange {true},restart {false};
    fractal::Engine engine;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MandelbrotProcessor)
};


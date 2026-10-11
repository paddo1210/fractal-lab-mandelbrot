#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Evolution.h"
#include <mutex>
class MandelbrotProcessor final : public juce::AudioProcessor {
public:
    MandelbrotProcessor();
    void prepareToPlay(double,int) override;
    void releaseResources() override { restart.store(true); }
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout& l) const override { return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo(); }
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "FRACTAL LAB MANDELBROT v0.3"; }
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
    bool prepareFlight(const fractal::Settings* origin=nullptr,const fractal::FlightConfig* config=nullptr);
    fractal::MemoryState memoryState() const noexcept {fractal::MemoryState m;memories.read([&](const fractal::MemoryState& value){m=value;});return m;}
    fractal::Settings previewSettings() const noexcept;
    fractal::EvolutionControls controls() const noexcept;
    bool flightNeedsPrepare() const noexcept {
        const auto s=settings();bool dirty=true;
        banks.read([&](const fractal::FlightBank& b){dirty=s.x!=b.origin.x || s.y!=b.origin.y || s.zoom!=b.origin.zoom || s.radius!=b.origin.radius || s.sampler!=b.origin.sampler || s.iterations!=b.origin.iterations || state.getRawParameterValue("driftX")->load()!=b.config.driftX || state.getRawParameterValue("driftY")->load()!=b.config.driftY || (state.getRawParameterValue("zoomDirection")->load()<.5f ? 1 : -1)!=b.config.direction;});return dirty;
    }
    std::atomic<bool> transportPlaying {false};
    std::atomic<int> changedSteps {0};
    std::atomic<double> nextEvolutionBeat {0}, hostBeat {0};
    juce::AudioProcessorValueTreeState state;
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    std::array<std::atomic<float>*,24> global {};
    std::array<std::atomic<float>*,16> degrees {},velocities {},enabled {};
    std::array<std::atomic<float>,16> mag {},movement {},contrast {},escape {},note {},on {},projected {};
    std::atomic<bool> appliedGen {true},validRange {true},restart {false};
    fractal::Engine engine;
    fractal::Exchange<fractal::FlightBank> banks;
    fractal::Exchange<fractal::MemoryState> memories;
    fractal::Exchange<fractal::MemoryState> restoredMemories;
    fractal::Exchange<fractal::Settings> previews;
    fractal::Evolution evolution;
    std::array<bool,4> previousCommands {};
    std::atomic<bool> restored {false};
    uint64_t bankGeneration=0;
    std::mutex preparationLock;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MandelbrotProcessor)
};


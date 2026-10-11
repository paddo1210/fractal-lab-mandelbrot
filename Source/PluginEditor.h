#pragma once
#include "PluginProcessor.h"
#include "ViewMath.h"
#include "BinaryData.h"
#include "LabTheme.h"
#include <mutex>

class FractalView final : public juce::Component, private juce::Thread, private juce::Timer {
public:
    FractalView() : Thread("Mandelbrot visual worker") { startThread(); startTimerHz(20); }
    ~FractalView() override { stopTimer(); signalThreadShouldExit(); notify(); stopThread(-1); }
    void request(fractal::Settings s) {
        { std::lock_guard<std::mutex> guard(lock); requested=s; ++serial; }
        notify();
    }
    void paint(juce::Graphics& g) override {
        g.fillAll(lab::background());
        if(frame.isValid()) g.drawImage(frame,getLocalBounds().toFloat());
        if(!frame.isValid()) return;
        const float sx=float(getWidth())/472, sy=float(getHeight())/104;
        const auto v=fractal::viewport(displayed,472,104);
        g.setColour(juce::Colours::white.withAlpha(.25f));
        g.drawLine(getWidth()*.5f,0,getWidth()*.5f,float(getHeight()));
        g.drawLine(0,getHeight()*.5f,float(getWidth()),getHeight()*.5f);
        for(int i=0;i<16;++i) {
            const auto p=v.pixel(fractal::coordinate(displayed,i));
            const float x=float(p.x)*sx, y=float(p.y)*sy;
            g.setColour((i<8 ? lab::cyan() : lab::orange()));
            g.drawEllipse(x-4,y-4,8,8,1.5f);
            g.drawText(juce::String(i+1),int(x+5),int(y-10),24,18,juce::Justification::left);
        }
        g.setColour(juce::Colours::white);
        g.drawText("PREVIEW  X "+juce::String(displayed.x,6)+"  Y "+juce::String(displayed.y,6)
            +"  Zoom x"+juce::String(displayed.zoom,2)+"  N="+juce::String(displayed.iterations)
            +"  black = unresolved at N",8,4,getWidth()-16,20,juce::Justification::left);
    }
private:
    void run() override {
        uint64_t processed=0;
        while(!threadShouldExit()) {
            fractal::Settings s; uint64_t version;
            { std::lock_guard<std::mutex> guard(lock); s=requested; version=serial; }
            if(version==processed) { wait(100); continue; }
            juce::Image next(juce::Image::RGB,472,104,false);
            const auto v=fractal::viewport(s,472,104);
            bool cancelled=false;
            for(int row=0;row<104;++row) {
                { std::lock_guard<std::mutex> guard(lock); cancelled=threadShouldExit(); }
                if(cancelled) break;
                for(int col=0;col<472;++col) {
                    const int n=fractal::escapeIterations(v.at(col+.5,row+.5),s.iterations);
                    const float t=n==0 ? 0 : .25f+.75f*float(std::log(1.+n)/std::log(1.+s.iterations));
                    auto colour=(col<236 ? lab::cyan() : lab::orange()).withMultipliedBrightness(t);
                    next.setPixelAt(col,row,colour);
                }
            }
            if(!cancelled) {
                std::lock_guard<std::mutex> guard(lock);
                ready=next; readySettings=s; ++completion;
                processed=version;
            }
        }
    }
    void timerCallback() override {
        { std::lock_guard<std::mutex> guard(lock);
          if(shown==completion) return;
          frame=ready; displayed=readySettings; shown=completion; }
        repaint();
    }
    std::mutex lock;
    fractal::Settings requested,readySettings,displayed;
    uint64_t serial=0,completion=0,shown=0;
    juce::Image ready,frame;
};

class LabEditor final : public juce::AudioProcessorEditor, private juce::Timer {
    struct ParameterWindow final : juce::DocumentWindow {
        ParameterWindow() : DocumentWindow("Host-assignable parameters",lab::background(),juce::DocumentWindow::closeButton) {}
        void closeButtonPressed() override { setVisible(false); }
    };
    using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment=juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment=juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    struct Dial { juce::Slider slider; juce::Label label; std::unique_ptr<SliderAttachment> attachment; };
    struct Step { juce::Slider degree,velocity; juce::ToggleButton on; std::unique_ptr<SliderAttachment> d,v; std::unique_ptr<ButtonAttachment> b; };
public:
    explicit LabEditor(MandelbrotProcessor& p) : AudioProcessorEditor(p),processor(p) {
        setLookAndFeel(&look);
        portrait=juce::ImageCache::getFromMemory(BinaryData::DesignReference_png,BinaryData::DesignReference_pngSize);
        const char* dialIds[]={"x","y","zoomExponent","radius","pitchAmount","rhythmAmount","velocityAmount","gate","zoom","root","velocity"};
        const char* names[]={"X CENTER","Y CENTER","ZOOM EXP (10^n)","LOCAL RADIUS","PITCH AMOUNT","RHYTHM MASK","VELOCITY AMOUNT","GATE","ZOOM MULTIPLIER","ROOT MIDI","BASE VELOCITY"};
        for(int i=0;i<11;++i) {
            auto& d=dials[size_t(i)];
            d.slider.setComponentID(dialIds[i]);
            d.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            d.slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,104,22);
            d.slider.setColour(juce::Slider::rotarySliderFillColourId,(i%2 ? lab::orange() : lab::cyan()));
            d.label.setText(names[i],juce::dontSendNotification); d.label.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(d.slider); addAndMakeVisible(d.label);
            d.attachment=std::make_unique<SliderAttachment>(p.state,dialIds[i],d.slider);
        }
        scale.addItemList({"Chromatic","Major","Minor","Minor Pentatonic"},1);
        mode.addItemList({"GEN","CTRL"},1); sampler.addItemList({"Circle","Line"},1);
        iterations.addItemList({"64 iterations","128 iterations","256 iterations","512 iterations"},1);
        for(auto* c : {&scale,&mode,&sampler,&iterations}) addAndMakeVisible(*c);
        scaleBinding=std::make_unique<ComboAttachment>(p.state,"scale",scale);
        modeBinding=std::make_unique<ComboAttachment>(p.state,"mode",mode);
        samplerBinding=std::make_unique<ComboAttachment>(p.state,"sampler",sampler);
        iterationBinding=std::make_unique<ComboAttachment>(p.state,"iterations",iterations);
        run.setButtonText("RUN"); panic.setButtonText("PANIC (toggle)");
        panic.setColour(juce::ToggleButton::tickColourId,lab::red());
        addAndMakeVisible(run); addAndMakeVisible(panic);
        runBinding=std::make_unique<ButtonAttachment>(p.state,"run",run);
        panicBinding=std::make_unique<ButtonAttachment>(p.state,"panic",panic);
        advanced.setButtonText("All parameters / diagnostics"); addAndMakeVisible(advanced);
        advanced.onClick=[this] {
            if(details) { details.reset(); return; }
            details=std::make_unique<ParameterWindow>();
            details->setContentOwned(new juce::GenericAudioProcessorEditor(processor),true);
            details->setUsingNativeTitleBar(true); details->centreWithSize(620,740); details->setVisible(true);
        };
        for(int i=0;i<16;++i) {
            auto& s=steps[size_t(i)]; const auto suffix=juce::String(i+1).paddedLeft('0',2);
            s.degree.setComponentID("step"+suffix); s.velocity.setComponentID("vel"+suffix); s.on.setComponentID("on"+suffix);
            s.on.setButtonText(juce::String(i+1));
            s.on.setColour(juce::ToggleButton::tickColourId,i<8 ? lab::cyan() : lab::orange());
            for(auto* slider : {&s.degree,&s.velocity}) {
                slider->setSliderStyle(juce::Slider::LinearVertical);
                slider->setTextBoxStyle(juce::Slider::TextBoxBelow,false,38,20);
                slider->setColour(juce::Slider::trackColourId,(i<8 ? lab::cyan() : lab::orange()));
                addAndMakeVisible(*slider);
            }
            s.degree.setTooltip("Step "+suffix+" scale degree"); s.velocity.setTooltip("Step "+suffix+" CTRL velocity");
            addAndMakeVisible(s.on);
            s.d=std::make_unique<SliderAttachment>(p.state,"step"+suffix,s.degree);
            s.v=std::make_unique<SliderAttachment>(p.state,"vel"+suffix,s.velocity);
            s.b=std::make_unique<ButtonAttachment>(p.state,"on"+suffix,s.on);
        }
        addAndMakeVisible(view); addAndMakeVisible(info);
        info.setMultiLine(true); info.setReadOnly(true); info.setScrollbarsShown(true);
        info.setFont(juce::Font(juce::FontOptions(11.f)));
        flightMode.addItemList({"OFF","MATH FLIGHT"},1);rate.addItemList({"4 bars","8 bars","16 bars"},1);depth.addItemList({"OFF","SUBTLE (1)","LOW (2)"},1);direction.addItemList({"Zoom In","Zoom Out"},1);
        const char* comboIds[]={"flightMode","evolutionRate","mutationDepth","zoomDirection"};
        juce::ComboBox* combos[]={&flightMode,&rate,&depth,&direction};
        for(int i=0;i<4;++i){addAndMakeVisible(*combos[i]);flightCombos[size_t(i)]=std::make_unique<ComboAttachment>(p.state,comboIds[i],*combos[i]);}
        const char* toggleIds[]={"flightEnable","freeze","hold","evolve","capture","recall","patternReset","flightReset"};
        const char* toggleNames[]={"FLIGHT","FREEZE","HOLD","EVOLVE","CAPTURE","RECALL","RESET A","FLIGHT RESET"};
        for(int i=0;i<8;++i){auto& button=flightButtons[size_t(i)];button.setButtonText(toggleNames[i]);button.setComponentID(toggleIds[i]);addAndMakeVisible(button);flightToggles[size_t(i)]=std::make_unique<ButtonAttachment>(p.state,toggleIds[i],button);}
        const char* sliderIds[]={"flightSpeed","driftX","driftY"};
        for(int i=0;i<3;++i){auto& slider=flightSliders[size_t(i)];slider.setSliderStyle(juce::Slider::LinearHorizontal);slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,46,22);slider.setTooltip(sliderIds[i]);addAndMakeVisible(slider);flightSliderBindings[size_t(i)]=std::make_unique<SliderAttachment>(p.state,sliderIds[i],slider);}
        prepare.setButtonText("PREPARE");addAndMakeVisible(prepare);
        prepare.onClick=[this]{prepareMessage=processor.prepareFlight() ? "Flight plan prepared" : "Stop transport before Prepare";};
        setSize(1200,1020); startTimerHz(20); timerCallback();
    }
    ~LabEditor() override { stopTimer(); details.reset(); setLookAndFeel(nullptr); }
    void paint(juce::Graphics& g) override {
        g.fillAll(lab::background());
        g.setColour(lab::cyan().withAlpha(.4f)); g.drawLine(16,62,1184,62,1);
        g.setColour(lab::text()); g.setFont(24.f);
        g.drawText("F R A C T A L   L A B  |  M A N D E L B R O T",24,14,950,40,juce::Justification::left);
        g.setFont(14.f); g.drawText("v0.3 PROTOTYPE",1050,14,130,40,juce::Justification::right);
        for(auto r : {juce::Rectangle<int>(16,70,310,370),juce::Rectangle<int>(850,70,334,370),juce::Rectangle<int>(16,448,1168,242),juce::Rectangle<int>(16,700,1168,170)}) {
            lab::panelFrame(g,r.toFloat());
        }
        for(int i=0;i<16;++i) {
            auto bounds=juce::Rectangle<float>(float(25+i*72),730,70,132);
            g.setColour(lab::background()); g.fillRoundedRectangle(bounds,5);
            g.setColour(lab::stroke()); g.drawRoundedRectangle(bounds,5,1);
        }
        // Use the reference's portrait area directly; no painted controls are reused.
        if(portrait.isValid()) g.drawImage(portrait,410,70,354,370,500,95,535,560);
        g.setColour(lab::cyan());
        g.drawText("NAVIGATION",30,78,260,24,juce::Justification::left);
        g.drawText("MUSICAL CONTROL",866,78,300,24,juce::Justification::left);
        g.drawText("16 STEPS  |  degree / CTRL velocity  |  host MIDI assignments",30,704,1120,22,juce::Justification::left);
        lab::panelFrame(g,juce::Rectangle<float>(16,872,1168,140));
        g.setColour(lab::cyan());g.drawText("FRACTAL FLIGHT / CONTROLLED EVOLUTION",28,874,900,22,juce::Justification::left);
    }
    void resized() override {
        auto dial=[this](int i,int x,int y,int w,int h) { auto& d=dials[size_t(i)]; d.label.setBounds(x,y,w,20); d.slider.setBounds(x,y+20,w,h-20); };
        dial(0,28,105,140,150); dial(1,174,105,140,150);
        dial(2,28,266,140,150); dial(3,174,266,140,150);
        dial(4,858,106,104,150); dial(5,964,106,104,150); dial(6,1070,106,104,150);
        scale.setBounds(864,267,158,28); mode.setBounds(1036,267,138,28);
        dial(7,858,310,104,122); dial(9,964,310,104,122); dial(10,1070,310,104,122);
        dial(8,28,470,180,120); sampler.setBounds(36,606,160,26); iterations.setBounds(36,644,160,26);
        view.setBounds(222,466,944,208);
        for(int i=0;i<16;++i) {
            int x=28+i*72; auto& s=steps[size_t(i)]; s.on.setBounds(x,730,65,24);
            s.degree.setBounds(x,755,34,103); s.velocity.setBounds(x+34,755,34,103);
        }
        flightButtons[0].setBounds(26,900,88,28);flightMode.setBounds(120,900,120,28);flightSliders[0].setBounds(246,900,130,28);
        flightButtons[1].setBounds(382,900,82,28);rate.setBounds(470,900,98,28);depth.setBounds(574,900,132,28);
        flightButtons[2].setBounds(712,900,72,28);flightButtons[3].setBounds(790,900,82,28);flightButtons[4].setBounds(878,900,86,28);flightButtons[5].setBounds(970,900,86,28);prepare.setBounds(1062,900,108,28);
        flightSliders[1].setBounds(28,938,130,28);flightSliders[2].setBounds(166,938,130,28);direction.setBounds(304,938,112,28);
        flightButtons[7].setBounds(424,938,126,28);flightButtons[6].setBounds(558,938,100,28);run.setBounds(668,938,80,28);panic.setBounds(758,938,136,28);advanced.setBounds(904,938,266,28);
        info.setBounds(24,976,1152,30);
    }
private:
    void timerCallback() override {
        const auto s=processor.previewSettings();
        if(!haveSettings || s.x!=last.x || s.y!=last.y || s.zoom!=last.zoom || s.radius!=last.radius || s.sampler!=last.sampler || s.iterations!=last.iterations) {
            view.request(s); last=s; haveSettings=true;
        }
        const auto text=processor.diagnostics();
        const auto c=processor.controls();
        const auto content=juce::String(c.hold ? "HOLD" : (c.evolve ? "EVOLVE" : "LOCKED"))+" | X "+juce::String(s.x,6)+" Y "+juce::String(s.y,6)+" Zoom x"+juce::String(s.zoom,2)+" | "+juce::String(c.rate)+" bars | Next PPQ "+juce::String(processor.nextEvolutionBeat.load(),2)+" | Changed "+juce::String(processor.changedSteps.load())+" | "+prepareMessage+"\n"+text;
        if(info.getText()!=content) info.setText(content,false);
    }
    MandelbrotProcessor& processor;
    lab::Look look;
    juce::Image portrait;
    std::array<Dial,11> dials;
    std::array<Step,16> steps;
    juce::ComboBox scale,mode,sampler,iterations;
    juce::ToggleButton run,panic;
    juce::TextButton advanced;
    juce::TextEditor info;
    juce::TooltipWindow tooltips{this,500};
    std::unique_ptr<ComboAttachment> scaleBinding,modeBinding,samplerBinding,iterationBinding;
    std::unique_ptr<ButtonAttachment> runBinding,panicBinding;
    std::unique_ptr<juce::DocumentWindow> details;
    std::array<juce::ToggleButton,8> flightButtons;
    std::array<juce::Slider,3> flightSliders;
    juce::ComboBox flightMode,rate,depth,direction;
    juce::TextButton prepare;
    std::array<std::unique_ptr<ButtonAttachment>,8> flightToggles;
    std::array<std::unique_ptr<SliderAttachment>,3> flightSliderBindings;
    std::array<std::unique_ptr<ComboAttachment>,4> flightCombos;
    juce::String prepareMessage="Speed / Drift X / Drift Y: hover for label; origin edits require stopped Prepare";
    FractalView view;
    fractal::Settings last;
    bool haveSettings=false;
};


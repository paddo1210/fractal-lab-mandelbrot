#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace lab {
inline juce::Colour cyan() { return juce::Colour(0xff00e5ff); }
inline juce::Colour blue() { return juce::Colour(0xff0088ff); }
inline juce::Colour orange() { return juce::Colour(0xffff9a3c); }
inline juce::Colour red() { return juce::Colour(0xffff3d57); }
inline juce::Colour background() { return juce::Colour(0xff0a0f14); }
inline juce::Colour panel() { return juce::Colour(0xff16222b); }
inline juce::Colour raised() { return juce::Colour(0xff1e2d3a); }
inline juce::Colour stroke() { return juce::Colour(0xff2f4a5a); }
inline juce::Colour text() { return juce::Colour(0xffe6f7ff); }
inline juce::Colour muted() { return juce::Colour(0xff8aa3b2); }

inline void panelFrame(juce::Graphics& g,juce::Rectangle<float> bounds) {
    g.setGradientFill(juce::ColourGradient(raised(),bounds.getTopLeft(),background(),bounds.getBottomRight(),false));
    g.fillRoundedRectangle(bounds,10);
    g.setColour(cyan().withAlpha(.45f)); g.drawRoundedRectangle(bounds.reduced(.5f),10,1);
}

class Look final : public juce::LookAndFeel_V4 {
public:
    Look() {
        setColour(juce::Slider::textBoxTextColourId,text());
        setColour(juce::Slider::textBoxBackgroundColourId,background());
        setColour(juce::Slider::textBoxOutlineColourId,stroke());
        setColour(juce::Slider::rotarySliderFillColourId,cyan());
        setColour(juce::Slider::trackColourId,cyan());
        setColour(juce::Label::textColourId,text());
        setColour(juce::ComboBox::backgroundColourId,background());
        setColour(juce::ComboBox::textColourId,text());
        setColour(juce::ComboBox::outlineColourId,stroke());
        setColour(juce::ComboBox::arrowColourId,cyan());
        setColour(juce::TextButton::buttonColourId,panel());
        setColour(juce::TextButton::buttonOnColourId,cyan());
        setColour(juce::TextButton::textColourOffId,text());
        setColour(juce::TextButton::textColourOnId,background());
        setColour(juce::ToggleButton::textColourId,text());
        setColour(juce::ToggleButton::tickColourId,cyan());
        setColour(juce::TextEditor::backgroundColourId,background());
        setColour(juce::TextEditor::textColourId,muted());
        setColour(juce::TextEditor::outlineColourId,stroke());
        setColour(juce::PopupMenu::backgroundColourId,panel());
        setColour(juce::PopupMenu::textColourId,text());
        setColour(juce::PopupMenu::highlightedBackgroundColourId,stroke());
    }
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider& slider) override {
        auto bounds=juce::Rectangle<float>(float(x),float(y),float(w),float(h)).reduced(10);
        const float r=std::min(bounds.getWidth(),bounds.getHeight())*.5f;
        auto c=bounds.getCentre();
        const bool enabled=slider.isEnabled();
        auto accent=enabled ? slider.findColour(juce::Slider::rotarySliderFillColourId) : muted().withAlpha(.35f);
        g.setColour(background()); g.fillEllipse(c.x-r,c.y-r,2*r,2*r);
        juce::Path arc; arc.addCentredArc(c.x,c.y,r-3,r-3,0,start,end,true);
        g.setColour(stroke()); g.strokePath(arc,juce::PathStrokeType(5));
        juce::Path value; value.addCentredArc(c.x,c.y,r-3,r-3,0,start,start+pos*(end-start),true);
        const bool hover=enabled && slider.isMouseOverOrDragging();
        if(hover) { g.setColour(accent.withAlpha(.18f)); g.strokePath(value,juce::PathStrokeType(11)); }
        g.setColour(accent); g.strokePath(value,juce::PathStrokeType(5));
        for(int i=0;i<=10;++i) {
            const float a=start+(end-start)*float(i)/10;
            g.setColour(muted().withAlpha(enabled ? .7f : .25f));
            g.drawLine(c.x+std::sin(a)*(r+4),c.y-std::cos(a)*(r+4),c.x+std::sin(a)*(r+7),c.y-std::cos(a)*(r+7),1);
        }
        const float inner=r-11;
        g.setGradientFill(juce::ColourGradient(raised().brighter(.15f),c.x-inner,c.y-inner,background(),c.x+inner,c.y+inner,false));
        g.fillEllipse(c.x-inner,c.y-inner,2*inner,2*inner);
        g.setColour(stroke()); g.drawEllipse(c.x-inner,c.y-inner,2*inner,2*inner,1);
        const float a=start+pos*(end-start);
        g.setColour(enabled ? text() : muted().withAlpha(.4f));
        g.drawLine(c.x+std::sin(a)*inner*.48f,c.y-std::cos(a)*inner*.48f,c.x+std::sin(a)*inner*.86f,c.y-std::cos(a)*inner*.86f,2);
    }
    void drawButtonBackground(juce::Graphics& g,juce::TextButton& button,const juce::Colour&,bool hover,bool down) override {
        auto bounds=button.getLocalBounds().toFloat().reduced(1);
        const bool active=button.getToggleState() || down;
        auto fill=active ? cyan() : panel();
        if(!button.isEnabled()) fill=background();
        g.setGradientFill(juce::ColourGradient(fill.brighter(.12f),bounds.getTopLeft(),fill.darker(.35f),bounds.getBottomRight(),false));
        g.fillRoundedRectangle(bounds,5);
        g.setColour(button.isEnabled() && (active || hover) ? cyan() : stroke());
        g.drawRoundedRectangle(bounds,5,1);
    }
    void drawToggleButton(juce::Graphics& g,juce::ToggleButton& button,bool hover,bool down) override {
        auto bounds=button.getLocalBounds().toFloat().reduced(1);
        const bool step=button.getComponentID().startsWith("on");
        auto accent=button.findColour(juce::ToggleButton::tickColourId);
        if(!button.isEnabled()) accent=muted().withAlpha(.35f);
        const bool active=button.getToggleState();
        g.setColour(active ? accent.withAlpha(down ? .4f : .2f) : panel());
        g.fillRoundedRectangle(bounds,5);
        g.setColour(active || hover ? accent : stroke()); g.drawRoundedRectangle(bounds,5,1);
        g.setColour(button.isEnabled() ? text() : muted());
        g.setFont(12.f); g.drawText(button.getButtonText(),bounds.toNearestInt(),juce::Justification::centred);
        if(step) {
            auto indicator=juce::Rectangle<float>(bounds.getX()+10,bounds.getBottom()-5,bounds.getWidth()-20,2);
            g.setColour(active ? accent : stroke()); g.fillRoundedRectangle(indicator,1);
        }
    }
    void drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float sliderPos,float minPos,float maxPos,const juce::Slider::SliderStyle style,juce::Slider& slider) override {
        if(style!=juce::Slider::LinearVertical) {
            LookAndFeel_V4::drawLinearSlider(g,x,y,w,h,sliderPos,minPos,maxPos,style,slider); return;
        }
        const float cx=float(x)+float(w)*.5f;
        auto track=juce::Rectangle<float>(cx-2,float(y)+3,4,float(h)-6);
        auto accent=slider.isEnabled() ? slider.findColour(juce::Slider::trackColourId) : muted().withAlpha(.35f);
        g.setColour(background()); g.fillRoundedRectangle(track,2);
        g.setColour(stroke()); g.drawRoundedRectangle(track,2,1);
        const float py=juce::jlimit(track.getY(),track.getBottom(),sliderPos);
        g.setColour(accent); g.fillRoundedRectangle(cx-1,py,2,track.getBottom()-py,1);
        g.setColour(accent.withAlpha(slider.isMouseOverOrDragging() ? .25f : .1f)); g.fillEllipse(cx-8,py-8,16,16);
        g.setColour(accent); g.fillEllipse(cx-5,py-5,10,10);
        g.setColour(text()); g.drawEllipse(cx-5,py-5,10,10,1);
    }
};
}

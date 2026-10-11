#include "PluginProcessor.h"
#include <stdexcept>
#include <iostream>
juce::Component* find(juce::Component& parent,const juce::String& id) {
    if(parent.getComponentID()==id) return &parent;
    for(auto* child:parent.getChildren()) if(auto* result=find(*child,id)) return result;
    return nullptr;
}
void check(bool value) { if(!value) throw std::runtime_error("GUI attachment test failed"); }
int main() {
    juce::ScopedJuceInitialiser_GUI init;
    MandelbrotProcessor p;
    for(int repeat=0;repeat<3;++repeat) {
        std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
        check(editor && editor->getWidth()==1200);
        auto* x=dynamic_cast<juce::Slider*>(find(*editor,"x")); check(x);
        auto* parameter=p.state.getParameter("x");
        parameter->setValueNotifyingHost(parameter->convertTo0to1(-.75f));
        check(std::abs(x->getValue()+.75)<1e-5);
        x->setValue(-.6,juce::sendNotificationSync);
        check(std::abs(p.state.getRawParameterValue("x")->load()+.6)<1e-5);
        for(int i=1;i<=16;++i) {
            const auto suffix=juce::String(i).paddedLeft('0',2);
            auto* degree=dynamic_cast<juce::Slider*>(find(*editor,"step"+suffix));
            auto* velocity=dynamic_cast<juce::Slider*>(find(*editor,"vel"+suffix));
            auto* on=dynamic_cast<juce::ToggleButton*>(find(*editor,"on"+suffix));
            check(degree && velocity && on);
            degree->setValue(i,juce::sendNotificationSync);
            velocity->setValue(80+i,juce::sendNotificationSync);
            on->setToggleState(false,juce::sendNotificationSync);
            check(p.state.getRawParameterValue("step"+suffix)->load()==i);
            check(p.state.getRawParameterValue("vel"+suffix)->load()==80+i);
            check(p.state.getRawParameterValue("on"+suffix)->load()==0);
        }
    }
    std::cout << "GUI host/control bindings and repeated editor lifetime passed\n";
}

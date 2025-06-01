#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "Rotary.h"

using namespace globals;
class REVERAudioProcessor;

class EnvelopeWidget : public juce::Component, private juce::AudioProcessorValueTreeState::Listener
{
public:
    std::unique_ptr<Rotary> thresh;
    std::unique_ptr<Rotary> amount;
    std::unique_ptr<Rotary> attack;
    std::unique_ptr<Rotary> release;
    std::unique_ptr<Rotary> hold;
    Slider filterRange;
    Label filterLabel;

    TextButton sidechainBtn;
    TextButton monitorBtn;
    TextButton autoRelBtn;

    EnvelopeWidget(REVERAudioProcessor& p, bool isResenv, int width);
    ~EnvelopeWidget() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void paint(juce::Graphics& g) override;
    void layoutComponents();

    void drawHeadphones(Graphics& g, Rectangle<int> bounds, Colour c);
    void drawSidechain(Graphics& g, Rectangle<int> bounds, Colour c);

private:
    bool isOn = false;
    const bool isSendenv;
    REVERAudioProcessor& audioProcessor;
};
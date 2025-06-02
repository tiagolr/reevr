#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "Rotary.h"
#include "AudioDisplay.h"

using namespace globals;
class REEVRAudioProcessor;

class AudioWidget : public juce::Component
{
public:
    std::unique_ptr<Rotary> threshold;
    std::unique_ptr<Rotary> sense;
    std::unique_ptr<Rotary> lowcut;
    std::unique_ptr<Rotary> highcut;
    std::unique_ptr<Rotary> offset;
    std::unique_ptr<AudioDisplay> audioDisplay;

    TextButton useSidechain;
    TextButton useMonitor;


    AudioWidget(REEVRAudioProcessor& p);
    ~AudioWidget() override;

    void paint(juce::Graphics& g) override;
    void toggleUIComponents();
    void resized() override;

private:
    REEVRAudioProcessor& audioProcessor;
};
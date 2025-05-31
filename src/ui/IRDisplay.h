#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;
class REVERAudioProcessor;

class IRDisplay : public juce::Component, private juce::Timer, private juce::AudioProcessorValueTreeState::Listener {
public:
    IRDisplay(REVERAudioProcessor& p);
    ~IRDisplay() override;
    void timerCallback() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void recalcWave();
    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseEnter(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;

    unsigned long int impulseVersion = 0;
    float attack = 0.0;
    float decay = 1.0;
    float trimLeft = 0.0;
    float trimRight = 0.0;

    int dragging = 0;
    bool mouseover = false;

private:
    Point<int> last_mouse_position;
    std::vector<float> waveLeft;
    std::vector<float> waveRight;
    REVERAudioProcessor& audioProcessor;

    Rectangle<int> getDisplayBounds();
    Rectangle<int> getTrimLeftBounds();
    Rectangle<int> getTrimRightBounds();
    Rectangle<int> getAttackBounds();
    Rectangle<int> getDecayBounds();
    RangedAudioParameter* getDragParam();
};
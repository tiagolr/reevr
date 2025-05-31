#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;
class REVERAudioProcessor;

class FileSelector : public juce::Component {
public:
    FileSelector(REVERAudioProcessor& p);
    ~FileSelector() override;

    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;


private:
    REVERAudioProcessor& audioProcessor;
};
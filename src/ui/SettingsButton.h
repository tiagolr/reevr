#pragma once

#include <JuceHeader.h>
#include <functional>

class REVERAudioProcessor;

class SettingsButton : public juce::Component {
public:
    SettingsButton(REVERAudioProcessor& p) : audioProcessor(p) {}
    ~SettingsButton() override {}

    void mouseDown(const juce::MouseEvent& e) override;
    void paint(Graphics& g) override;

    std::function<void()> onScaleChange;
    std::function<void()> toggleUIComponents;
    std::function<void()> toggleAbout;

private:
    REVERAudioProcessor& audioProcessor;
};
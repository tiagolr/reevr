#pragma once

#include <JuceHeader.h>
#include <functional>

class REEVRAudioProcessor;

class SettingsButton : public juce::Component {
public:
    SettingsButton(REEVRAudioProcessor& p) : audioProcessor(p) {}
    ~SettingsButton() override {}

    void mouseDown(const juce::MouseEvent& e) override;
    void paint(Graphics& g) override;

    std::function<void()> onScaleChange;
    std::function<void()> toggleUIComponents;
    std::function<void()> toggleAbout;

private:
    REEVRAudioProcessor& audioProcessor;
};
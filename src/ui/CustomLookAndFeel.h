#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel();
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font&) override;
    int getPopupMenuBorderSize() override;
    void drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown) override;
    void drawComboBox (Graphics&, int width, int height, bool isButtonDown,int buttonX, int buttonY, int buttonW, int buttonH, ComboBox&) override;
    void positionComboBoxText (ComboBox& box, Label& label) override;
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override {
        (void)buttonHeight;
        return button.getComponentID() == "small" ? FontOptions(12.0f) : FontOptions(16.0f);  
    }

    juce::Font getComboBoxFont (juce::ComboBox& box) override
    {
        return box.getComponentID() == "small" ? FontOptions(12.0f) : FontOptions(16.0f);
    }

    void drawTreeviewPlusMinusBox(Graphics& g, const Rectangle<float>& area,
        juce::Colour backgroundColour, bool isOpen, bool isMouseOver) override;

private:
    juce::Typeface::Ptr typeface;
};
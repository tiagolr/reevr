#include "CustomLookAndFeel.h"

CustomLookAndFeel::CustomLookAndFeel()
{
  setColour(ComboBox::backgroundColourId, Colour(COLOR_BG));
  setColour(ComboBox::textColourId, Colour(COLOR_ACTIVE));
  setColour(ComboBox::arrowColourId, Colour(COLOR_ACTIVE));
  setColour(ComboBox::outlineColourId, Colour(COLOR_ACTIVE));
  setColour(TooltipWindow::backgroundColourId, Colour(COLOR_BG).brighter(0.15f));
  setColour(PopupMenu::backgroundColourId, Colour(COLOR_ACTIVE).darker(0.5f).withAlpha(0.99f));
  setColour(PopupMenu::highlightedBackgroundColourId, Colour(COLOR_ACTIVE).darker(0.8f));
  setColour(TextButton::buttonColourId, Colour(COLOR_ACTIVE));
  setColour(TextButton::buttonOnColourId, Colour(COLOR_ACTIVE));
  setColour(TextButton::textColourOnId, Colour(COLOR_BG));
  setColour(TextButton::textColourOffId, Colour(COLOR_ACTIVE));
  setColour(BubbleComponent::ColourIds::backgroundColourId, Colour(COLOR_ACTIVE).darker(0.75f));
  setColour(BubbleComponent::ColourIds::outlineColourId, Colours::transparentWhite);
  setColour(ScrollBar::thumbColourId, Colour(COLOR_ACTIVE));
  setColour(ScrollBar::trackColourId, juce::Colours::transparentBlack);

  typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::UbuntuMedium_ttf, BinaryData::UbuntuMedium_ttfSize);
  setDefaultSansSerifTypeface(typeface);
  this->setDefaultLookAndFeel(this);
}

// Override the getTypefaceForFont function
juce::Typeface::Ptr CustomLookAndFeel::getTypefaceForFont(const juce::Font& /*font*/)
{
    return typeface;
}

int CustomLookAndFeel::getPopupMenuBorderSize()
{
    return 5;
}

void CustomLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float minSliderPos, float maxSliderPos,
    const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    auto tag = slider.getComponentID();
    if (tag != "symmetric" && tag != "symmetric_vertical") {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    if (tag == "symmetric_vertical") {
        const float center = (minSliderPos + maxSliderPos) * 0.5f;

        juce::Rectangle<float> trackBounds(x + width / 2.f - width / 8.f, (float)y, width / 4.f, (float)height );

        g.setColour(slider.findColour(juce::Slider::backgroundColourId));
        g.fillRoundedRectangle(trackBounds, 3.f);
        g.setColour(slider.findColour(juce::Slider::trackColourId));

        if (sliderPos <= center) {
            g.fillRoundedRectangle(trackBounds.getX(),
                sliderPos,
                trackBounds.getWidth(),
                center - sliderPos,
                3.f);
        }
        else {
            g.fillRoundedRectangle(trackBounds.getX(),
                center,
                trackBounds.getWidth(),
                sliderPos - center,
                3.f);
        }

        // Thumb
        g.setColour(slider.findColour(juce::Slider::thumbColourId));
        auto thumbSize = slider.getWidth() * 0.6f;
        g.fillEllipse(x + width / 2.f - thumbSize / 2.f,
            sliderPos - thumbSize / 2.f,
            thumbSize,
            thumbSize);
        return;
    }

    const float center = (minSliderPos + maxSliderPos) * 0.5f;

    juce::Rectangle<float> trackBounds((float)x, (float)(y + height / 2 - height / 8.f), (float)width, height / 4.f);
    g.setColour (slider.findColour (Slider::backgroundColourId));
    g.fillRoundedRectangle(trackBounds, 3.f);

    g.setColour(slider.findColour(Slider::trackColourId));
    if (sliderPos >= center)
        g.fillRoundedRectangle(center, trackBounds.getY(), sliderPos - center, trackBounds.getHeight(), 3.f);
    else
        g.fillRoundedRectangle(sliderPos, trackBounds.getY(), center - sliderPos, trackBounds.getHeight(), 3.f);

    g.setColour(slider.findColour(Slider::thumbColourId));
    auto thumbWidth = slider.getHeight() * 0.5f;
    g.fillEllipse(sliderPos - thumbWidth / 2.f, y + height / 2 - thumbWidth / 2.f, thumbWidth, thumbWidth);
};

void CustomLookAndFeel::drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown)
{
    auto tag = button.getComponentID();
    auto bounds = button.getLocalBounds().toFloat();
    auto cornerSize = 3.0f;

    if (tag == "button" || tag == "small" || tag == "button-noborder") {
        g.setColour(backgroundColour);
        if (button.getToggleState())
            g.fillRoundedRectangle(bounds, cornerSize);
        else if (tag != "button-noborder")
            g.drawRoundedRectangle(bounds.reduced(0.5, 0.5), cornerSize, 1.0f);

        return;
    }

    if (tag != "leftPattern" && tag != "rightPattern" && tag != "pattern") {
        LookAndFeel_V4::drawButtonBackground(g, button, backgroundColour, isMouseOverButton, isButtonDown);
        return;
    }

    bool roundLeft = tag == "leftPattern";
    bool roundRight = tag == "rightPattern";

    juce::Path path;

    if (roundLeft && roundRight) {
        path.addRoundedRectangle(bounds, 0.0f);
    }
    else {
        float topLeft = roundLeft ? cornerSize : 0.0f;
        float topRight = roundRight ? cornerSize : 0.0f;
        float bottomLeft = roundLeft ? cornerSize : 0.0f;
        float bottomRight = roundRight ? cornerSize : 0.0f;

        path.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerSize, cornerSize, topLeft, topRight, bottomLeft, bottomRight);
    }

    g.setColour(backgroundColour);
    g.fillPath(path);
}

void CustomLookAndFeel::drawComboBox(Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, ComboBox& box)
{
    (void)buttonX;
    (void)buttonY;
    (void)buttonH;
    (void)buttonW;
    (void)isButtonDown;

    auto cornerSize = 3.0f;
    Rectangle<int> boxBounds (0, 0, width, height);

    g.setColour (box.findColour (ComboBox::backgroundColourId));
    g.fillRoundedRectangle (boxBounds.toFloat(), cornerSize);

    g.setColour (box.findColour (ComboBox::outlineColourId));
    g.drawRoundedRectangle (boxBounds.toFloat().reduced (0.5f, 0.5f), cornerSize, 1.0f);

    auto arrowZone = Rectangle<int>(width - 20, 0, 20, height).translated(-3,0).toFloat();
    Path path;
    auto r = 4.0f;
    path.startNewSubPath({arrowZone.getCentreX() - r, arrowZone.getCentreY() - r/2});
    path.lineTo(arrowZone.getCentreX(), arrowZone.getCentreY() + r/2);
    path.lineTo(arrowZone.getCentreX() + r, arrowZone.getCentreY() - r/2);

    g.setColour (box.findColour (ComboBox::arrowColourId).withAlpha ((box.isEnabled() ? 0.9f : 0.2f)));
    g.strokePath (path, PathStrokeType(2.f));
}

void CustomLookAndFeel::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (1, 1,
        box.getWidth() - 20,
        box.getHeight() - 2);

    label.setFont (getComboBoxFont (box));
}

void CustomLookAndFeel::drawTreeviewPlusMinusBox(Graphics& g, const Rectangle<float>& area,
    juce::Colour backgroundColour, bool isOpen, bool isMouseOver)
{
    (void)backgroundColour;
    (void)isMouseOver;
    Path p;
    g.setColour(Colour(COLOR_ACTIVE)); 
    auto c = area.getCentre();
    auto r = 5.f;

    if (isOpen) {
        p.addTriangle(c.x - r, c.y - r, c.x, c.y+r, c.x+r, c.y-r);
    }
    else {
        p.addTriangle(c.x -r, c.y -r, c.x + r, c.y, c.x-r, c.y+r);
    }

    g.fillPath(p);
}
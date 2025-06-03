#include "EnvelopeWidget.h"
#include "../PluginProcessor.h"

EnvelopeWidget::EnvelopeWidget(REEVRAudioProcessor& p, bool isSendenv, int width) : audioProcessor(p), isSendenv(isSendenv) 
{
    audioProcessor.params.addParameterListener(isSendenv ? "sendenvon" : "revenvon", this);
    audioProcessor.params.addParameterListener(isSendenv ? "sendenvamt" : "revenvamt", this);
    audioProcessor.params.addParameterListener(isSendenv ? "sendenvlowcut" : "revenvlowcut", this);
    audioProcessor.params.addParameterListener(isSendenv ? "sendenvhighcut" : "revenvhighcut", this);
    
    isOn = (bool)audioProcessor.params.getRawParameterValue(isSendenv ? "sendenvon" : "revenvon")->load();

    int col = 0;
    int row = 5;

    thresh = std::make_unique<Rotary>(p, isSendenv ? "sendenvthresh" : "revenvthresh", "Thresh", RotaryLabel::gainTodB1f, false);
    addAndMakeVisible(*thresh);
    thresh->setBounds(col,row,80,65);
    col += 75;

    amount = std::make_unique<Rotary>(p, isSendenv ? "sendenvamt" : "revenvamt", "Amount", RotaryLabel::percx100, true);
    addAndMakeVisible(*amount);
    amount->setBounds(col,row,80,65);
    col += 75;

    col = width - 10 - PLUG_PADDING;
    row += 3; // align buttons middle
    addAndMakeVisible(sidechainBtn);
    sidechainBtn.setTooltip("Use sidechain as envelope input");
    sidechainBtn.setBounds(col-25-25-10, row, 25, 25);
    sidechainBtn.setAlpha(0.0f);
    sidechainBtn.onClick = [this, isSendenv] {
        if (isSendenv) audioProcessor.sendenvSidechain = !audioProcessor.sendenvSidechain;
        else audioProcessor.revenvSidechain = !audioProcessor.revenvSidechain;
        MessageManager::callAsync([this]{ audioProcessor.sendChangeMessage(); });
    };

    addAndMakeVisible(monitorBtn);
    monitorBtn.setTooltip("Monitor envelope input");
    monitorBtn.setBounds(col-25, row, 25,25);
    monitorBtn.setAlpha(0.0f);
    monitorBtn.onClick = [this, isSendenv] {
        if (isSendenv) audioProcessor.sendenvMonitor = !audioProcessor.sendenvMonitor;
        else audioProcessor.revenvMonitor = !audioProcessor.revenvMonitor;

        if (isSendenv && audioProcessor.sendenvMonitor) {
            audioProcessor.useMonitor = false;
            audioProcessor.revenvMonitor = false;
        }

        if (!isSendenv && audioProcessor.revenvMonitor) {
            audioProcessor.useMonitor = false;
            audioProcessor.sendenvMonitor = false;
        }

        MessageManager::callAsync([this]{ audioProcessor.sendChangeMessage(); });
    };

    addAndMakeVisible(autoRelBtn);
    autoRelBtn.setTooltip("Toggle auto release mode");
    autoRelBtn.setBounds(col-25-25-10, row+35, 60, 25);
    autoRelBtn.setComponentID("button");
    autoRelBtn.setButtonText("Auto");
    autoRelBtn.onClick = [this, isSendenv] {
        if (isSendenv) audioProcessor.resenvAutoRel = !audioProcessor.resenvAutoRel;
        else audioProcessor.revenvAutoRel = !audioProcessor.revenvAutoRel;
        MessageManager::callAsync([this]{ 
            audioProcessor.onSlider();
            audioProcessor.sendChangeMessage(); 
        });
    };
    if (!isSendenv) {
        autoRelBtn.setColour(TextButton::buttonColourId, Colour(0xffffffff));
        autoRelBtn.setColour(TextButton::buttonOnColourId, Colour(0xffffffff));
        autoRelBtn.setColour(TextButton::textColourOnId, Colour(COLOR_BG));
        autoRelBtn.setColour(TextButton::textColourOffId, Colour(0xffffffff));
    }

    addAndMakeVisible(filterRange);
    filterRange.setTooltip("Frequency range of the envelope input signal");
    filterRange.setSliderStyle(Slider::SliderStyle::TwoValueHorizontal);
    filterRange.setRange(20.0, 20000.0);
    filterRange.setSkewFactor(0.5, false);
    filterRange.setTextBoxStyle(Slider::NoTextBox, false, 80, 20);
    filterRange.setBounds(amount->getBounds().getRight() - 10, 20, autoRelBtn.getBounds().getX() - amount->getBounds().getRight() + 10 - 5, 25);
    filterRange.setColour(Slider::backgroundColourId, Colour(COLOR_BG).brighter(0.1f));
    filterRange.setColour(Slider::trackColourId, Colour(COLOR_ACTIVE).darker(0.5f));
    filterRange.setColour(Slider::thumbColourId, Colour(COLOR_ACTIVE));
    filterRange.onValueChange = [this, isSendenv]() {
        auto lowcut = filterRange.getMinValue();
        auto highcut = filterRange.getMaxValue();
        if (lowcut > highcut)
            filterRange.setMinAndMaxValues(highcut, highcut);

        MessageManager::callAsync([this, lowcut, highcut, isSendenv] {
            auto param = audioProcessor.params.getParameter(isSendenv ? "sendenvlowcut" : "revenvlowcut");
            param->setValueNotifyingHost(param->convertTo0to1((float)lowcut));
            param = audioProcessor.params.getParameter(isSendenv ? "sendenvhighcut" : "revenvhighcut");
            param->setValueNotifyingHost(param->convertTo0to1((float)highcut));
        });

        auto lowcutstr = lowcut > 1000 ? String(int(lowcut * 10 / 1000.0) / 10.0) + "k" : String((int)lowcut);
        auto highcutstr = highcut > 1000 ? String(int(highcut * 10 / 1000.0) / 10.0) + "k" : String((int)highcut);
        filterLabel.setText(lowcutstr + "-" + highcutstr + " Hz", dontSendNotification);
    };
    filterRange.setVelocityModeParameters(1.0,1,0.0,true,ModifierKeys::Flags::shiftModifier);
    filterRange.onDragEnd = [this]() {
        filterLabel.setText("Filter", dontSendNotification);
    };
    filterRange.setMinAndMaxValues(
        (double)audioProcessor.params.getRawParameterValue(isSendenv ? "sendenvlowcut" : "revenvlowcut")->load(),
        (double)audioProcessor.params.getRawParameterValue(isSendenv ? "sendenvhighcut" : "revenvhighcut")->load(),
        dontSendNotification
    );

    addAndMakeVisible(filterLabel);
    filterLabel.setFont(FontOptions(16.f));
    filterLabel.setJustificationType(Justification::centredBottom);
    filterLabel.setText("Filter", dontSendNotification);
    filterLabel.setBounds(filterRange.getBounds().withBottomY(65 + 5 + 1));

    row += 75;
    col = 0;

    attack = std::make_unique<Rotary>(p, isSendenv ? "sendenvatk" : "revenvatk", "Attack", RotaryLabel::envatk);
    addAndMakeVisible(*attack);
    attack->setBounds(col,row,80,65);
    col += 75;

    hold = std::make_unique<Rotary>(p, isSendenv ? "sendenvhold" : "revenvhold", "Hold", RotaryLabel::envhold);
    addAndMakeVisible(*hold);
    hold->setBounds(col,row,80,65);
    col += 75;

    release = std::make_unique<Rotary>(p, isSendenv ? "sendenvrel" : "revenvrel", "Release", RotaryLabel::envrel);
    addAndMakeVisible(*release);
    release->setBounds(col,row,80,65);
    col += 75;

}

EnvelopeWidget::~EnvelopeWidget()
{
    audioProcessor.params.removeParameterListener(isSendenv ? "sendenvamt" : "revenvamt", this);
    audioProcessor.params.removeParameterListener(isSendenv ? "sendenvlowcut" : "revenvlowcut", this);
    audioProcessor.params.removeParameterListener(isSendenv ? "sendenvhighcut" : "revenvhighcut", this);
    audioProcessor.params.removeParameterListener(isSendenv ? "sendenvon" : "revenvon", this);
}

void EnvelopeWidget::parameterChanged(const juce::String& parameterID, float newValue)
{
    bool isrevenvon = (bool)audioProcessor.params.getRawParameterValue("revenvon")->load();
    bool issendenvon = (bool)audioProcessor.params.getRawParameterValue("sendenvon")->load();
    isOn = (isSendenv && issendenvon) || (!isSendenv && isrevenvon);

    if (isVisible() && parameterID == "sendenvamt" && newValue != 0.0f && !issendenvon) {
        MessageManager::callAsync([this] {
            audioProcessor.params.getParameter("sendenvon")->setValueNotifyingHost(1.0f);
        });
    }
    if (isVisible() && parameterID == "sendenvamt" && newValue == 0.0f && issendenvon) {
        MessageManager::callAsync([this] {
            audioProcessor.params.getParameter("sendenvon")->setValueNotifyingHost(0.0f);
        });
    }
    if (isVisible() && parameterID == "revenvamt" && newValue != 0.0f && !isrevenvon) {
        MessageManager::callAsync([this] {
            audioProcessor.params.getParameter("revenvon")->setValueNotifyingHost(1.0f);
        });
    }
    if (isVisible() && parameterID == "revenvamt" && newValue == 0.0f && isrevenvon) {
        MessageManager::callAsync([this] {
            audioProcessor.params.getParameter("revenvon")->setValueNotifyingHost(0.0f);
        });
    }
    if (parameterID == "revenvlowcut" || parameterID == "sendenvlowcut") {
        filterRange.setMinValue((double)newValue, dontSendNotification);
    }
    if (parameterID == "revenvhighcut" || parameterID == "sendenvhighcut") {
        filterRange.setMaxValue((double)newValue, dontSendNotification);
    }
    repaint();
}

void EnvelopeWidget::paint(juce::Graphics& g) 
{
    g.setColour(Colour(COLOR_BG));
    g.fillAll();
    auto bounds = getLocalBounds().expanded(0, -1).toFloat();
    g.setColour(Colour(COLOR_BG).darker(0.125f));
    g.fillRoundedRectangle(bounds, 3.f);
    g.setColour(Colour(isSendenv ? COLOR_ACTIVE : 0xffffffff).withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.translated(0.5f, -0.5f), 3.f, 1.f);

    g.setColour((isSendenv ? Colour(COLOR_ACTIVE) : Colours::white).withAlpha(isOn ? 1.0f : 0.5f));
    if ((isSendenv && audioProcessor.sendenvMonitor) || (!isSendenv && audioProcessor.revenvMonitor)) {
        g.fillRoundedRectangle(monitorBtn.getBounds().toFloat().translated(0.5f, 0.5f), 3.f);
        drawHeadphones(g, monitorBtn.getBounds(), Colour(COLOR_BG));
    }
    else {
        g.drawRoundedRectangle(monitorBtn.getBounds().toFloat().translated(0.5f, 0.5f), 3.f, 1.f);
        drawHeadphones(g, monitorBtn.getBounds(), (isSendenv ? Colour(COLOR_ACTIVE) : Colours::white).withAlpha(isOn ? 1.0f : 0.5f));
    }

    g.setColour(isSendenv ? Colour(COLOR_ACTIVE) : Colours::white);
    if ((isSendenv && audioProcessor.sendenvSidechain) || (!isSendenv && audioProcessor.revenvSidechain)) {
        g.fillRoundedRectangle(sidechainBtn.getBounds().toFloat().translated(0.5f, 0.5f), 3.f);
        drawSidechain(g, sidechainBtn.getBounds(), Colour(COLOR_BG));
    }
    else {
        g.drawRoundedRectangle(sidechainBtn.getBounds().toFloat().translated(0.5f, 0.5f), 3.f, 1.f);
        drawSidechain(g, sidechainBtn.getBounds(), isSendenv ? Colour(COLOR_ACTIVE) : Colours::white);
    }
}

void EnvelopeWidget::drawHeadphones(Graphics& g, Rectangle<int> bounds, Colour c)
{
    g.setColour(c);
    auto b = bounds.toFloat().expanded(-6,-3).translated(0,4);
    Path p;
    p.startNewSubPath(b.getX(), b.getCentreY());
    p.addArc(b.getX(), b.getY(), b.getWidth(), b.getHeight(), -MathConstants<float>::halfPi, MathConstants<float>::halfPi);
    g.strokePath(p, PathStrokeType(1.5f));
    b = bounds.toFloat().expanded(-5,0).translated(0,4);
    g.fillRoundedRectangle(b.getX(), b.getCentreY() - 3.f, 3.f, 6.f, 3.f);
    g.fillRoundedRectangle(b.getRight()-3.f, b.getCentreY() - 3.f, 3.f, 6.f, 3.f);
}

void EnvelopeWidget::drawSidechain(Graphics& g, Rectangle<int> bounds, Colour c)
{
    auto b = bounds.toFloat().expanded(0.f,-6.f);
    Path p;
    float centerA = b.getX() + b.getWidth() / 3.f;
    float centerB = b.getX() + b.getWidth() / 3.f * 2;

    g.setColour(c);
    p.startNewSubPath(centerA, b.getBottom());
    p.lineTo(centerA, b.getY());
    p.startNewSubPath(centerB, b.getBottom());
    p.lineTo(centerB, b.getY());

    // draw arrowheads
    float r = 3.f;
    p.startNewSubPath(centerA-r, b.getY()+r);
    p.lineTo(centerA, b.getY());
    p.lineTo(centerA+r, b.getY()+r);

    p.startNewSubPath(centerB-r, b.getY()+r);
    p.lineTo(centerB, b.getY());
    p.lineTo(centerB+r, b.getY()+r);

    g.strokePath(p, PathStrokeType(1.f));
}

void EnvelopeWidget::layoutComponents()
{
    autoRelBtn.setToggleState((isSendenv && audioProcessor.resenvAutoRel) || (!isSendenv && audioProcessor.revenvAutoRel), dontSendNotification);
    repaint();
}
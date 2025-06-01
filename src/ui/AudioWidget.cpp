#include "AudioWidget.h"
#include "../PluginProcessor.h"

AudioWidget::AudioWidget(REVERAudioProcessor& p) : audioProcessor(p)
{
    int col = 0;
    int row = 0;

    threshold = std::make_unique<Rotary>(p, "threshold", "Thres", RotaryLabel::gainTodB1f, false, COLOR_AUDIO);
    addAndMakeVisible(*threshold);
    threshold->setBounds(col,row,80,65);
    col += 70;

    sense = std::make_unique<Rotary>(p, "sense", "Sense", RotaryLabel::percx100, false, COLOR_AUDIO);
    addAndMakeVisible(*sense);
    sense->setBounds(col,row,80,65);
    col += 70;

    lowcut = std::make_unique<Rotary>(p, "lowcut", "Low Cut", RotaryLabel::hzHp, false, COLOR_AUDIO);
    addAndMakeVisible(*lowcut);
    lowcut->setBounds(col,row,80,65);
    col += 70;

    highcut = std::make_unique<Rotary>(p, "highcut", "Hi Cut", RotaryLabel::hzLp, false, COLOR_AUDIO);
    addAndMakeVisible(*highcut);
    highcut->setBounds(col,row,80,65);
    col += 70;

    offset = std::make_unique<Rotary>(p, "offset", "Offset", RotaryLabel::audioOffset, true, COLOR_AUDIO);
    addAndMakeVisible(*offset);
    offset->setBounds(col,row,80,65);
    col += 70;

    col = getWidth() - PLUG_PADDING - 90;
    addAndMakeVisible(useSidechain);
    useSidechain.setTooltip("Use sidechain for transient detection");
    useSidechain.setButtonText("Sidechain");
    useSidechain.setComponentID("button");
    useSidechain.setColour(TextButton::buttonColourId, Colour(COLOR_AUDIO));
    useSidechain.setColour(TextButton::buttonOnColourId, Colour(COLOR_AUDIO));
    useSidechain.setColour(TextButton::textColourOnId, Colour(COLOR_BG));
    useSidechain.setColour(TextButton::textColourOffId, Colour(COLOR_AUDIO));
    useSidechain.setBounds(col,row+5,90,25);
    useSidechain.onClick = [this]() {
        MessageManager::callAsync([this] {
            audioProcessor.useSidechain = !audioProcessor.useSidechain;
            toggleUIComponents();
            });
        };

    addAndMakeVisible(useMonitor);
    useMonitor.setTooltip("Monitor signal used for transient detection");
    useMonitor.setButtonText("Monitor");
    useMonitor.setComponentID("button");
    useMonitor.setColour(TextButton::buttonColourId, Colour(COLOR_AUDIO));
    useMonitor.setColour(TextButton::buttonOnColourId, Colour(COLOR_AUDIO));
    useMonitor.setColour(TextButton::textColourOnId, Colour(COLOR_BG));
    useMonitor.setColour(TextButton::textColourOffId, Colour(COLOR_AUDIO));
    useMonitor.setBounds(col,row+35,90,25);
    useMonitor.onClick = [this]() {
        MessageManager::callAsync([this] {
            audioProcessor.useMonitor = !audioProcessor.useMonitor;
            if (audioProcessor.useMonitor) {
                audioProcessor.revenvMonitor = false;
                audioProcessor.sendenvMonitor = false;
            }
            audioProcessor.sendChangeMessage();
            });
        };

    col = 0;
    row += 75;

    audioDisplay = std::make_unique<AudioDisplay>(p);
    addAndMakeVisible(*audioDisplay);
    audioDisplay->setBounds(col+10,row,getWidth(), 65);

    
}

AudioWidget::~AudioWidget() 
{
}

void AudioWidget::resized()
{
    auto w = getWidth();
    auto bounds = useSidechain.getBounds();
    useSidechain.setBounds(bounds.withX(w - bounds.getWidth()));
    bounds = useMonitor.getBounds();
    useMonitor.setBounds(bounds.withX(w - bounds.getWidth()));
    bounds = audioDisplay->getBounds();
    audioDisplay->setBounds(bounds.withRight(getLocalBounds().getRight()));
}

void AudioWidget::paint(juce::Graphics& g)
{
    g.setColour(Colour(COLOR_BG));
    g.fillAll();
}

void AudioWidget::toggleUIComponents()
{
    useSidechain.setToggleState(audioProcessor.useSidechain, dontSendNotification);
    useMonitor.setToggleState(audioProcessor.useMonitor, dontSendNotification);
}
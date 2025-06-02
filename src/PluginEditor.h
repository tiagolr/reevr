/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/Rotary.h"
#include "ui/TextDial.h"
#include "ui/GridSelector.h"
#include "ui/CustomLookAndFeel.h"
#include "ui/About.h"
#include "ui/View.h"
#include "ui/SettingsButton.h"
#include "ui/AudioDisplay.h"
#include "ui/PaintToolWidget.h"
#include "ui/SequencerWidget.h"
#include "ui/EnvelopeWidget.h"
#include "ui/AudioWidget.h"
#include "ui/IRDisplay.h"
#include "ui/FileSelector.h"

using namespace globals;

class REVERAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::AudioProcessorValueTreeState::Listener, public juce::ChangeListener
{
public:
    REVERAudioProcessorEditor (REVERAudioProcessor&);
    ~REVERAudioProcessorEditor() override;

    //==============================================================================
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void toggleUIComponents ();
    void paint (juce::Graphics&) override;
    void resized() override;
    void changeListenerCallback(ChangeBroadcaster* source) override;
    void drawGear(Graphics&g, Rectangle<int> bounds, float radius, int segs, Colour color, Colour bg);
    void drawUndoButton(Graphics& g, juce::Rectangle<float> area, bool invertx, Colour color);
    void drawPowerButton(Graphics& g, Rectangle<float> area, Colour color);

private:
    bool init = false;
    REVERAudioProcessor& audioProcessor;
    CustomLookAndFeel* customLookAndFeel = nullptr;
    std::unique_ptr<About> about;
    std::unique_ptr<IRDisplay> irDisplay;
    std::unique_ptr<FileSelector> fileSelector;
    TextButton prevFile;
    TextButton nextFile;
    TextButton currentFile;

    std::vector<std::unique_ptr<TextButton>> patterns;

#if defined(DEBUG)
    juce::TextButton presetExport;
#endif

    Label logoLabel;
    ComboBox syncMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncAttachment;
    Label patSyncLabel;
    ComboBox patSyncMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> patSyncAttachment;
    std::unique_ptr<SettingsButton> settingsButton;
    std::unique_ptr<TextDial> rateDial;
    std::unique_ptr<EnvelopeWidget> revenv;
    std::unique_ptr<EnvelopeWidget> sendenv;

    std::unique_ptr<Rotary> predelay;
    std::unique_ptr<Rotary> stretch;
    std::unique_ptr<Rotary> width;
    std::unique_ptr<Rotary> lowcut;
    std::unique_ptr<Rotary> highcut;
    std::unique_ptr<Rotary> drywet;

    std::unique_ptr<Rotary> reverb;
    std::unique_ptr<Rotary> send;
    std::unique_ptr<Rotary> revoffset;
    std::unique_ptr<Rotary> sendoffset;
    std::unique_ptr<Rotary> rate;
    std::unique_ptr<Rotary> smooth;
    std::unique_ptr<Rotary> attack;
    std::unique_ptr<Rotary> release;
    std::unique_ptr<Rotary> tension;
    std::unique_ptr<Rotary> tensionatk;
    std::unique_ptr<Rotary> tensionrel;
    
    std::unique_ptr<AudioWidget> audioWidget;
    ComboBox algoMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> algoAttachment;
    TextButton lowcutSlope;
    TextButton highcutSlope;
    TextButton revEnvButton;
    TextButton revEnvOnButton;
    TextButton sendEnvButton;
    TextButton sendEnvOnButton;
    TextButton nudgeRightButton;
    Label nudgeLabel;
    TextButton nudgeLeftButton;
    TextButton undoButton;
    TextButton redoButton;
    TextButton paintButton;
    TextButton sequencerButton;
    ComboBox pointMenu;
    Label pointLabel;
    TextButton loopButton;
    Label triggerLabel;
    ComboBox triggerMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> triggerAttachment;
    TextButton audioSettingsButton;
    TextButton snapButton;
    std::unique_ptr<GridSelector> gridSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> snapAttachment;
    std::unique_ptr<View> view;
    Label latencyWarning;
    std::unique_ptr<PaintToolWidget> paintWidget;
    std::unique_ptr<SequencerWidget> seqWidget;

    TooltipWindow tooltipWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (REVERAudioProcessorEditor)
};

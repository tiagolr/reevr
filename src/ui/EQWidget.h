#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "Rotary.h"
#include "EQDisplay.h"

using namespace globals;
class REEVRAudioProcessorEditor;

class EQWidget 
	: public juce::Component
	, private juce::AudioProcessorValueTreeState::Listener
{
public:
	std::unique_ptr<EQDisplay> eq;
	std::vector<std::unique_ptr<Rotary>> freqknobs;
	std::vector<std::unique_ptr<Rotary>> qknobs;
	std::vector<std::unique_ptr<Rotary>> gainknobs;

	Slider rateSlider;
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateSliderAttachment;

	TextButton bandBtn;

	EQWidget(REEVRAudioProcessorEditor& e, SVF::EQType type);
	~EQWidget();

	void parameterChanged(const juce::String& parameterID, float newValue) override;
	void paint(Graphics& g) override;
	void resized() override;
	void toggleUIComponents();
	void showBandModeMenu();

private:
	int selband = 0;
	String prel;
	SVF::EQType type;
	REEVRAudioProcessorEditor& editor;
};
#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;
class REEVRAudioProcessorEditor;

class EQWidget : public juce::Component
{
public:
	EQWidget (REEVRAudioProcessorEditor& e) : editor(e) {}
	~EQWidget () {}

	void paint(Graphics& g) override;

private:
	REEVRAudioProcessorEditor& editor;
};
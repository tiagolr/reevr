#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;
class REEVRAudioProcessorEditor;

class DecayWidget : public juce::Component
{
public:
	DecayWidget (REEVRAudioProcessorEditor& e) : editor(e) {}
	~DecayWidget () {}

	void paint(Graphics& g) override;

private:
	REEVRAudioProcessorEditor& editor;
};
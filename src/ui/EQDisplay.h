#pragma once
#include <JuceHeader.h>
#include "../dsp/SVF.h"
#include "../Globals.h"

using namespace globals;
class REEVRAudioProcessorEditor;

class EQDisplay
	: public juce::Component
	, private juce::Timer
{
public:
	enum EQType {
		PostEQ,
		DecayEQ
	};

	EQDisplay(REEVRAudioProcessorEditor& e, EQType type);
	~EQDisplay();

	void timerCallback();
	void mouseDown(const MouseEvent& e) override;
	void mouseDrag(const MouseEvent& e) override;
	void mouseUp(const MouseEvent& e) override;
	void mouseDoubleClick(const MouseEvent& e) override;
	void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    void paint(juce::Graphics& g) override;
	void resized() override;
	void toggleUIComponents();

	void updateEQCurve();

private:
	EQType type;
	int selband = 0;
	int dragband = -1;
	Rectangle<float> viewBounds{};
	std::array<Rectangle<float>, EQ_BANDS> bandBounds{};
	std::array<SVF, EQ_BANDS> bandFilters{};

	bool mouse_down = false;
	float cur_freq_normed_value = 0.f;
	float cur_gain_normed_value = 0.f;
	float cur_q_normed_value = 0.f;
	Point<int> last_mouse_pos;
	Point<int> start_mouse_pos;

	std::vector<float> magPoints;

	REEVRAudioProcessorEditor& editor;
};
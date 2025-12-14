#pragma once
#include <functional>
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

	EQDisplay(REEVRAudioProcessorEditor& e, SVF::EQType type);
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

	std::function<void(int band)> onMouseDownBand;
	std::function<void()> onMouseUp;
	std::function<void()> onMouseDrag;

	std::array<SVF, EQ_BANDS> bandFilters{};
private:
	SVF::EQType type;
	String prel;
	int selband = 0;
	int dragband = -1;
	Rectangle<float> viewBounds{};
	std::array<Rectangle<float>, EQ_BANDS> bandBounds{};

	bool mouse_down = false;
	float cur_freq_normed_value = 0.f;
	float cur_gain_normed_value = 0.f;
	float cur_q_normed_value = 0.f;
	Point<int> last_mouse_pos;
	Point<int> start_mouse_pos;

	std::vector<float> magPoints;

	REEVRAudioProcessorEditor& editor;
};
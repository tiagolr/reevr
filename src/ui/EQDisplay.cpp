#include "EQDisplay.h"
#include "../PluginEditor.h"

EQDisplay::EQDisplay(REEVRAudioProcessorEditor& e, SVF::EQType _type)
	: editor(e)
	, type(_type)
	, prel(type == SVF::PostEQ ? "post" : "decay")
{
	startTimerHz(30);
	updateEQCurve();
}

EQDisplay::~EQDisplay()
{
}

void EQDisplay::timerCallback()
{
	if (isShowing()) {
		repaint();
	}
}

void EQDisplay::mouseDown(const MouseEvent& e)
{
	dragband = -1;
	for (int i = 0; i < EQ_BANDS; ++i) {
		auto& bounds = bandBounds[i];
		if (bounds.contains((float)e.x, (float)e.y)) {
			dragband = i;
			break;
		}
	}

	if (dragband == -1)
		return;

	selband = dragband;
	if (onMouseDownBand)
		onMouseDownBand(selband);

	mouse_down = true;
	e.source.enableUnboundedMouseMovement(true);
	auto pre = prel + "eq_band" + String(selband + 1);
	cur_freq_normed_value = editor.audioProcessor.params.getParameter(pre + "_freq")->getValue();
	cur_gain_normed_value = editor.audioProcessor.params.getParameter(pre + "_gain")->getValue();
	cur_q_normed_value = editor.audioProcessor.params.getParameter(pre + "_q")->getValue();
	setMouseCursor(MouseCursor::NoCursor);
	start_mouse_pos = Desktop::getInstance().getMousePosition();
	last_mouse_pos = e.getPosition();
	toggleUIComponents();
}

void EQDisplay::mouseUp(const MouseEvent& e)
{
	if (!mouse_down) return;
	if (onMouseUp)
		onMouseUp();

	mouse_down = false;
	setMouseCursor(MouseCursor::NormalCursor);
	e.source.enableUnboundedMouseMovement(false);
	auto pt = localPointToGlobal(bandBounds[selband].getCentre()).toInt();
	Desktop::getInstance().setMousePosition(pt);
	repaint();
}

void EQDisplay::mouseDrag(const MouseEvent& e)
{
	if (!mouse_down) return;

	auto change = e.getPosition() - last_mouse_pos;
	last_mouse_pos = e.getPosition();
	auto speed = (e.mods.isShiftDown() ? 40.0f : 4.0f) * 100.f;
	auto changeX = change.getX() / speed;
	auto changeY = change.getY() / speed;

	if (e.mods.isCommandDown()) {
		cur_q_normed_value += changeY;
		cur_q_normed_value = std::clamp(cur_q_normed_value, 0.f, 1.f);
	}
	else {
		cur_freq_normed_value += changeX;
		cur_gain_normed_value -= changeY * 2.f;
		cur_freq_normed_value = std::clamp(cur_freq_normed_value, 0.f, 1.f);
		cur_gain_normed_value = std::clamp(cur_gain_normed_value, 0.f, 1.f);
	}

	auto pre = prel + "eq_band" + String(selband + 1);
	if (e.mods.isCommandDown()) {
		editor.audioProcessor.params.getParameter(pre + "_q")->setValueNotifyingHost(cur_q_normed_value);
	}
	else {
		editor.audioProcessor.params.getParameter(pre + "_freq")->setValueNotifyingHost(cur_freq_normed_value);
		editor.audioProcessor.params.getParameter(pre + "_gain")->setValueNotifyingHost(cur_gain_normed_value);
	}

	if (onMouseDrag) 
		onMouseDrag();

	repaint();
}

void EQDisplay::mouseDoubleClick(const MouseEvent& e)
{
	for (int i = 0; i < EQ_BANDS; ++i) {
		auto& bounds = bandBounds[i];
		if (bounds.contains((float)e.x, (float)e.y)) {
			auto pre = prel + "eq_band" + String(i + 1);
			if (e.mods.isCommandDown()) {
				auto param = editor.audioProcessor.params.getParameter(pre + "_q");
				param->setValueNotifyingHost(param->getDefaultValue());
			}
			else {
				auto param = editor.audioProcessor.params.getParameter(pre + "_freq");
				param->setValueNotifyingHost(param->getDefaultValue());
				param = editor.audioProcessor.params.getParameter(pre + "_gain");
				param->setValueNotifyingHost(param->getDefaultValue());
			}
			repaint();
			break;
		}
	}
}

void EQDisplay::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
	if (mouse_down) return;
	auto speed = (e.mods.isShiftDown() ? 0.01f : 0.05f);
	auto slider_change = wheel.deltaY > 0 ? speed : wheel.deltaY < 0 ? -speed : 0;

	for (int i = 0; i < EQ_BANDS; ++i) {
		auto& bounds = bandBounds[i];
		if (bounds.contains((float)e.x, (float)e.y)) {
			auto pre = prel + "eq_band" + String(i + 1);
			auto param = editor.audioProcessor.params.getParameter(pre + "_q");
			param->beginChangeGesture();
			param->setValueNotifyingHost(param->getValue() + slider_change);
			while (wheel.deltaY > 0.0f && param->getValue() == 0.0f) { // FIX wheel not working when value is zero, first step takes more than 0.05%
				slider_change += 0.05f;
				param->setValueNotifyingHost(param->getValue() + slider_change);
			}
			param->endChangeGesture();
			repaint();
			break;
		}
	}
}

void EQDisplay::paint(juce::Graphics& g)
{
	g.setColour(Colour(COLOR_NEUTRAL));
	g.drawRect(viewBounds.expanded(2), 1.f);

	std::array<float, EQ_BANDS> freqs{};
	std::array<float, EQ_BANDS> gains{};
	std::array<float, EQ_BANDS> qs{};

	// draw points
	for (int i = 0; i < EQ_BANDS; ++i) {
		auto pre = prel + "eq_band" + String(i + 1);
		auto freq = editor.audioProcessor.params.getRawParameterValue(pre + "_freq")->load();
		auto gain = editor.audioProcessor.params.getRawParameterValue(pre + "_gain")->load();
		auto q = editor.audioProcessor.params.getRawParameterValue(pre + "_q")->load();

		freqs[i] = freq;
		gains[i] = gain;
		qs[i] = q;

		auto xnorm = (log(freq) - log(20.f)) / (log(20000.f) - log(20.f));
		auto ynorm = (gain + EQ_MAX_GAIN) / (EQ_MAX_GAIN * 2.f);

		auto r = 6.f;
		auto x = viewBounds.getX() + viewBounds.getWidth() * xnorm;
		auto y = viewBounds.getBottom() - viewBounds.getHeight() * ynorm;

		bandBounds[i] = { x - r, y - r, r * 2, r * 2 };
		g.setColour(Colours::white.withAlpha(selband == i ? 1.f : 0.5f));
		g.fillEllipse(bandBounds[i]);
	}

	// draw eq
	g.setColour(Colours::white);
	updateEQCurve();
	Path p;
	auto pixels = viewBounds.getWidth();
	for (int i = 0; i < pixels; ++i) {
		float mag = magPoints[i];
		float dB = 20.0f * std::log10(mag);
		float y = std::clamp((EQ_MAX_GAIN - dB) / (2.f * EQ_MAX_GAIN), 0.f, 1.f);
		if (isnan(y)) y = 0.f;
		y = viewBounds.getY() + y * viewBounds.getHeight();
		float x = viewBounds.getX() + i;

		if (i == 0) {
			p.startNewSubPath(x, y);
		}
		else {
			p.lineTo(x, y);
		}
	}
	g.strokePath(p, PathStrokeType(2.f));
	p.lineTo(viewBounds.getX() + viewBounds.getWidth(), viewBounds.getCentreY());
	p.lineTo(viewBounds.getX(), viewBounds.getCentreY());
	p.closeSubPath();
	g.setColour(Colours::white.withAlpha(0.1f));
	g.fillPath(p);

	// draw point numbers
	g.setColour(Colour(COLOR_BG));
	g.setFont(FontOptions(12.f));
	for (int i = 0; i < bandBounds.size(); ++i) {
		g.drawText(String(i + 1), bandBounds[i], Justification::centred);
	}
}

void EQDisplay::resized()
{
	auto b = getLocalBounds();
	viewBounds = b.reduced(2).toFloat();
	toggleUIComponents();
}

void EQDisplay::toggleUIComponents()
{
	repaint();
}

void EQDisplay::updateEQCurve()
{
	int numPoints = (int)viewBounds.getWidth();
	if (numPoints == 0) numPoints = 1; // Allow EQ curve to compute on init before resize
	const float minFreq = 20.0f;
	const float maxFreq = 20000.0f;

	magPoints.clear();

	auto firstBandMode = (int)editor.audioProcessor.params.getRawParameterValue(prel + "eq_band1_mode")->load();
	auto lastBandMode = (int)editor.audioProcessor.params.getRawParameterValue(prel + "eq_band" + String(EQ_BANDS) + "_mode")->load();

	for (int i = 0; i < numPoints; ++i) {
		float norm = (float)i / (numPoints - 1);
		float freq = minFreq * std::pow(maxFreq / minFreq, norm);

		float mag = 1.0f;

		for (int b = 0; b < EQ_BANDS; ++b) {
			auto pre = prel + "eq_band" + String(b + 1);
			auto srate = editor.audioProcessor.srate;
			auto cutoff = editor.audioProcessor.params.getRawParameterValue(pre + "_freq")->load();
			auto gain = editor.audioProcessor.params.getRawParameterValue(pre + "_gain")->load();
			gain = exp(gain * DB2LOG);
			auto q = editor.audioProcessor.params.getRawParameterValue(pre + "_q")->load();
			SVF::Mode mode = b == 0 && firstBandMode == 0 ? SVF::HP
				: b == 0 && firstBandMode == 1 ? SVF::LS
				: b == EQ_BANDS - 1 && lastBandMode == 0 ? SVF::LP
				: b == EQ_BANDS - 1 && lastBandMode == 1 ? SVF::HS
				: SVF::PK;

			if (mode == SVF::LP) bandFilters[b].lp((float)srate, cutoff, q);
			else if (mode == SVF::LS) bandFilters[b].ls((float)srate, cutoff, q, gain);
			else if (mode == SVF::HP) bandFilters[b].hp((float)srate, cutoff, q);
			else if (mode == SVF::HS) bandFilters[b].hs((float)srate, cutoff, q, gain);
			else bandFilters[b].pk((float)srate, cutoff, q, gain);

			mag *= bandFilters[b].getMagnitude(freq);
		}

		magPoints.push_back((mag));
	}
}
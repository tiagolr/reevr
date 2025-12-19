#include "EQDisplay.h"
#include "../PluginEditor.h"

EQDisplay::EQDisplay(REEVRAudioProcessorEditor& e, SVF::EQType _type)
	: editor(e)
	, type(_type)
	, prel(type == SVF::ParamEQ ? "post" : "decay")
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
		if (editor.audioProcessor.eqFFTReady.load(std::memory_order_acquire)) {
			editor.audioProcessor.eqFFTReady.store(false, std::memory_order_release);
			recalcFFTMags();
		}

		repaint();
	}
}

void EQDisplay::mouseDown(const MouseEvent& e)
{
	if (e.mods.isRightButtonDown()) {
		for (int i = 0; i < EQ_BANDS; ++i) {
			auto& bounds = bandBounds[i];
			if (bounds.contains((float)e.x, (float)e.y)) {
				showBandMenu(i);
				break;
			}
		}
		return;
	}

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
	if (e.mods.isRightButtonDown())
		return;

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
				param = editor.audioProcessor.params.getParameter(pre + "_mode");
				param->setValueNotifyingHost(param->getDefaultValue());
			}
			repaint();
			break;
		}
	}
}

void EQDisplay::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
	auto speed = (e.mods.isShiftDown() ? 0.01f : 0.05f);
	auto slider_change = wheel.deltaY > 0 ? speed : wheel.deltaY < 0 ? -speed : 0;

	for (int i = 0; i < EQ_BANDS; ++i) {
		auto& bounds = bandBounds[i];
		if ((mouse_down && selband == i) || (!mouse_down && bounds.contains((float)e.x, (float)e.y))) {
			auto pre = prel + "eq_band" + String(i + 1);
			auto param = editor.audioProcessor.params.getParameter(pre + "_q");
			param->setValueNotifyingHost(param->getValue() + slider_change);
			while (wheel.deltaY > 0.0f && param->getValue() == 0.0f) { // FIX wheel not working when value is zero, first step takes more than 0.05%
				slider_change += 0.05f;
				param->setValueNotifyingHost(param->getValue() + slider_change);
			}
			repaint();
			break;
		}
	}
}

void EQDisplay::paint(juce::Graphics& g)
{
	g.setColour(Colour(COLOR_NEUTRAL));
	auto expBounds = viewBounds.toFloat().expanded(2.f);
	g.drawRect(expBounds, 1.f);

	g.setFont(10.f);
	for (int i = 0; i < 8; ++i) {
		auto yy = viewBounds.getY() + (i + 1) * viewBounds.getHeight() / 8;
		g.setColour(Colour(COLOR_NEUTRAL));
		if (i == 0)
			g.drawText(type == SVF::ParamEQ ? "+18" : "+75", Rectangle<float>(viewBounds.getX() + 1, yy - 10 - 1, 20, 20), Justification::centred);
		else if (i == 1)
			g.drawText(type == SVF::ParamEQ ? "+12" : "+50", Rectangle<float>(viewBounds.getX() + 1, yy - 10 - 1, 20, 20), Justification::centred);
		else if (i == 2)
			g.drawText(type == SVF::ParamEQ ? "+6" : "+25", Rectangle<float>(viewBounds.getX() + 1, yy - 10 - 1, 20, 20), Justification::centred);
		else if (i == 4)
			g.drawText(type == SVF::ParamEQ ? "-6" : "-25", Rectangle<float>(viewBounds.getX() + 1, yy - 10 - 1, 20, 20), Justification::centred);
		else if (i == 5)
			g.drawText(type == SVF::ParamEQ ? "-12" : "-50", Rectangle<float>(viewBounds.getX() + 1, yy - 10 - 1, 20, 20), Justification::centred);
		else if (i == 6)
			g.drawText(type == SVF::ParamEQ ? "-18" : "-75", Rectangle<float>(viewBounds.getX() + 1, yy - 10 - 1, 20, 20), Justification::centred);

		g.setColour(Colour(COLOR_NEUTRAL).withAlpha(0.25f));
		g.drawHorizontalLine((int)yy, expBounds.getX() + 25, expBounds.getRight());
	}

	const float logMin = std::log(20.f);
	const float logScale = 1.f / (std::log(20000.f) - logMin);

	std::vector<float> xpos = { 50.f, 200.f, 800.f, 3000.f, 10000.f };
	for (auto& pos : xpos) {
		auto xnorm = (std::log(pos) - logMin) * logScale;
		float xx = viewBounds.getX() + xnorm * viewBounds.getWidth();
		g.setColour(Colour(COLOR_NEUTRAL).withAlpha(0.25f));
		g.drawVerticalLine(int(xx), expBounds.getY(), expBounds.getBottom());
		g.setColour(Colour(COLOR_NEUTRAL));

		String txt = pos >= 1000 ? String(pos / 1000.f, 0) + "k" : String(pos);
		g.drawText(txt, Rectangle<float>(xx + 1, viewBounds.getBottom() - 20 - 1, 30, 20), Justification::bottomLeft);
	}

	drawWaveform(g);

	std::array<float, EQ_BANDS> freqs{};
	std::array<float, EQ_BANDS> gains{};
	std::array<float, EQ_BANDS> qs{};

	// draw points
	for (int i = 0; i < EQ_BANDS; ++i) {
		auto pre = prel + "eq_band" + String(i + 1);
		auto freq = editor.audioProcessor.params.getRawParameterValue(pre + "_freq")->load();
		auto gain = editor.audioProcessor.params.getRawParameterValue(pre + "_gain")->load();
		auto q = editor.audioProcessor.params.getRawParameterValue(pre + "_q")->load();
		auto bypass = (bool)editor.audioProcessor.params.getRawParameterValue(pre + "_bypass")->load();

		freqs[i] = freq;
		gains[i] = gain;
		qs[i] = q;

		auto xnorm = (std::log(freq) - logMin) * logScale;
		auto ynorm = (gain + EQ_MAX_GAIN) / (EQ_MAX_GAIN * 2.f);

		auto r = 6.f;
		auto x = viewBounds.getX() + viewBounds.getWidth() * xnorm;
		auto y = viewBounds.getBottom() - viewBounds.getHeight() * ynorm;

		bandBounds[i] = { x - r, y - r, r * 2, r * 2 };
		g.setColour(Colours::white.withAlpha(selband == i ? 1.f : 0.5f));
		if (bypass)
			g.drawEllipse(bandBounds[i], 2.f);
		else 
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
		if (std::isnan(y)) y = 0.f;
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

void EQDisplay::drawWaveform(juce::Graphics& g)
{
	auto size = fftMagnitudes.size();
	auto bounds = viewBounds;

	if (size == 0)
		return;

	juce::Path waveformPath;

	const float minDB = -90.0f;
	const float maxDB = 0.0f;
	const float minFreq = 20.0f;
	const float maxFreq = 20000.f;
	const float srate = static_cast<float>(editor.audioProcessor.srate);

	for (size_t i = 0; i < size; ++i) {
		float freq = (i * srate) / (2.0f * (size - 1));
		freq = std::max(freq, minFreq);
		float logPos = std::log10(freq / minFreq) / std::log10(maxFreq / minFreq);
		float x = bounds.getX() + logPos * bounds.getWidth();

		float magnitudeDB = juce::Decibels::gainToDecibels(fftMagnitudes[i], minDB);
		float y = juce::jmap(magnitudeDB, minDB, maxDB, bounds.getBottom(), bounds.getY());

		if (i == 0)
			waveformPath.startNewSubPath(x, y);
		else
			waveformPath.lineTo(x, y);
	}

	g.setColour(Colour(COLOR_ACTIVE));
	g.strokePath(waveformPath, juce::PathStrokeType(1.0f));

	// hide zero values
	g.setColour(Colour(COLOR_BG));
	g.fillRect(bounds.toFloat().expanded(1.f).withHeight(2.f).withBottomY((float)bounds.getBottom() + 1));
}

void EQDisplay::recalcFFTMags()
{
	auto fftSize = 1 << EQ_FFT_ORDER;
	auto writeIndex = editor.audioProcessor.eqWriteIndex;
	auto bufferSize = fftData.size();
	size_t startIndex = (writeIndex + bufferSize - fftSize) % bufferSize;

	if (startIndex + fftSize <= bufferSize) {
		std::copy_n(&editor.audioProcessor.eqBuffer[startIndex], fftSize, fftData.data());
	}
	else {
		// Wrap-around copy
		size_t firstPart = bufferSize - startIndex;
		std::copy_n(&editor.audioProcessor.eqBuffer[startIndex], firstPart, fftData.data());
		std::copy_n(&editor.audioProcessor.eqBuffer[0], fftSize - firstPart, fftData.data() + firstPart);
	}

	window.multiplyWithWindowingTable(fftData.data(), fftData.size());
	fft.performFrequencyOnlyForwardTransform(fftData.data(), true);
	float norm = 1.f / (fftData.size() * 0.5f);
	for (size_t j = 0; j < fftSize / 2; ++j) {
	    float mag = fftData[j] * norm;
	    fftMagnitudes[j] = mag * 0.8f + fftMagnitudes[j] * 0.2f;
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


	for (int i = 0; i < numPoints; ++i) {
		float norm = (float)i / (numPoints - 1);
		float freq = minFreq * std::pow(maxFreq / minFreq, norm);
		float mag = 1.0f;

		for (int b = 0; b < EQ_BANDS; ++b) {
			auto pre = prel + "eq_band" + String(b + 1);
			auto m = (int)editor.audioProcessor.params.getRawParameterValue(pre +  "_mode")->load();
			auto srate = editor.audioProcessor.srate;
			auto cutoff = editor.audioProcessor.params.getRawParameterValue(pre + "_freq")->load();
			auto gain = editor.audioProcessor.params.getRawParameterValue(pre + "_gain")->load();
			gain = exp(gain * DB2LOG);
			auto q = editor.audioProcessor.params.getRawParameterValue(pre + "_q")->load();
			auto bypass = (bool)editor.audioProcessor.params.getRawParameterValue(pre + "_bypass")->load();
			if (bypass) continue;
			SVF::Mode mode = b == 0 && m == 0 ? SVF::HP
				: b == 0 && m > 0 ? SVF::LS
				: b == EQ_BANDS - 1 && m == 0 ? SVF::LP
				: b == EQ_BANDS - 1 && m > 0 ? SVF::HS
				: m == 0 ? SVF::BP : m == 1 ? SVF::PK 
				: SVF::BS;

			if (mode == SVF::LP) bandFilters[b].lp((float)srate, cutoff, q);
			else if (mode == SVF::LS) bandFilters[b].ls((float)srate, cutoff, q, gain);
			else if (mode == SVF::HP) bandFilters[b].hp((float)srate, cutoff, q);
			else if (mode == SVF::HS) bandFilters[b].hs((float)srate, cutoff, q, gain);
			else if (mode == SVF::BP) bandFilters[b].bp((float)srate, cutoff, q);
			else if (mode == SVF::BS) bandFilters[b].bs((float)srate, cutoff, q);
			else bandFilters[b].pk((float)srate, cutoff, q, gain);

			mag *= bandFilters[b].getMagnitude(freq);
		}

		magPoints.push_back((mag));
	}
}

void EQDisplay::showBandMenu(int band)
{
	PopupMenu menu;
	auto pre = prel + "eq_band" + String(band + 1);
	bool bypass = (bool)editor.audioProcessor.params.getRawParameterValue(pre + "_bypass")->load();
	menu.addItem(100, "Bypass", true, bypass);

	auto mode = (int)editor.audioProcessor.params.getRawParameterValue(pre + "_mode")->load();
	if (band == 0 && mode == 0)
		menu.addItem(1, "Low Shelf");
	else if (band == 0 && mode > 0)
		menu.addItem(2, "Low Cut");
	else if (band == EQ_BANDS - 1 && mode == 0)
		menu.addItem(3, "High Shelf");
	else if (band == EQ_BANDS - 1 && mode > 0)
		menu.addItem(4, "High Cut");
	else if (mode == 0) {
		menu.addItem(5, "Peak");
		menu.addItem(7, "Notch");
	}
	else if (mode == 1) {
		menu.addItem(6, "Band Pass");
		menu.addItem(7, "Notch");
	}
	else if (mode == 2) {
		menu.addItem(5, "Peak");
		menu.addItem(6, "Band Pass");
	}


	auto mousePos = localPointToGlobal(bandBounds[band].getBottomLeft().toInt());
	menu.showMenuAsync(
		juce::PopupMenu::Options()
		.withTargetComponent(*this)
		.withTargetScreenArea({ mousePos.getX(), mousePos.getY(), 1,1 }), 
		[this, pre, bypass](int result)
		{
			if (result == 0) return;
			auto param = editor.audioProcessor.params.getParameter(pre + "_mode");
			if (result == 2 || result == 4 || result == 6) {
				param->setValueNotifyingHost(0.f);
			}
			if (result == 1 || result == 3 || result == 5) {
				param->setValueNotifyingHost(param->convertTo0to1(1.f));
			}
			if (result == 7) {
				param->setValueNotifyingHost(param->convertTo0to1(2.f));
			}
			if (result == 100) {
				auto p = editor.audioProcessor.params.getParameter(pre + "_bypass");
				p->setValueNotifyingHost(bypass ? 0.f : 1.f);
			}
		}
	);
}
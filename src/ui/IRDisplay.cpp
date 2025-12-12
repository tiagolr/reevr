#include "IRDisplay.h"
#include "../PluginProcessor.h"

IRDisplay::IRDisplay(REEVRAudioProcessor& p) : audioProcessor(p)
{
	startTimerHz(15);
	attack = audioProcessor.params.getRawParameterValue("irattack")->load();
	decay = audioProcessor.params.getRawParameterValue("irdecay")->load();
	trimLeft = audioProcessor.params.getRawParameterValue("irtrimleft")->load();
	trimRight = audioProcessor.params.getRawParameterValue("irtrimright")->load();

	audioProcessor.params.addParameterListener("irattack", this);
	audioProcessor.params.addParameterListener("irdecay", this);
	audioProcessor.params.addParameterListener("irtrimleft", this);
	audioProcessor.params.addParameterListener("irtrimright", this);

	addAndMakeVisible(reverseButton);
	reverseButton.setTooltip("Reverse IR");
	reverseButton.setBounds(getLocalBounds().getRight()-25, getLocalBounds().getY() + 5, 20, 20);
	reverseButton.setAlpha(0.f);
	reverseButton.onClick = [this] {
		bool reverse = (bool)audioProcessor.params.getRawParameterValue("irreverse")->load();
		audioProcessor.params.getParameter("irreverse")->setValueNotifyingHost((float)!reverse);
	};
}


IRDisplay::~IRDisplay()
{
	audioProcessor.params.removeParameterListener("irattack", this);
	audioProcessor.params.removeParameterListener("irdecay", this);
	audioProcessor.params.removeParameterListener("irtrimleft", this);
	audioProcessor.params.removeParameterListener("irtrimright", this);
}

void IRDisplay::timerCallback()
{
	if (audioProcessor.impulse->version != impulseVersion) {
		recalcWave();
		repaint();
		impulseVersion = audioProcessor.impulse->version;
	}
}

bool IRDisplay::isInterestedInFileDrag(const juce::StringArray& files)
{
	// Return true if you want to accept these files (e.g., check extensions)
	for (const auto& file : files)
	{
		auto f = File(file);
		auto ext = f.getFileExtension().substring(1).toLowerCase();
		AudioFormatManager manager;
		manager.registerBasicFormats();
		if (manager.findFormatForFileExtension(ext)) {
			return true;
		}
	}
	return false;
}

void IRDisplay::filesDropped(const juce::StringArray& files, int x, int y)
{
	(void)x;
	(void)y;

	for (const auto& file : files)
	{
		AudioFormatManager mgr;
		auto f = File(file);
		auto ext = f.getFileExtension().substring(1).toLowerCase();
		mgr.registerBasicFormats();
		if (mgr.findFormatForFileExtension(ext)) {
			audioProcessor.loadImpulse(file);
			return;
		}
	}
}

void IRDisplay::parameterChanged(const juce::String& parameterID, float newValue)
{
	if (parameterID == "irattack") attack = newValue;
	if (parameterID == "irdecay") decay = newValue;
	if (parameterID == "irtrimleft") trimLeft = newValue;
	if (parameterID == "irtrimright") trimRight = newValue;
	MessageManager::callAsync([this]{ repaint();});
}

void IRDisplay::recalcWave()
{
	auto bufl = audioProcessor.impulse->bufferLL;
	auto bufr = audioProcessor.impulse->bufferRR;
	int trimLeftSamples = audioProcessor.impulse->trimLeftSamples;
	int trimRightSamples = audioProcessor.impulse->trimRightSamples;

	// normalize
	auto peak = audioProcessor.impulse->peak;
	if (peak > 0.0f) {
		for (int i = 0; i < bufl.size(); ++i) {
			bufl[i] /= peak;
			bufr[i] /= peak;
		}
	}

	bufl.insert(bufl.begin(), trimLeftSamples, 0.0);
	bufr.insert(bufr.begin(), trimLeftSamples, 0.0);
	bufl.insert(bufl.end(), trimRightSamples, 0.0);
	bufr.insert(bufr.end(), trimRightSamples, 0.0);

	std::fill(waveLeft.begin(), waveLeft.end(), 0.f);
	std::fill(waveRight.begin(), waveRight.end(), 0.f);
	float displayScale = float(waveLeft.size()) / (float)bufl.size();
	for (int i = 0; i < bufl.size(); ++i) {
		int index = (int)(displayScale * i);
		auto valueL = std::fabs(bufl[i]);
		auto valueR = std::fabs(bufr[i]);
		if (waveLeft[index] < valueL) waveLeft[index] = valueL;
		if (waveRight[index] < valueR) waveRight[index] = valueR;
	}
}

void IRDisplay::resized() 
{
	auto bounds = getDisplayBounds();
	waveLeft.resize(bounds.getWidth(), 0.0);
	waveRight.resize(bounds.getWidth(), 0.0);
	reverseButton.setBounds(reverseButton.getBounds().withRightX(bounds.getRight() - 5).withY(bounds.getY() + 5));
	recalcWave();
}

void IRDisplay::paint(juce::Graphics& g)
{
	auto bounds = getDisplayBounds().toFloat();
	auto bright = getTrimRightBounds().toFloat();
	auto bleft = getTrimLeftBounds().toFloat();
	auto batk = getAttackBounds().toFloat();
	auto bdec = getDecayBounds().toFloat();

	g.setColour(Colour(COLOR_ACTIVE).withAlpha(0.15f));
	if (attack > 0.f && mouseover) {
		Path pp;
		pp.startNewSubPath(bleft.getCentreX(), bounds.getCentreY());
		pp.lineTo(batk.getCentreX(), batk.getCentreY());
		pp.lineTo(batk.getCentreX(), bounds.getBottom());
		pp.closeSubPath();
		g.fillPath(pp);
	}
	if (decay > 0.f && mouseover) {
		Path pp;
		pp.startNewSubPath(bright.getCentreX(), bounds.getCentreY());
		pp.lineTo(bdec.getCentreX(), bdec.getCentreY());
		pp.lineTo(bdec.getCentreX(), bounds.getBottom());
		pp.closeSubPath();
		g.fillPath(pp);
	}
	auto left = (int)batk.getCentreX();
	auto right = (int)bdec.getCentreX();
	if (left < right && mouseover)
		g.fillRect(Rectangle<int>(left, (int)batk.getCentreY(), (int)bounds.getWidth(), (int)bounds.getHeight()).withRight(right));
	

	auto x = bounds.getX();
	auto cy = (float)std::floor(bounds.getCentreY());
	auto h = bounds.getHeight();
	Path l;
	Path r;
	l.startNewSubPath(x, cy);
	r.startNewSubPath(x, cy);
	for (int i = 0; i < waveLeft.size(); ++i) {
		l.lineTo(x + i, cy - h / 2 * waveLeft[i]);
		r.lineTo(x + i, cy + h / 2 * waveRight[i]);
	}
	l.lineTo(x + (float)waveLeft.size(), cy);
	r.lineTo(x + (float)waveLeft.size(), cy);
	l.closeSubPath();
	r.closeSubPath();
	g.setColour(Colours::white.withAlpha(.8f));
	g.fillPath(l);
	g.fillPath(r);

	g.setColour(Colour(COLOR_NEUTRAL));
	g.drawRect(bounds, 1.f);
	Path p;
	g.setColour(Colours::white);
	
	p.addTriangle(bright.getTopLeft().translated(bright.getWidth() / 2.f, 0.f), bright.getBottomLeft(), bright.getBottomRight());
	p.addTriangle(bleft.getTopLeft().translated(bleft.getWidth()/2.f, 0.f), bleft.getBottomLeft(), bleft.getBottomRight());
	g.fillPath(p);

	g.setColour(Colours::white);
	g.fillEllipse(batk);
	g.fillEllipse(bdec);

	g.setColour(Colour(audioProcessor.impulse->reverse ? COLOR_ACTIVE : COLOR_NEUTRAL));
	bounds = reverseButton.getBounds().toFloat();
	bounds.removeFromRight(6);
	auto rr = 3.f;
	Path arrows;
	arrows.startNewSubPath(bounds.getRight(), bounds.getCentreY() - 4);
	arrows.lineTo(bounds.getX(), bounds.getCentreY() - 4);
	arrows.startNewSubPath(bounds.getX() + rr, bounds.getCentreY() - 4 - rr);
	arrows.lineTo(bounds.getX(), bounds.getCentreY() - 4);
	arrows.lineTo(bounds.getX()+rr, bounds.getCentreY() - 4 + rr);

	arrows.startNewSubPath(bounds.getRight(), bounds.getCentreY() + 4);
	arrows.lineTo(bounds.getX(), bounds.getCentreY() + 4);
	arrows.startNewSubPath(bounds.getRight() - rr, bounds.getCentreY() + 4 - rr);
	arrows.lineTo(bounds.getRight(), bounds.getCentreY() + 4);
	arrows.lineTo(bounds.getRight()-rr, bounds.getCentreY() + 4 + rr);

	g.strokePath(arrows, PathStrokeType(1.f));
}

void IRDisplay::mouseDown(const juce::MouseEvent& e)
{
	dragging = 0;
	auto triml = getTrimLeftBounds();
	auto trimr = getTrimRightBounds();
	auto atk = getAttackBounds();
	auto dec = getDecayBounds();

	if (trimr.expanded(2, 2).contains(e.getPosition())) {
		dragging = 2;
	}
	else if (triml.expanded(2, 2).contains(e.getPosition())) {
		dragging = 1;
	}
	else if (dec.expanded(4, 4).contains(e.getPosition())) {
		dragging = 4;
	}
	else if (atk.expanded(4, 4).contains(e.getPosition())) {
		dragging = 3;
	}

	if (dragging) {
		last_mouse_position = e.getPosition();
		e.source.enableUnboundedMouseMovement(true);
		setMouseCursor(MouseCursor::NoCursor);
		auto param = getDragParam();
		param->beginChangeGesture();
	}
}

void IRDisplay::mouseUp(const juce::MouseEvent& e)
{
	if (dragging) {
		e.source.enableUnboundedMouseMovement(false);
		setMouseCursor(MouseCursor::NormalCursor);
		auto param = getDragParam();
		param->endChangeGesture();
		dragging = 0;
	}
}

void IRDisplay::mouseDrag(const juce::MouseEvent& e)
{
	if (!dragging) return;
		
	auto change = e.getPosition() - last_mouse_position;
	last_mouse_position = e.getPosition();
	auto speed = (e.mods.isShiftDown() ? 40.0f : 4.0f) * 100;
	auto slider_change = float(change.getX()) / speed;
	if (dragging == 2 || dragging == 4)
		slider_change *= -1;
	auto param = getDragParam();
	param->setValueNotifyingHost(param->getValue() + slider_change);
}

void IRDisplay::mouseEnter(const MouseEvent& e)
{
	(void)e;
	mouseover = true;
	repaint();
}
void IRDisplay::mouseExit(const MouseEvent& e) 
{
	(void)e;
	mouseover = false;
	repaint();
}

RangedAudioParameter* IRDisplay::getDragParam() 
{
	auto pname = dragging == 1 ? "irtrimleft" 
		: dragging == 2 ? "irtrimright"
		: dragging == 3 ? "irattack"
		: "irdecay";

	return audioProcessor.params.getParameter(pname);
}

Rectangle<int> IRDisplay::getDisplayBounds()
{
	return getLocalBounds().expanded(-8, -8);
}
Rectangle<int> IRDisplay::getTrimLeftBounds() 
{
	auto r = 8;
	auto bounds = getDisplayBounds();
	auto pt = bounds.getBottomLeft().translated(-r/2, 0).translated((int)(trimLeft * bounds.getWidth()), 0);
	return Rectangle<int>(pt.x, pt.y, r, r);
}
Rectangle<int> IRDisplay::getTrimRightBounds() 
{
	auto r = 8;
	auto bounds = getDisplayBounds();
	auto pt = bounds.getBottomRight().translated(-r/2, 0).translated((int)(-trimRight * bounds.getWidth()), 0);
	return Rectangle<int>(pt.x, pt.y, r, r);
}

Rectangle<int> IRDisplay::getAttackBounds()
{
	auto r = 6;
	auto bounds = getDisplayBounds();
	auto w = bounds.getWidth() - trimLeft * bounds.getWidth() - trimRight * bounds.getWidth();
	auto pt = bounds.getTopLeft().translated(-r/2, -r/2).translated((int)(trimLeft * bounds.getWidth() + attack * w), 0);
	return Rectangle<int>(pt.x, pt.y, r, r);
}

Rectangle<int> IRDisplay::getDecayBounds()
{
	auto r = 6;
	auto bounds = getDisplayBounds();
	auto w = bounds.getWidth() - trimLeft * bounds.getWidth() - trimRight * bounds.getWidth();
	auto pt = bounds.getTopRight().translated(-r/2, -r/2).translated((int)(-trimRight * bounds.getWidth() - decay * w), 0);
	return Rectangle<int>(pt.x, pt.y, r, r);
}
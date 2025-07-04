#include "About.h"

void About::mouseDown(const juce::MouseEvent& e)
{
	(void)e;
	setVisible(false);
};

void About::paint(Graphics& g)
{
	auto bounds = getBounds();
	g.setColour(Colour(0xdd000000));
	g.fillRect(bounds);

	bounds.reduce(50,50);
	g.setColour(Colours::white);
	g.setFont(FontOptions(30.f));
	g.drawText("REEV-R", bounds.removeFromTop(35), Justification::centred);
	g.setFont(FontOptions(20.f));
	g.drawText(std::string("v") + PROJECT_VERSION, bounds.removeFromTop(25), Justification::centred);
	g.setFont(FontOptions(20.0f));
	g.drawText("Copyright (C) Tilr 2025", bounds.removeFromTop(25), Justification::centred);
	g.setColour(Colour(COLOR_ACTIVE));
	g.drawText("github.com/tiagolr/reevr", bounds.removeFromTop(25), Justification::centred);
	g.setColour(Colours::white);
	bounds.removeFromTop(40);
	auto w = PLUG_WIDTH - 100;
	bounds.setWidth(w);
	bounds.setX(getWidth() / 2 - w/2);
	g.drawText("- Shift for fine slider adjustments.", bounds.removeFromTop(25), Justification::centredLeft);
	g.drawText("- Shift toggles snap on/off.", bounds.removeFromTop(25), Justification::centredLeft);
	g.drawText("- Mouse wheel on view changes grid size.", bounds.removeFromTop(25), Justification::centredLeft);
	g.drawText("- Right click points changes point type.", bounds.removeFromTop(25), Justification::centredLeft);
	g.drawText("- Alt click to insert new points.", bounds.removeFromTop(25), Justification::centredLeft);
	g.drawText("- Alt + drag selection handles skews selected points.", bounds.removeFromTop(25), Justification::centredLeft);
	g.drawText("- Right click + drag in paint mode changes paint tool tension", bounds.removeFromTop(25), Justification::centredLeft);
	g.drawText("- Shit + wheel to change sequencer step size.", bounds.removeFromTop(25), Justification::centredLeft);
};


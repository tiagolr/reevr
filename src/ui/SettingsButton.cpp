#include "SettingsButton.h"
#include "../PluginProcessor.h"
#include "../Globals.h"

void SettingsButton::paint(Graphics& g) 
{
	auto r = 1.5f;
	auto bounds = getLocalBounds().expanded(-2,-4).toFloat();
	g.setColour(Colour(globals::COLOR_ACTIVE));
	g.fillRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), r * 2, 2.f);
	g.fillRoundedRectangle(bounds.getX(), bounds.getCentreY() - r, bounds.getWidth(), r*2, 2.f);
	g.fillRoundedRectangle(bounds.getX(), bounds.getBottom() - r*2, bounds.getWidth(), r*2, 2.f);
};

void SettingsButton::mouseDown(const juce::MouseEvent& e)
{
	(void)e;

	PopupMenu uiScale;
	uiScale.addItem(1, "100%", true, audioProcessor.scale == 1.0f);
	uiScale.addItem(2, "125%", true, audioProcessor.scale == 1.25f);
	uiScale.addItem(3, "150%", true, audioProcessor.scale == 1.5f);
	uiScale.addItem(4, "175%", true, audioProcessor.scale == 1.75f);
	uiScale.addItem(5, "200%", true, audioProcessor.scale == 2.0f);

	PopupMenu midiTriggerChn;
	midiTriggerChn.addItem(2010, "Off", true, audioProcessor.midiTriggerChn == -1);
	for (int i = 0; i < 16; i++) {
		midiTriggerChn.addItem(2010 + i + 1, String(i + 1), true, audioProcessor.midiTriggerChn == i);
	}
	midiTriggerChn.addItem(2027, "Any", true, audioProcessor.midiTriggerChn == 16);

	PopupMenu triggerChn;
	triggerChn.addItem(10, "Off", true, audioProcessor.triggerChn == -1);
	for (int i = 0; i < 16; i++) {
		triggerChn.addItem(10 + i + 1, String(i + 1), true, audioProcessor.triggerChn == i);
	}
	triggerChn.addItem(27, "Any", true, audioProcessor.triggerChn == 16);

	PopupMenu audioTrigger;
	audioTrigger.addItem(32, "Ignore hits while playing", true, audioProcessor.audioIgnoreHitsWhilePlaying);

	PopupMenu CC;
	CC.addItem(300, "Off", true, audioProcessor.outputCC == 0);
	CC.addSeparator();
	for (int i = 1; i <= 128; ++i) {
		CC.addItem(300 + i,  String(i - 1), true, audioProcessor.outputCC == i);
	}

	PopupMenu CCChan;
	for (int i = 0; i < 16; ++i) {
		CCChan.addItem(450 + i, String(i+1), true, audioProcessor.outputCCChan == i);
	}

	auto midiNoteToName = [](int noteNumber) -> std::string {
		const std::string noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
		int octave = (noteNumber / 12) - 1;
		std::string noteName = noteNames[noteNumber % 12];
		return std::to_string(noteNumber) + " " + noteName + std::to_string(octave);
	};

	PopupMenu audioOutputMIDI;
	audioOutputMIDI.addItem(500, "Off", true, audioProcessor.outputATMIDI == 0);
	audioOutputMIDI.addSeparator();
	for (int i = 1; i < 129; ++i) {
		audioOutputMIDI.addItem(500+i, midiNoteToName(i-1), true, audioProcessor.outputATMIDI == i);
	}

	PopupMenu output;
	output.addItem(700, "CV", true, audioProcessor.outputCV);
	output.addSubMenu("CC", CC);
	output.addSubMenu("CC Channel", CCChan);
	output.addSubMenu("Audio Trig. MIDI", audioOutputMIDI);
	output.addSeparator();
	output.addItem(701, "Bipolar CC", true, audioProcessor.bipolarCC);

	PopupMenu options;
	options.addSubMenu("Output", output);
	options.addSubMenu("MIDI trigger chn", midiTriggerChn);
	options.addSubMenu("Patt trigger chn", triggerChn);
	options.addSubMenu("Audio trigger", audioTrigger);
	options.addSeparator();
	options.addItem(30, "Dual smooth", true, audioProcessor.dualSmooth);
	options.addItem(31, "Dual tension", true, audioProcessor.dualTension);


	PopupMenu load;
	load.addItem(100, "Sine", audioProcessor.uimode != UIMode::Seq);
	load.addItem(101, "Triangle", audioProcessor.uimode != UIMode::Seq);
	load.addItem(102, "Random", audioProcessor.uimode != UIMode::Seq);
	load.addSeparator();
	load.addItem(110, "Init");

	PopupMenu presets;

	presets.addItem(111, "Clear Tails");
	presets.addItem(112, "Envelopes");
	presets.addItem(113, "Rising");
	presets.addItem(114, "Waves");
	presets.addItem(115, "Offbeat");
	presets.addItem(116, "Bunker");
	presets.addItem(117, "Gated 1");
	presets.addItem(118, "Gated 2");
	presets.addItem(119, "Gated 3");
	presets.addItem(120, "Gated 4");

	PopupMenu loadOther;
	loadOther.addItem(150, "Restore paint patterns");

	load.addSubMenu("Presets", presets);
	load.addSeparator();
	load.addSubMenu("Other", loadOther);
	

	PopupMenu menu;
	auto menuPos = localPointToGlobal(getLocalBounds().getBottomRight());
	menu.addSubMenu("UI Scale", uiScale);
	menu.addSubMenu("Options", options);
	menu.addSeparator();
	menu.addItem(53, "Copy", audioProcessor.uimode != UIMode::Seq);
	menu.addItem(54, "Paste", audioProcessor.uimode != UIMode::Seq);
	menu.addItem(55, "Invert", audioProcessor.uimode != UIMode::Seq);
	menu.addItem(56, "Reverse", audioProcessor.uimode != UIMode::Seq);
	menu.addItem(57, "Double");
	menu.addItem(52, audioProcessor.uimode == UIMode::Seq ? "Reset" : "Clear");
	menu.addSeparator();
	menu.addSubMenu("Load", load);
	menu.addItem(1000, "About");
	menu.showMenuAsync(PopupMenu::Options()
		.withTargetScreenArea({menuPos.getX() -110, menuPos.getY(), 1, 1}),
		[this](int result) {
			if (result == 0) return;
			else if (result >= 1 && result <= 5) { // UI Scale
				audioProcessor.setScale(result == 5 ? 2.0f : result == 4 ? 1.75f : result == 3 ? 1.5f : result == 2 ? 1.25f : 1.0f);
				onScaleChange();
			}
			else if (result >= 2010 && result <= 2027) {
				audioProcessor.midiTriggerChn = result - 2010 - 1;
			}
			else if (result >= 10 && result <= 27) { // Trigger channel
				audioProcessor.triggerChn = result - 10 - 1;
			}
			else if (result == 30) { // Dual smooth
				audioProcessor.dualSmooth = !audioProcessor.dualSmooth;
				toggleUIComponents();
			}
			else if (result == 31) { // Dual tension
				MessageManager::callAsync([this]() {
					audioProcessor.dualTension = !audioProcessor.dualTension;
					audioProcessor.onTensionChange();
					toggleUIComponents();
				});
			}
			else if (result == 32) {
				MessageManager::callAsync([this]() {
					audioProcessor.audioIgnoreHitsWhilePlaying = !audioProcessor.audioIgnoreHitsWhilePlaying;
				});
			}
			else if (result == 52) {
				if (audioProcessor.uimode == UIMode::Seq) {
					auto snap = audioProcessor.sequencer->cells;
					audioProcessor.sequencer->clear();
					audioProcessor.sequencer->createUndo(snap);
					audioProcessor.sequencer->build();
				}
				else {
					auto snapshot = audioProcessor.viewPattern->points;
					audioProcessor.viewPattern->clear();
					audioProcessor.viewPattern->buildSegments();
					audioProcessor.createUndoPointFromSnapshot(snapshot);
				}
			}
			else if (result == 53) {
				audioProcessor.viewPattern->copy();
			}
			else if (result == 54) {
				auto snapshot = audioProcessor.viewPattern->points;
				audioProcessor.viewPattern->paste();
				audioProcessor.viewPattern->buildSegments();
				audioProcessor.createUndoPointFromSnapshot(snapshot);
			}
			else if (result == 55) {
				auto snapshot = audioProcessor.viewPattern->points;
				audioProcessor.viewPattern->invert();
				audioProcessor.viewPattern->buildSegments();
				audioProcessor.createUndoPointFromSnapshot(snapshot);
			}
			else if (result == 56) {
				auto snapshot = audioProcessor.viewPattern->points;
				audioProcessor.viewPattern->reverse();
				audioProcessor.viewPattern->buildSegments();
				audioProcessor.createUndoPointFromSnapshot(snapshot);
			}
			else if (result == 57) {
				if (audioProcessor.uimode == UIMode::Seq) {
					audioProcessor.sequencer->doublePattern();
				}
				else {
					auto snapshot = audioProcessor.viewPattern->points;
					audioProcessor.viewPattern->doublePattern();
					audioProcessor.viewPattern->buildSegments();
					audioProcessor.createUndoPointFromSnapshot(snapshot);
				}
			}
			else if (result == 110) {
				audioProcessor.loadProgram(0);
			}
			else if (result >= 100 && result <= 200) { // load
				if (result == 100) { // load sine
					audioProcessor.viewPattern->loadSine();
					audioProcessor.viewPattern->buildSegments();
				}
				if (result == 101) { // load triangle
					audioProcessor.viewPattern->loadTriangle();
					audioProcessor.viewPattern->buildSegments();
				}
				if (result == 102) { // load random
					int grid = audioProcessor.getCurrentGrid();
					audioProcessor.viewPattern->loadRandom(grid);
					audioProcessor.viewPattern->buildSegments();
				}
				if (result >= 111 && result <= 120) {
					MessageManager::callAsync([this, result]() {
						audioProcessor.loadProgram(result-110);
					});
				}
				if (result == 150) {
					audioProcessor.restorePaintPatterns();
				}
			}
			// output cc channel
			else if (result >= 300 && result <= 300 + 129) {
				audioProcessor.outputCC = result - 300;
			}
			else if (result >= 450 && result <= 450 + 16) {
				audioProcessor.outputCCChan = result - 450;
			}
			// output audio trigger midi note
			else if (result >= 500 && result <= 500 + 129) {
				audioProcessor.outputATMIDI = result - 500;
			}
			// output options
			else if (result == 700) { 
				audioProcessor.outputCV = !audioProcessor.outputCV;
			}
			else if (result == 701) {
				audioProcessor.bipolarCC = !audioProcessor.bipolarCC;
			}
			else if (result == 1000) {
				toggleAbout();
			}
		}
	);
};


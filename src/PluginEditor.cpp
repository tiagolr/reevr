// Copyright 2025 tilr

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Globals.h"

REEVRAudioProcessorEditor::REEVRAudioProcessorEditor (REEVRAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , audioProcessor (p)
{
    audioProcessor.loadSettings(); // load saved paint patterns from other plugin instances
    setResizable(true, false);
    setResizeLimits(PLUG_WIDTH, PLUG_HEIGHT, MAX_PLUG_WIDTH, MAX_PLUG_HEIGHT);
    setSize (audioProcessor.plugWidth, audioProcessor.plugHeight);
    setScaleFactor(audioProcessor.scale);

    audioProcessor.addChangeListener(this);
    audioProcessor.params.addParameterListener("sync", this);
    audioProcessor.params.addParameterListener("trigger", this);
    audioProcessor.params.addParameterListener("revenvon", this);
    audioProcessor.params.addParameterListener("sendenvon", this);
    audioProcessor.params.addParameterListener("irlowcutslope", this);
    audioProcessor.params.addParameterListener("irhighcutslope", this);
    audioProcessor.params.addParameterListener("predelayusesync", this);

    auto col = PLUG_PADDING;
    auto row = PLUG_PADDING;

    // FIRST ROW

    addAndMakeVisible(logoLabel);
    logoLabel.setColour(juce::Label::ColourIds::textColourId, Colours::white);
    logoLabel.setFont(FontOptions(26.0f));
    logoLabel.setText("REEV-R", NotificationType::dontSendNotification);
    logoLabel.setBounds(col-5, row-3, 95, 30);
    col += 100;

#if defined(DEBUG)
    addAndMakeVisible(presetExport);
    presetExport.setAlpha(0.f);
    presetExport.setTooltip("DEBUG ONLY - Exports preset to debug console");
    presetExport.setButtonText("Export");
    presetExport.setBounds(10, 10, 100, 25);
    presetExport.onClick = [this] {
        //std::ostringstream oss;
        //auto points = audioProcessor.viewPattern->points;
        //for (const auto& point : points) {
        //    oss << point.x << " " << point.y << " " << point.tension << " " << point.type << " ";
        //}
        //DBG(oss.str() << "\n");
        //std::ostringstream oss2;
        //points = audioProcessor.sendpattern->points;
        //for (const auto& point : points) {
        //    oss2 << point.x << " " << point.y << " " << point.tension << " " << point.type << " ";
        //}
        //DBG(oss2.str() << "\n");
        juce::MemoryBlock stateData;
        processor.getStateInformation(stateData);

        if (auto xml = AudioProcessor::getXmlFromBinary(stateData.getData(), (int)stateData.getSize())) {
            DBG(xml->toString());
        }
        else {
            DBG("Failed to parse XML from state information.");
        }
    };
#endif

    addAndMakeVisible(syncMenu);
    syncMenu.setTooltip("Tempo sync");
    syncMenu.addSectionHeading("Tempo Sync");
    syncMenu.addItem("Rate Hz", 1);
    syncMenu.addSectionHeading("Straight");
    syncMenu.addItem("1/256", 2);
    syncMenu.addItem("1/128", 3);
    syncMenu.addItem("1/64", 4);
    syncMenu.addItem("1/32", 5);
    syncMenu.addItem("1/16", 6);
    syncMenu.addItem("1/8", 7);
    syncMenu.addItem("1/4", 8);
    syncMenu.addItem("1/2", 9);
    syncMenu.addItem("1 Bar", 10);
    syncMenu.addItem("2 Bars", 11);
    syncMenu.addItem("4 Bars", 12);
    syncMenu.addSectionHeading("Triplet");
    syncMenu.addItem("1/16T", 13);
    syncMenu.addItem("1/8T", 14);
    syncMenu.addItem("1/4T", 15);
    syncMenu.addItem("1/2T", 16);
    syncMenu.addItem("1/1T", 17);
    syncMenu.addSectionHeading("Dotted");
    syncMenu.addItem("1/16.", 18);
    syncMenu.addItem("1/8.", 19);
    syncMenu.addItem("1/4.", 20);
    syncMenu.addItem("1/2.", 21);
    syncMenu.addItem("1/1.", 22);
    syncMenu.setBounds(col, row, 90, 25);
    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.params, "sync", syncMenu);
    col += 100;

    rateDial = std::make_unique<TextDial>(p, "rate", "", "", TextDialLabel::tdRateHz, 16.f, 0xffffffff);
    addAndMakeVisible(*rateDial);
    rateDial->setBounds(col, row, 50, 25);
    col += 60;

    addAndMakeVisible(triggerLabel);
    triggerLabel.setColour(juce::Label::ColourIds::textColourId, Colour(COLOR_NEUTRAL_LIGHT));
    triggerLabel.setFont(FontOptions(16.0f));
    triggerLabel.setJustificationType(Justification::centredRight);
    triggerLabel.setText("Trigger", NotificationType::dontSendNotification);
    triggerLabel.setBounds(col, row, 60, 25);
    col += 70;

    addAndMakeVisible(triggerMenu);
    triggerMenu.setTooltip("Envelope trigger:\nSync - song playback\nMIDI - midi notes\nAudio - audio input");
    triggerMenu.addSectionHeading("Trigger");
    triggerMenu.addItem("Sync", 1);
    triggerMenu.addItem("MIDI", 2);
    triggerMenu.addItem("Audio", 3);
    triggerMenu.setBounds(col, row, 75, 25);
    triggerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.params, "trigger", triggerMenu);
    col += 85;

    addAndMakeVisible(algoMenu);
    algoMenu.setTooltip("Algorithm used for transient detection");
    algoMenu.addItem("Simple", 1);
    algoMenu.addItem("Drums", 2);
    algoMenu.setBounds(col,row,75,25);
    algoMenu.setColour(ComboBox::arrowColourId, Colour(COLOR_AUDIO));
    algoMenu.setColour(ComboBox::textColourId, Colour(COLOR_AUDIO));
    algoMenu.setColour(ComboBox::outlineColourId, Colour(COLOR_AUDIO));
    algoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.params, "algo", algoMenu);
    col += 85;

    addAndMakeVisible(audioSettingsButton);
    audioSettingsButton.setBounds(col, row, 25, 25);
    audioSettingsButton.onClick = [this]() {
        audioProcessor.showAudioKnobs = !audioProcessor.showAudioKnobs;
        if (audioProcessor.showAudioKnobs) {
            audioProcessor.showEnvelopeKnobs = false;
        }
        toggleUIComponents();
    };
    audioSettingsButton.setAlpha(0.0f);
    col += 35;

    // FIRST ROW RIGHT

    col = getWidth() - PLUG_PADDING;
    settingsButton = std::make_unique<SettingsButton>(p);
    addAndMakeVisible(*settingsButton);
    settingsButton->onScaleChange = [this]() { setScaleFactor(audioProcessor.scale); };
    settingsButton->toggleUIComponents = [this]() { toggleUIComponents(); };
    settingsButton->toggleAbout = [this]() { about.get()->setVisible(true); };
    settingsButton->setBounds(col-20,row,25,25);

    col -= 25;

    // SECOND ROW

    row += 35;
    col = PLUG_PADDING;
    for (int i = 0; i < 12; ++i) {
        auto btn = std::make_unique<TextButton>(std::to_string(i + 1));
        btn->setRadioGroupId (1337);
        btn->setToggleState(audioProcessor.pattern->index == i, dontSendNotification);
        btn->setClickingTogglesState (false);
        btn->setColour (TextButton::textColourOffId,  Colour(COLOR_BG));
        btn->setColour (TextButton::textColourOnId,   Colour(COLOR_BG));
        btn->setColour (TextButton::buttonColourId,   Colour(COLOR_ACTIVE).darker(0.8f));
        btn->setColour (TextButton::buttonOnColourId, Colour(COLOR_ACTIVE));
        btn->setBounds (col + i * 22, row, 22+1, 25); // width +1 makes it seamless on higher DPI
        btn->setConnectedEdges (((i != 0) ? Button::ConnectedOnLeft : 0) | ((i != 11) ? Button::ConnectedOnRight : 0));
        btn->setComponentID(i == 0 ? "leftPattern" : i == 11 ? "rightPattern" : "pattern");
        btn->onClick = [i, this]() {
            audioProcessor.queuePattern(i + 1);
        };
        addAndMakeVisible(*btn);
        patterns.push_back(std::move(btn));
    }
    col += 265 + 10;

    addAndMakeVisible(patSyncLabel);
    patSyncLabel.setColour(juce::Label::ColourIds::textColourId, Colour(COLOR_NEUTRAL_LIGHT));
    patSyncLabel.setFont(FontOptions(16.0f));
    patSyncLabel.setText("Pat. Sync", NotificationType::dontSendNotification);
    patSyncLabel.setJustificationType(Justification::centredLeft);
    patSyncLabel.setBounds(col, row, 70, 25);
    col += 80;

    addAndMakeVisible(patSyncMenu);
    patSyncMenu.setTooltip("Changes pattern in sync with song position during playback");
    patSyncMenu.addSectionHeading("Pattern Sync");
    patSyncMenu.addItem("Off", 1);
    patSyncMenu.addItem("1/4 Beat", 2);
    patSyncMenu.addItem("1/2 Beat", 3);
    patSyncMenu.addItem("1 Beat", 4);
    patSyncMenu.addItem("2 Beats", 5);
    patSyncMenu.addItem("4 Beats", 6);
    patSyncMenu.setBounds(col, row, 75, 25);
    patSyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.params, "patsync", patSyncMenu);

    // SECOND ROW RIGHT
    col = getWidth() - PLUG_PADDING;
    addAndMakeVisible(revEnvButton);
    revEnvButton.setButtonText("Envelope");
    revEnvButton.setComponentID("button");
    revEnvButton.setColour(TextButton::buttonColourId, Colours::white);
    revEnvButton.setColour(TextButton::buttonOnColourId, Colours::white);
    revEnvButton.setColour(TextButton::textColourOnId, Colour(COLOR_BG));
    revEnvButton.setColour(TextButton::textColourOffId, Colours::white);
    revEnvButton.setBounds(col-90, row, 90, 25);
    revEnvButton.onClick = [this]() {
        audioProcessor.showEnvelopeKnobs = !audioProcessor.showEnvelopeKnobs;
        if (audioProcessor.showEnvelopeKnobs && audioProcessor.showAudioKnobs) {
            audioProcessor.showAudioKnobs = false;
        }
        toggleUIComponents();
    };

    addAndMakeVisible(sendEnvButton);
    sendEnvButton.setButtonText("Envelope");
    sendEnvButton.setComponentID("button");
    sendEnvButton.setBounds(col-90, row, 90, 25);
    sendEnvButton.onClick = [this]() {
        audioProcessor.showEnvelopeKnobs = !audioProcessor.showEnvelopeKnobs;
        if (audioProcessor.showEnvelopeKnobs && audioProcessor.showAudioKnobs) {
            audioProcessor.showAudioKnobs = false;
        }
        toggleUIComponents();
    };
    col -= 100;

    addAndMakeVisible(revEnvOnButton);
    revEnvOnButton.setBounds(col-25, row, 25, 25);
    revEnvOnButton.setAlpha(0.f);
    revEnvOnButton.onClick = [this]() {
        MessageManager::callAsync([this] {
            bool on = (bool)audioProcessor.params.getRawParameterValue("revenvon")->load();
            audioProcessor.params.getParameter("revenvon")->setValueNotifyingHost(on ? 0.f : 1.f);
            toggleUIComponents();
            });
    };

    addAndMakeVisible(sendEnvOnButton);
    sendEnvOnButton.setBounds(col-25, row, 25, 25);
    sendEnvOnButton.setAlpha(0.f);
    sendEnvOnButton.onClick = [this]() {
        MessageManager::callAsync([this] {
            bool on = (bool)audioProcessor.params.getRawParameterValue("sendenvon")->load();
            audioProcessor.params.getParameter("sendenvon")->setValueNotifyingHost(on ? 0.f : 1.f);
            toggleUIComponents();
            });
    };

    // KNOBS ROW

    row += 35;
    col = PLUG_PADDING;

    irDisplay = std::make_unique<IRDisplay>(p);
    addAndMakeVisible(*irDisplay);
    irDisplay->setBounds(col-5,row,75*2+10+5+5,(int)(65*1.5));

    col += 75*2+10;
    lowcut = std::make_unique<Rotary>(p, "irlowcut", "Lowcut", RotaryLabel::hzHp);
    addAndMakeVisible(*lowcut);
    lowcut->setBounds(col,row,80,65);

    col += 75;

    highcut = std::make_unique<Rotary>(p, "irhighcut", "Highcut", RotaryLabel::hzLp);
    addAndMakeVisible(*highcut);
    highcut->setBounds(col,row,80,65);

    col += 75;

    stretch = std::make_unique<Rotary>(p, "irstretch", "Stretch", RotaryLabel::exp2Range, true);
    addAndMakeVisible(*stretch);
    stretch->setBounds(col,row,80,65);
    col += 75;

    width = std::make_unique<Rotary>(p, "width", "Width", RotaryLabel::percx100, true);
    addAndMakeVisible(*width);
    width->setBounds(col,row,80,65);
    col += 75;

    predelay = std::make_unique<Rotary>(p, "predelay", "Delay", RotaryLabel::kMillis);
    addAndMakeVisible(*predelay);
    predelay->setBounds(col,row,80,65);

    predelaysync = std::make_unique<Rotary>(p, "predelaysync", "Delay", RotaryLabel::kChoice);
    addAndMakeVisible(*predelaysync);
    predelaysync->setBounds(col,row,80,65);

    col += 75;

    drywet = std::make_unique<Rotary>(p, "drywet", "Dry/Wet", RotaryLabel::dryWet, true);
    addAndMakeVisible(*drywet);
    drywet->setBounds(col,row,80,65);
    col += 75;

    addAndMakeVisible(lowcutSlope);
    lowcutSlope.setBounds(lowcut->getBounds().getRight()-15, lowcut->getBounds().getY(), 25,20);
    lowcutSlope.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
    lowcutSlope.setAlpha(0.0f);
    lowcutSlope.onClick = [this]() {
        MessageManager::callAsync([this] {
            int slope = (int)audioProcessor.params.getRawParameterValue("irlowcutslope")->load();
            slope = (slope + 1) % 3;
            auto param = audioProcessor.params.getParameter("irlowcutslope");
            param->setValueNotifyingHost(param->convertTo0to1((float)slope));
            toggleUIComponents();
        });
    };

    addAndMakeVisible(highcutSlope);
    highcutSlope.setBounds(highcut->getBounds().getRight()-15, highcut->getBounds().getY(), 25,20);
    highcutSlope.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
    highcutSlope.setAlpha(0.0f);
    highcutSlope.onClick = [this]() {
        MessageManager::callAsync([this] {
            int slope = (int)audioProcessor.params.getRawParameterValue("irhighcutslope")->load();
            slope = (slope + 1) % 3;
            auto param = audioProcessor.params.getParameter("irhighcutslope");
            param->setValueNotifyingHost(param->convertTo0to1((float)slope));
            toggleUIComponents();
        });
    };

    // KNOBS 2ND ROW
    row += 75;
    col = PLUG_PADDING;

    addAndMakeVisible(currentFile);
    currentFile.setBounds(col, row + 42, 75*2+10, 25);
    currentFile.setAlpha(0.f);
    currentFile.onClick = [this] {
        audioProcessor.showFileSelector = !audioProcessor.showFileSelector;
        toggleUIComponents();
    };

    addAndMakeVisible(fileInfo);
    fileInfo.setFont(FontOptions(11.f));
    fileInfo.setJustificationType(Justification::centred);
    fileInfo.setColour(Label::ColourIds::textColourId, Colour(COLOR_NEUTRAL_LIGHT));
    fileInfo.setText("1 File, 44.1k->88k, 2.3s, 4ch", dontSendNotification);
    fileInfo.setBounds(currentFile.getBounds().translated(0, -16).withHeight(16));

    col = PLUG_PADDING + 75*2+10;

    send = std::make_unique<Rotary>(p, "send", "Send", RotaryLabel::percx100, false, COLOR_ACTIVE, ResKnob);
    addAndMakeVisible(*send);
    send->setBounds(col,row,80,65);
    col += 75;

    reverb = std::make_unique<Rotary>(p, "reverb", "Reverb", RotaryLabel::percx100, false, COLOR_ACTIVE, CutoffKnob);
    addAndMakeVisible(*reverb);
    reverb->setBounds(col,row,80,65);
    col += 75;

    revoffset = std::make_unique<Rotary>(p, "revoffset", "Offset", RotaryLabel::percx100, true);
    addAndMakeVisible(*revoffset);
    revoffset->setTooltip("Automate this param instead of Reverb");
    revoffset->setBounds(col,row,80,65);

    sendoffset = std::make_unique<Rotary>(p, "sendoffset", "Offset", RotaryLabel::percx100, true);
    addAndMakeVisible(*sendoffset);
    sendoffset->setTooltip("Automate this param instead of Send");
    sendoffset->setBounds(col,row,80,65);
    col += 75;

    smooth = std::make_unique<Rotary>(p, "smooth", "Smooth", RotaryLabel::percx100);
    addAndMakeVisible(*smooth);
    smooth->setBounds(col,row,80,65);
    col += 75;

    attack = std::make_unique<Rotary>(p, "attack", "Attack", RotaryLabel::percx100);
    addAndMakeVisible(*attack);
    attack->setBounds(col,row,80,65);
    col += 75;

    release = std::make_unique<Rotary>(p, "release", "Release", RotaryLabel::percx100);
    addAndMakeVisible(*release);
    release->setBounds(col,row,80,65);
    col += 75;

    tension = std::make_unique<Rotary>(p, "tension", "Tension", RotaryLabel::percx100, true);
    addAndMakeVisible(*tension);
    tension->setBounds(col,row,80,65);

    tensionatk = std::make_unique<Rotary>(p, "tensionatk", "TAtk", RotaryLabel::percx100, true);
    addAndMakeVisible(*tensionatk);
    tensionatk->setBounds(col,row,80,65);
    col += 75;

    tensionrel = std::make_unique<Rotary>(p, "tensionrel", "TRel", RotaryLabel::percx100, true);
    addAndMakeVisible(*tensionrel);
    tensionrel->setBounds(col,row,80,65);
    col += 75;

    addAndMakeVisible(predelayUseSync);
    predelayUseSync.setBounds(predelay->getBounds().getRight()-15, predelay->getBounds().getY(), 25,20);
    predelayUseSync.setAlpha(0.0f);
    predelayUseSync.onClick = [this]() {
        MessageManager::callAsync([this] {
            int useSync = (bool)audioProcessor.params.getRawParameterValue("predelayusesync")->load();
            auto param = audioProcessor.params.getParameter("predelayusesync");
            param->setValueNotifyingHost(param->convertTo0to1((float)!useSync));
            toggleUIComponents();
        });
    };

    // AUDIO WIDGET
    audioWidget = std::make_unique<AudioWidget>(p);
    addAndMakeVisible(*audioWidget);
    audioWidget->setBounds(PLUG_PADDING+75*2+10, row-75, PLUG_WIDTH - PLUG_PADDING*2 - 75*2 - 10, 75*2);

    // ENVELOPE WIDGETS
    auto b = Rectangle<int>(PLUG_PADDING +75*4-15, row-75, PLUG_WIDTH - PLUG_PADDING*2 - 75*4+15, 75*2-5);
    revenv = std::make_unique<EnvelopeWidget>(p, false, b.getWidth());
    addAndMakeVisible(*revenv);
    revenv->setBounds(b.expanded(0,4));

    sendenv = std::make_unique<EnvelopeWidget>(p, true, b.getWidth());
    addAndMakeVisible(*sendenv);
    sendenv->setBounds(b.expanded(0,5));

    // 3RD ROW
    col = PLUG_PADDING;
    row += 80;

    addAndMakeVisible(paintButton);
    paintButton.setButtonText("Paint");
    paintButton.setComponentID("button");
    paintButton.setBounds(col, row, 75, 25);
    paintButton.onClick = [this]() {
        if (audioProcessor.uimode == UIMode::PaintEdit && audioProcessor.luimode == UIMode::Paint) {
            audioProcessor.setUIMode(UIMode::Normal);
        }
        else {
            audioProcessor.togglePaintMode();
        }
    };
    col += 85;

    addAndMakeVisible(sequencerButton);
    sequencerButton.setButtonText("Seq");
    sequencerButton.setComponentID("button");
    sequencerButton.setBounds(col, row, 75, 25);
    sequencerButton.onClick = [this]() {
        if (audioProcessor.uimode == UIMode::PaintEdit && audioProcessor.luimode == UIMode::Seq) {
            audioProcessor.setUIMode(UIMode::Normal);
        }
        else {
            audioProcessor.toggleSequencerMode();
        }
    };

    col += 85;
    addAndMakeVisible(pointLabel);
    pointLabel.setText("p", dontSendNotification);
    pointLabel.setBounds(col-2,row,25,25);
    pointLabel.setVisible(false);
    col += 35-4;

    addAndMakeVisible(pointMenu);
    pointMenu.setTooltip("Point mode\nRight click points to change mode");
    pointMenu.addSectionHeading("Point Mode");
    pointMenu.addItem("Hold", 1);
    pointMenu.addItem("Curve", 2);
    pointMenu.addItem("S-Curve", 3);
    pointMenu.addItem("Pulse", 4);
    pointMenu.addItem("Wave", 5);
    pointMenu.addItem("Triangle", 6);
    pointMenu.addItem("Stairs", 7);
    pointMenu.addItem("Smooth St", 8);
    pointMenu.addItem("Half Sine", 9);
    pointMenu.setBounds(col, row, 75, 25);
    pointMenu.setSelectedId(audioProcessor.pointMode + 1, dontSendNotification);
    pointMenu.onChange = [this]() {
        MessageManager::callAsync([this]() {
            audioProcessor.pointMode = pointMenu.getSelectedId() - 1;
        });
    };
    col += 85;

    addAndMakeVisible(loopButton);
    loopButton.setTooltip("Toggle continuous play");
    loopButton.setColour(TextButton::buttonColourId, Colours::transparentWhite);
    loopButton.setColour(ComboBox::outlineColourId, Colours::transparentWhite);
    loopButton.setBounds(col, row, 25, 25);
    loopButton.onClick = [this]() {
        MessageManager::callAsync([this] {
            audioProcessor.alwaysPlaying = !audioProcessor.alwaysPlaying;
            repaint();
        });
    };
    col += 35;

    // 4TH ROW RIGHT
    col = getWidth() - PLUG_PADDING - 60;

    addAndMakeVisible(snapButton);
    snapButton.setTooltip("Toggle snap by using ctrl key");
    snapButton.setButtonText("Snap");
    snapButton.setComponentID("button");
    snapButton.setBounds(col, row, 60, 25);
    snapButton.setClickingTogglesState(true);
    snapAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.params, "snap", snapButton);

    col -= 60;
    gridSelector = std::make_unique<GridSelector>(p);
    gridSelector.get()->setTooltip("Grid size can also be set using mouse wheel on view");
    addAndMakeVisible(*gridSelector);
    gridSelector->setBounds(col,row,50,25);

    col -= 10+20+5;
    addAndMakeVisible(nudgeRightButton);
    nudgeRightButton.setAlpha(0.f);
    nudgeRightButton.setBounds(col, row, 20, 25);
    nudgeRightButton.onClick = [this]() {
        MessageManager::callAsync([this] {
            if (audioProcessor.uimode == UIMode::Seq) {
                audioProcessor.sequencer->rotateRight();
                return;
            }
            double grid = (double)audioProcessor.getCurrentGrid();
            auto snapshot = audioProcessor.viewPattern->points;
            audioProcessor.viewPattern->rotate(1.0/grid);
            audioProcessor.viewPattern->buildSegments();
            audioProcessor.createUndoPointFromSnapshot(snapshot);
        });
    };

    col -= 10+20-5;
    addAndMakeVisible(nudgeLeftButton);
    nudgeLeftButton.setAlpha(0.f);
    nudgeLeftButton.setBounds(col, row, 20, 25);
    nudgeLeftButton.onClick = [this]() {
        MessageManager::callAsync([this] {
            if (audioProcessor.uimode == UIMode::Seq) {
                audioProcessor.sequencer->rotateLeft();
                return;
            }
            double grid = (double)audioProcessor.getCurrentGrid();
            auto snapshot = audioProcessor.viewPattern->points;
            audioProcessor.viewPattern->rotate(-1.0/grid);
            audioProcessor.viewPattern->buildSegments();
            audioProcessor.createUndoPointFromSnapshot(snapshot);
        });
    };

    col -= 30;
    addAndMakeVisible(redoButton);
    redoButton.setButtonText("redo");
    redoButton.setComponentID("button");
    redoButton.setBounds(col, row, 20, 25);
    redoButton.setAlpha(0.f);
    redoButton.onClick = [this]() {
        MessageManager::callAsync([this] {
            if (audioProcessor.uimode == UIMode::Seq) {
                audioProcessor.sequencer->redo();
            }
            else {
                audioProcessor.viewPattern->redo();
                audioProcessor.updateSendFromPattern();
                audioProcessor.updateReverbFromPattern();
            }
            repaint();
        });
    };

    col -= 35;
    addAndMakeVisible(undoButton);
    undoButton.setButtonText("undo");
    undoButton.setComponentID("button");
    undoButton.setBounds(col, row, 20, 25);
    undoButton.setAlpha(0.f);
    undoButton.onClick = [this]() {
        MessageManager::callAsync([this] {
            if (audioProcessor.uimode == UIMode::Seq) {
                audioProcessor.sequencer->undo();
            }
            else {
                audioProcessor.viewPattern->undo();
                audioProcessor.updateSendFromPattern();
                audioProcessor.updateReverbFromPattern();
            }
            repaint();
        });
    };
    row += 35;

    paintWidget = std::make_unique<PaintToolWidget>(p);
    addAndMakeVisible(*paintWidget);
    paintWidget->setBounds(PLUG_PADDING,row,PLUG_WIDTH - PLUG_PADDING * 2, 40);

    row += 50;
    col = PLUG_PADDING;
    seqWidget = std::make_unique<SequencerWidget>(p);
    addAndMakeVisible(*seqWidget);
    seqWidget->setBounds(col,row,PLUG_WIDTH - PLUG_PADDING*2, 25*2+10);

    // VIEW
    col = 0;
    row += 50;
    view = std::make_unique<View>(p);
    addAndMakeVisible(*view);
    view->setBounds(col,row,getWidth(), getHeight() - row);

    fileSelector = std::make_unique<FileSelector>(p, [this] {
        audioProcessor.showFileSelector = !audioProcessor.showFileSelector;
        toggleUIComponents();
    });
    addAndMakeVisible(*fileSelector);
    fileSelector->setBounds(view->getBounds().withTrimmedTop(PLUG_PADDING));

    addAndMakeVisible(latencyWarning);
    latencyWarning.setText("Plugin latency has changed, restart playback", dontSendNotification);
    latencyWarning.setColour(Label::backgroundColourId, Colours::black.withAlpha(0.5f));
    latencyWarning.setJustificationType(Justification::centred);
    latencyWarning.setColour(Label::textColourId, Colour(COLOR_ACTIVE));
    latencyWarning.setBounds(view->getBounds().getCentreX() - 200, PLUG_HEIGHT - 20 - 25, 300, 25);

    // ABOUT
    about = std::make_unique<About>();
    addAndMakeVisible(*about);
    about->setBounds(getBounds());
    about->setVisible(false);

    customLookAndFeel = new CustomLookAndFeel();
    setLookAndFeel(customLookAndFeel);

    init = true;
    resized();
    toggleUIComponents();
}

REEVRAudioProcessorEditor::~REEVRAudioProcessorEditor()
{
    audioProcessor.saveSettings(); // save paint patterns to disk
    setLookAndFeel(nullptr);
    delete customLookAndFeel;
    audioProcessor.params.removeParameterListener("sync", this);
    audioProcessor.params.removeParameterListener("trigger", this);
    audioProcessor.params.removeParameterListener("revenvon", this);
    audioProcessor.params.removeParameterListener("sendenvon", this);
    audioProcessor.params.removeParameterListener("irlowcutslope", this);
    audioProcessor.params.removeParameterListener("irhighcutslope", this);
    audioProcessor.params.removeParameterListener("predelayusesync", this);
    audioProcessor.removeChangeListener(this);
}

void REEVRAudioProcessorEditor::changeListenerCallback(ChangeBroadcaster* source)
{
    (void)source;

    MessageManager::callAsync([this] { toggleUIComponents(); });
}

void REEVRAudioProcessorEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    (void)parameterID;
    (void)newValue;
    MessageManager::callAsync([this]() { toggleUIComponents(); });
};

void REEVRAudioProcessorEditor::toggleUIComponents()
{
    patterns[audioProcessor.pattern->index].get()->setToggleState(true, dontSendNotification);
    bool isSendMode = audioProcessor.sendEditMode;
    revoffset->setVisible(!isSendMode);
    sendoffset->setVisible(isSendMode);

    bool useSync = (bool)audioProcessor.params.getRawParameterValue("predelayusesync")->load();
    predelay->setVisible(!useSync);
    predelaysync->setVisible(useSync);

    auto trigger = (int)audioProcessor.params.getRawParameterValue("trigger")->load();
    auto triggerColor = trigger == 0 ? COLOR_ACTIVE : trigger == 1 ? COLOR_MIDI : COLOR_AUDIO;
    triggerMenu.setColour(ComboBox::arrowColourId, Colour(triggerColor));
    triggerMenu.setColour(ComboBox::textColourId, Colour(triggerColor));
    triggerMenu.setColour(ComboBox::outlineColourId, Colour(triggerColor));
    algoMenu.setVisible(trigger == Trigger::Audio);
    audioSettingsButton.setVisible(trigger == Trigger::Audio);
    if (!audioSettingsButton.isVisible()) {
        audioProcessor.showAudioKnobs = false;
    }
    loopButton.setVisible(trigger > 0);

    auto sync = (int)audioProcessor.params.getRawParameterValue("sync")->load();
    rateDial->setVisible(sync == 0);

    triggerLabel.setBounds(triggerLabel.getBounds().withX(rateDial->isVisible()
        ? rateDial->getBounds().getRight() + 5
        : syncMenu.getBounds().getRight() + 10
    ));
    triggerMenu.setBounds(triggerMenu.getBounds().withX(triggerLabel.getBounds().getRight() + 10));
    algoMenu.setBounds(algoMenu.getBounds().withX(triggerMenu.getBounds().getRight() + 10));
    audioSettingsButton.setBounds(audioSettingsButton.getBounds().withX(algoMenu.getBounds().getRight() + 10));

    bool showAudioKnobs = audioProcessor.showAudioKnobs;

    // layout knobs
    tension->setVisible(!audioProcessor.dualTension);
    tensionatk->setVisible(audioProcessor.dualTension);
    tensionrel->setVisible(audioProcessor.dualTension);

    {
        auto col = revoffset->getBounds().getX();
        auto row = revoffset->getBounds().getY();
        col += 75;

        if (audioProcessor.dualSmooth) {
            smooth->setVisible(false);
            attack->setVisible(true);
            release->setVisible(true);
            attack->setTopLeftPosition(col, row);
            col += 75;
            release->setTopLeftPosition(col, row);
            col+= 75;
        }
        else {
            smooth->setVisible(true);
            attack->setVisible(false);
            release->setVisible(false);
            smooth->setTopLeftPosition(col, row);
            col += 75;
        }
        tension->setTopLeftPosition(col, row);
        tensionatk->setTopLeftPosition(col, row);
        col += 75;
        tensionrel->setTopLeftPosition(col, row);
        if (audioProcessor.dualTension) col += 75;
    }

    audioWidget->setVisible(showAudioKnobs);
    audioWidget->toggleUIComponents();

    latencyWarning.setVisible(audioProcessor.showLatencyWarning);

    paintWidget->setVisible(audioProcessor.showPaintWidget);
    seqWidget->setVisible(audioProcessor.showSequencer);
    seqWidget->setBounds(seqWidget->getBounds().withY(paintWidget->isVisible()
        ? paintWidget->getBounds().getBottom() + 10
        : paintWidget->getBounds().getY()
    ).withWidth(getWidth() - PLUG_PADDING * 2));

    if (seqWidget->isVisible()) {
        view->setBounds(view->getBounds().withTop(seqWidget->getBottom()));
    }
    else if (paintWidget->isVisible()) {
        view->setBounds(view->getBounds().withTop(paintWidget->getBounds().getBottom()));
    }
    else {
        view->setBounds(view->getBounds().withTop(paintWidget->getBounds().getY() - 10));
    }
    fileSelector->setBounds(view->getBounds().withTrimmedTop(PLUG_PADDING));

    auto uimode = audioProcessor.uimode;
    paintButton.setToggleState(uimode == UIMode::Paint || (uimode == UIMode::PaintEdit && audioProcessor.luimode == UIMode::Paint), dontSendNotification);
    sequencerButton.setToggleState(audioProcessor.sequencer->isOpen, dontSendNotification);
    paintWidget->toggleUIComponents();

    revEnvButton.setVisible(!isSendMode);
    revEnvButton.setToggleState(audioProcessor.showEnvelopeKnobs, dontSendNotification);
    revEnvOnButton.setVisible(!isSendMode);
    sendEnvButton.setVisible(isSendMode);
    sendEnvButton.setToggleState(audioProcessor.showEnvelopeKnobs, dontSendNotification);
    sendEnvOnButton.setVisible(isSendMode);

    revenv->setVisible(!isSendMode && audioProcessor.showEnvelopeKnobs);
    sendenv->setVisible(isSendMode && audioProcessor.showEnvelopeKnobs);

    revenv->layoutComponents();
    sendenv->layoutComponents();

    fileSelector->setVisible(audioProcessor.showFileSelector);

    auto formatNumber = [](double value)
        {
            double rounded = std::round(value * 10.0) / 10.0;  // one-decimal rounding
            bool isWhole = (std::fmod(rounded, 1.0) == 0.0);

            return isWhole ? String(rounded, 0)
                : String(rounded, 1);
        };

    String text = "";
    auto nfiles = audioProcessor.impulse->nfiles;
    if (nfiles > 1)
        text += String(nfiles) + " files";
    auto duration = (double)audioProcessor.impulse->rawBufferLL.size() / audioProcessor.impulse->srate;
    text += (nfiles > 1 ? String(", ") : "") + formatNumber(duration) + "s";
    auto irsrate = audioProcessor.impulse->irsrate;
    auto srate = audioProcessor.impulse->srate;
    text += String(", ") + formatNumber(irsrate / 1000.0) + "k";
    if (irsrate != srate) {
        text += String("->") + formatNumber(srate / 1000.0) + "k";
    }
    text += String(", ") + String(audioProcessor.impulse->numChans) + "ch";
    fileInfo.setText(text, dontSendNotification);

    MessageManager::callAsync([this] {
        repaint();
    });
}

//==============================================================================

void REEVRAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll(Colour(COLOR_BG));
    auto bounds = getLocalBounds().withTop(view->getBounds().getY() + 10).withHeight(3).toFloat();
    if (audioProcessor.uimode == UIMode::Seq)
        bounds = bounds.withY((float)seqWidget->getBounds().getBottom() + 10);

    auto grad = ColourGradient(
        Colours::black.withAlpha(0.25f),
        bounds.getTopLeft(),
        Colours::transparentBlack,
        bounds.getBottomLeft(),
        false
    );
    g.setGradientFill(grad);
    g.fillRect(bounds);

    // draw point mode icon
    g.setColour(Colour(COLOR_NEUTRAL));
    g.drawEllipse(pointLabel.getBounds().expanded(-2,-2).toFloat(), 1.f);
    g.fillEllipse(pointLabel.getBounds().expanded(-10,-10).toFloat());

    bounds = audioProcessor.sendEditMode ? send->getBounds().toFloat() : reverb->getBounds().toFloat();
    bounds.removeFromTop(50.f);
    g.setColour((audioProcessor.sendEditMode ? Colour(COLOR_ACTIVE) : Colours::white).withAlpha(0.3f));
    g.fillRoundedRectangle(bounds.toFloat().expanded(-8.f, 2.f).translated(0.5f, 0.5f), 3.f);

    // draw loop play button
    auto trigger = (int)audioProcessor.params.getRawParameterValue("trigger")->load();
    if (trigger != Trigger::Sync) {
        if (audioProcessor.alwaysPlaying) {
            g.setColour(Colours::yellow);
            auto loopBounds = loopButton.getBounds().expanded(-5);
            g.fillRect(loopBounds.removeFromLeft(5));
            loopBounds = loopButton.getBounds().expanded(-5);
            g.fillRect(loopBounds.removeFromRight(5));
        }
        else {
            g.setColour(Colour(0xff00ff00));
            juce::Path triangle;
            auto loopBounds = loopButton.getBounds().expanded(-5);
            triangle.startNewSubPath(0.0f, 0.0f);
            triangle.lineTo(0.0f, (float)loopBounds.getHeight());
            triangle.lineTo((float)loopBounds.getWidth(), loopBounds.getHeight() / 2.f);
            triangle.closeSubPath();
            g.fillPath(triangle, AffineTransform::translation((float)loopBounds.getX(), (float)loopBounds.getY()));
        }
    }

    // draw audio settings button outline
    if (audioSettingsButton.isVisible() && audioProcessor.showAudioKnobs) {
        g.setColour(Colour(COLOR_AUDIO));
        g.fillRoundedRectangle(audioSettingsButton.getBounds().toFloat(), 3.0f);
        drawGear(g, audioSettingsButton.getBounds(), 10, 6, Colour(COLOR_BG), Colour(COLOR_AUDIO));
    }
    else if (audioSettingsButton.isVisible()) {
        drawGear(g, audioSettingsButton.getBounds(), 10, 6, Colour(COLOR_AUDIO), Colour(COLOR_BG));
    }

    // draw rotate pat triangles
    g.setColour(Colour(COLOR_ACTIVE));
    auto triCenter = nudgeLeftButton.getBounds().toFloat().getCentre();
    auto triRadius = 5.f;
    juce::Path nudgeLeftTriangle;
    nudgeLeftTriangle.addTriangle(
        triCenter.translated(-triRadius, 0),
        triCenter.translated(triRadius, -triRadius),
        triCenter.translated(triRadius, triRadius)
    );
    g.fillPath(nudgeLeftTriangle);

    triCenter = nudgeRightButton.getBounds().toFloat().getCentre();
    juce::Path nudgeRightTriangle;
    nudgeRightTriangle.addTriangle(
        triCenter.translated(-triRadius, -triRadius),
        triCenter.translated(-triRadius, triRadius),
        triCenter.translated(triRadius, 0)
    );
    g.fillPath(nudgeRightTriangle);

    // draw undo redo buttons
    auto canUndo = audioProcessor.uimode == UIMode::Seq
        ? !audioProcessor.sequencer->undoStack.empty()
        : !audioProcessor.viewPattern->undoStack.empty();

    auto canRedo = audioProcessor.uimode == UIMode::Seq
        ? !audioProcessor.sequencer->redoStack.empty()
        : !audioProcessor.viewPattern->redoStack.empty();

    drawUndoButton(g, undoButton.getBounds().toFloat(), true, Colour(canUndo ? COLOR_ACTIVE : COLOR_NEUTRAL));
    drawUndoButton(g, redoButton.getBounds().toFloat(), false, Colour(canRedo ? COLOR_ACTIVE : COLOR_NEUTRAL));

    // envelope draws
    bool isSendMode = audioProcessor.sendEditMode;
    bool isRevEnvOn = (bool)audioProcessor.params.getRawParameterValue("revenvon")->load();
    bool isSendEnvOn = (bool)audioProcessor.params.getRawParameterValue("sendenvon")->load();

    // draw envelope button extension
    if (audioProcessor.showEnvelopeKnobs) {
        g.setColour(Colour(isSendMode ? COLOR_ACTIVE : 0xffffffff));
        g.fillRect(revEnvButton.getBounds().expanded(0, 20).translated(0, 30));
    }

    if (!isSendMode) {
        g.setColour(Colours::white);
        bounds = revEnvOnButton.getBounds().toFloat().translated(0.5f, 0.5f);
        if (isRevEnvOn) {
            drawPowerButton(g, bounds, Colour(COLOR_ACTIVE));
        }
        else {
            drawPowerButton(g, bounds, Colour(COLOR_NEUTRAL));
        }
    }
    else {
        g.setColour(Colour(COLOR_ACTIVE));
        bounds = sendEnvOnButton.getBounds().toFloat().translated(0.5f, 0.5f);
        if (isSendEnvOn) {
            drawPowerButton(g, bounds, Colour(COLOR_ACTIVE));
        }
        else {
            drawPowerButton(g, bounds, Colour(COLOR_NEUTRAL));
        }
    }

    // draw slope buttons text
    g.setColour(Colour(COLOR_ACTIVE));
    g.setFont(FontOptions(10.0f));
    int lowcutslope = (int)audioProcessor.params.getRawParameterValue("irlowcutslope")->load();
    int highcutslope = (int)audioProcessor.params.getRawParameterValue("irhighcutslope")->load();
    String lowslopeLabel = lowcutslope == 0 ? "6dB" : lowcutslope == 1 ? "12dB" : "24dB";
    String highslopeLabel = highcutslope == 0 ? "6dB" : highcutslope == 1 ? "12dB" : "24dB";
    g.drawFittedText(lowslopeLabel, lowcutSlope.getBounds().translated(2,0), Justification::centredLeft, 1, 1.f);
    g.drawFittedText(highslopeLabel, highcutSlope.getBounds().translated(2,0), Justification::centredLeft, 1, 1.f);

    // draw predelay useSync
    bool useSync = (bool)audioProcessor.params.getRawParameterValue("predelayusesync")->load();
    g.setColour(Colour(useSync ? COLOR_ACTIVE : COLOR_NEUTRAL));
    bounds = predelayUseSync.getBounds().withTrimmedBottom(4).toFloat();
    auto r = 3.f;
    g.fillEllipse(bounds.getCentreX() - r*2, bounds.getBottom()-r*2, r*2, r*2);
    g.drawLine(bounds.getCentreX(), bounds.getBottom()-r, bounds.getCentreX(), bounds.getY()+r);

    bounds = currentFile.getBounds().toFloat();
    if (audioProcessor.showFileSelector) {
        g.setColour(Colour(COLOR_ACTIVE));
        g.fillRect(bounds);
    }
    g.setColour(Colour(audioProcessor.showFileSelector ? COLOR_BG : COLOR_ACTIVE));
    g.setFont(FontOptions(18.f));
    g.drawFittedText(audioProcessor.impulse->name, bounds.expanded(-3, 0).toNearestInt(), Justification::centred, 2, 1.f);
}

void REEVRAudioProcessorEditor::drawPowerButton(Graphics& g, Rectangle<float> bounds, Colour color)
{
    bounds.expand(-6,-6);
    auto pi = MathConstants<float>::pi;
    g.setColour(color);
    Path p;
    p.addArc(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 0.75f, 2.f * pi - 0.75f, true);
    p.startNewSubPath(bounds.getCentreX(), bounds.getY() - 2);
    p.lineTo(bounds.getCentreX(), bounds.getY() + 4);
    g.strokePath(p, PathStrokeType(2.f, PathStrokeType::curved, PathStrokeType::rounded));
}

void REEVRAudioProcessorEditor::drawGear(Graphics& g, Rectangle<int> bounds, float radius, int segs, Colour color, Colour background)
{
    float x = bounds.toFloat().getCentreX();
    float y = bounds.toFloat().getCentreY();
    float oradius = radius;
    float iradius = radius / 3.f;
    float cradius = iradius / 1.5f;
    float coffset = MathConstants<float>::twoPi;
    float inc = MathConstants<float>::twoPi / segs;

    g.setColour(color);
    g.fillEllipse(x-oradius,y-oradius,oradius*2.f,oradius*2.f);

    g.setColour(background);
    for (int i = 0; i < segs; i++) {
        float angle = coffset + i * inc;
        float cx = x + std::cos(angle) * oradius;
        float cy = y + std::sin(angle) * oradius;
        g.fillEllipse(cx - cradius, cy - cradius, cradius * 2, cradius * 2);
    }
    g.fillEllipse(x-iradius, y-iradius, iradius*2.f, iradius*2.f);
}
void REEVRAudioProcessorEditor::drawUndoButton(Graphics& g, juce::Rectangle<float> area, bool invertx, Colour color)
{
        auto bounds = area;
        auto thickness = 2.f;
        float left = bounds.getX();
        float right = bounds.getRight();
        float top = bounds.getCentreY() - 4;
        float bottom = bounds.getCentreY() + 4;
        float centerY = bounds.getCentreY();
        float shaftStart = right - 7;

        Path arrowPath;
        // arrow head
        arrowPath.startNewSubPath(right, centerY);
        arrowPath.lineTo(shaftStart, top);
        arrowPath.startNewSubPath(right, centerY);
        arrowPath.lineTo(shaftStart, bottom);

        // shaft
        float radius = (bottom - centerY);
        arrowPath.startNewSubPath(right, centerY);
        arrowPath.lineTo(left + radius - 1, centerY);

        // semi circle
        arrowPath.startNewSubPath(left + radius, centerY);
        arrowPath.addArc(left, centerY, radius, radius, 2.f * float_Pi, float_Pi);

        if (invertx) {
            AffineTransform flipTransform = AffineTransform::scale(-1.0f, 1.0f)
                .translated(bounds.getWidth(), 0);

            // First move the path to origin, apply transform, then move back
            arrowPath.applyTransform(AffineTransform::translation(-bounds.getPosition()));
            arrowPath.applyTransform(flipTransform);
            arrowPath.applyTransform(AffineTransform::translation(bounds.getPosition()));
        }

        g.setColour(color);
        g.strokePath(arrowPath, PathStrokeType(thickness));
}

void REEVRAudioProcessorEditor::resized()
{
    if (!init) return; // defer resized() call during constructor

    // layout right aligned components and view
    // first row
    auto col = getWidth() - PLUG_PADDING;
    auto bounds = settingsButton->getBounds();
    settingsButton->setBounds(bounds.withX(col - bounds.getWidth()));

    audioWidget->setBounds(audioWidget->getBounds().withRight(getWidth() - PLUG_PADDING));

    about->setBounds(0,0,getWidth(), getHeight());

    sendenv->setBounds(sendenv->getBounds().withRightX(getWidth() + 10));
    revenv->setBounds(sendenv->getBounds().withRightX(getWidth() + 10));

    // 3rd row
    sendEnvButton.setBounds(sendEnvButton.getBounds().withRightX(col));
    revEnvButton.setBounds(revEnvButton.getBounds().withRightX(col));
    revEnvOnButton.setBounds(revEnvOnButton.getBounds().withRightX(revEnvButton.getBounds().getX() - 10));
    sendEnvOnButton.setBounds(sendEnvOnButton.getBounds().withRightX(sendEnvButton.getBounds().getX() - 10));

    // 4th row
    bounds = snapButton.getBounds();
    auto dx = (col - bounds.getWidth()) - bounds.getX();
    snapButton.setBounds(snapButton.getBounds().translated(dx, 0));
    gridSelector->setBounds(gridSelector->getBounds().translated(dx, 0));
    nudgeLeftButton.setBounds(nudgeLeftButton.getBounds().translated(dx, 0));
    nudgeRightButton.setBounds(nudgeRightButton.getBounds().translated(dx, 0));
    redoButton.setBounds(redoButton.getBounds().translated(dx, 0));
    undoButton.setBounds(undoButton.getBounds().translated(dx, 0));

    // view
    bounds = view->getBounds();
    view->setBounds(bounds.withWidth(getWidth()).withHeight(getHeight() - bounds.getY()));

    fileSelector->setBounds(view->getBounds().withTrimmedTop(PLUG_PADDING));

    bounds = seqWidget->getBounds();
    seqWidget->setBounds(bounds.withWidth(getWidth() - PLUG_PADDING * 2));

    bounds = latencyWarning.getBounds();
    latencyWarning.setBounds(bounds
        .withX(view->getBounds().getCentreX() - bounds.getWidth() / 2)
        .withY(getHeight() - 20 - bounds.getHeight())
    );

    audioProcessor.plugWidth = getWidth();
    audioProcessor.plugHeight = getHeight();
}

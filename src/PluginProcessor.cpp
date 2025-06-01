 // Copyright 2025 tilr

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <ctime>

REVERAudioProcessor::REVERAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
         .withInput("Input", juce::AudioChannelSet::stereo(), true)
         .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)
         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
     )
    , settings{}
    , convolver(new StereoConvolver())
    , loadConvolver(new StereoConvolver())
    , impulse(new Impulse())
    , params(*this, &undoManager, "PARAMETERS", {
        std::make_unique<juce::AudioParameterInt>("pattern", "Pattern", 1, 12, 1),
        std::make_unique<juce::AudioParameterChoice>("patsync", "Pattern Sync", StringArray { "Off", "1/4 Beat", "1/2 Beat", "1 Beat", "2 Beats", "4 Beats"}, 0),
        std::make_unique<juce::AudioParameterChoice>("trigger", "Trigger", StringArray { "Sync", "MIDI", "Audio" }, 0),
        std::make_unique<juce::AudioParameterChoice>("sync", "Sync", StringArray { "Rate Hz", "1/16", "1/8", "1/4", "1/2", "1/1", "2/1", "4/1", "1/16t", "1/8t", "1/4t", "1/2t", "1/1t", "1/16.", "1/8.", "1/4.", "1/2.", "1/1." }, 5),
        std::make_unique<juce::AudioParameterFloat>("rate", "Rate Hz", juce::NormalisableRange<float>(0.01f, 5000.0f, 0.01f, 0.2f), 1.0f),
        std::make_unique<juce::AudioParameterFloat>("phase", "Phase", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("min", "Min", 0.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("max", "Max", 0.0f, 1.0f, 1.0f),
        std::make_unique<juce::AudioParameterFloat>("smooth", "Smooth", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("attack", "Attack", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("release", "Release", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("tension", "Tension", -1.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("tensionatk", "Attack Tension", -1.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("tensionrel", "Release Tension", -1.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterBool>("snap", "Snap", false),
        std::make_unique<juce::AudioParameterInt>("grid", "Grid", 0, (int)std::size(GRID_SIZES)-1, 2),
        std::make_unique<juce::AudioParameterInt>("seqstep", "Sequencer Step", 0, (int)std::size(GRID_SIZES)-1, 2),
        // reverb params
        std::make_unique<juce::AudioParameterFloat>("reverb", "Send Offset", juce::NormalisableRange<float> (0.0f, 1.0f), 1.0f),
        std::make_unique<juce::AudioParameterFloat>("send", "Send Offset", juce::NormalisableRange<float> (0.0f, 1.0f), 1.0f),
        std::make_unique<juce::AudioParameterFloat>("sendoffset", "Send Offset", juce::NormalisableRange<float> (-1.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("revoffset", "Reverb Offset", juce::NormalisableRange<float> (-1.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("predelay", "Pre-Delay", NormalisableRange<float>(0.0f, 1000.f, 1.0f, 0.5f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("width", "Width", 0.f, 2.f, 1.0f),
        std::make_unique<juce::AudioParameterFloat>("irattack", "IR Attack", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("irdecay", "IR Decay", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("irtrimleft", "IR Trim Left", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("irtrimright", "IR Trim Right", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("irstretch", "IR Stretch", juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("irlowcut", "IR LowCut", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.3f) , 20.f),
        std::make_unique<juce::AudioParameterFloat>("irhighcut", "IR HighCut", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.3f) , 20000.f),
        std::make_unique<juce::AudioParameterChoice>("irlowcutslope", "IR Lowcut Slope", StringArray { "6dB", "12dB", "24dB" }, 1),
        std::make_unique<juce::AudioParameterChoice>("irhighcutslope", "IR Lowcut Slope", StringArray { "6dB", "12dB", "24dB" }, 1),
        std::make_unique<juce::AudioParameterFloat>("drywet", "DryWet Mix", juce::NormalisableRange<float> (0.f, 1.0f), 0.25f),
        // audio trigger params
        std::make_unique<juce::AudioParameterChoice>("algo", "Audio Algorithm", StringArray { "Simple", "Drums" }, 0),
        std::make_unique<juce::AudioParameterFloat>("threshold", "Audio Threshold", NormalisableRange<float>(0.0f, 1.0f), 0.5f),
        std::make_unique<juce::AudioParameterFloat>("sense", "Audio Sensitivity", 0.0f, 1.0f, 0.5f),
        std::make_unique<juce::AudioParameterFloat>("lowcut", "Audio LowCut", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.3f) , 20.f),
        std::make_unique<juce::AudioParameterFloat>("highcut", "Audio HighCut", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.3f) , 20000.f),
        std::make_unique<juce::AudioParameterFloat>("offset", "Audio Offset", -1.0f, 1.0f, 0.0f),
        // envelope follower params
        std::make_unique<juce::AudioParameterBool>("sendenvon", "Send Env ON", false),
        std::make_unique<juce::AudioParameterFloat>("sendenvthresh", "Send Env Thresh", NormalisableRange<float>( 0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("sendenvamt", "Send Env Amount", NormalisableRange<float>( -5.0f, 5.0f, 0.01f, 0.5, true), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("sendenvatk", "Send Env Attack", NormalisableRange<float>( 0.f, 1.0f, 0.0001f, 0.75f), 0.f),
        std::make_unique<juce::AudioParameterFloat>("sendenvrel", "Send Env Release", NormalisableRange<float>( 0.f, 1.0f, 0.0001f, 0.5f), 0.05f),
        std::make_unique<juce::AudioParameterFloat>("sendenvhold", "Send Env Hold", NormalisableRange<float>( 0.f, 1.0f, 0.0001f, 0.75f), 0.f),
        std::make_unique<juce::AudioParameterFloat>("sendenvlowcut", "Send Env Lowcut", NormalisableRange<float>( 20.f, 20000.0f, 1.f, 0.3f), 20.f),
        std::make_unique<juce::AudioParameterFloat>("sendenvhighcut", "Send Env HighCut", NormalisableRange<float>( 20.f, 20000.0f, 1.f, 0.3f), 20000.f),
        std::make_unique<juce::AudioParameterBool>("revenvon", "Rev Env ON", false),
        std::make_unique<juce::AudioParameterFloat>("revenvthresh", "Rev Env Threvh", NormalisableRange<float>( 0.0f, 1.0f), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("revenvamt", "Rev Env Amount", NormalisableRange<float>( -5.0f, 5.0f, 0.01f, 0.5, true), 0.0f),
        std::make_unique<juce::AudioParameterFloat>("revenvatk", "Rev Env Attack", NormalisableRange<float>( 0.f, 1.0f, 0.0001f, 0.75f), 0.f),
        std::make_unique<juce::AudioParameterFloat>("revenvrel", "Rev Env Release", NormalisableRange<float>( 0.f, 1.0f, 0.0001f, 0.5f), 0.05f),
        std::make_unique<juce::AudioParameterFloat>("revenvhold", "Rev Env Hold", NormalisableRange<float>( 0.f, 1.0f, 0.0001f, 0.75f), 0.f),
        std::make_unique<juce::AudioParameterFloat>("revenvlowcut", "Rev Env LowCut", NormalisableRange<float>( 20.f, 20000.0f, 1.f, 0.3f), 20.f),
        std::make_unique<juce::AudioParameterFloat>("revenvhighcut", "Rev Env HighCut", NormalisableRange<float>( 20.f, 20000.0f, 1.f, 0.3f), 20000.f),
    })
#endif
{
    srand(static_cast<unsigned int>(time(nullptr))); // seed random generator
    juce::PropertiesFile::Options options{};
    options.applicationName = ProjectInfo::projectName;
    options.filenameSuffix = ".settings";
#if defined(JUCE_LINUX) || defined(JUCE_BSD)
    options.folderName = "~/.config/rever";
#endif
    options.osxLibrarySubFolder = "Application Support";
    options.storageFormat = PropertiesFile::storeAsXML;
    settings.setStorageParameters(options);

    for (auto* param : getParameters()) {
        param->addListener(this);
    }

    params.addParameterListener("pattern", this);

    // init patterns
    for (int i = 0; i < 12; ++i) {
        patterns[i] = new Pattern(i);
        patterns[i]->insertPoint(0.0, 0.0, 0, 1);
        patterns[i]->buildSegments();

        sendpatterns[i] = new Pattern(i+12);
        sendpatterns[i]->insertPoint(0.0, 0.0, 0, 1);
        sendpatterns[i]->buildSegments();
    }

    // init paintMode Patterns
    for (int i = 0; i < PAINT_PATS; ++i) {
        paintPatterns[i] = new Pattern(i + PAINT_PATS_IDX);
        if (i < 8) {
            auto preset = Presets::getPaintPreset(i);
            for (auto& point : preset) {
                paintPatterns[i]->insertPoint(point.x, point.y, point.tension, point.type);
            }
        }
        else {
            paintPatterns[i]->insertPoint(0.0, 1.0, 0.0, 1);
            paintPatterns[i]->insertPoint(1.0, 0.0, 0.0, 1);
        }
        paintPatterns[i]->buildSegments();
    }

    sequencer = new Sequencer(*this);
    pattern = patterns[0];
    sendpattern = sendpatterns[0];
    viewPattern = pattern;
    viewSubPattern = sendpattern;
    preSamples.resize(MAX_PLUG_WIDTH, 0); // samples array size must be >= viewport width
    postSamples.resize(MAX_PLUG_WIDTH, 0);
    monSamples.resize(MAX_PLUG_WIDTH, 0); // samples array size must be >= audio monitor width
    revvalue = new RCSmoother();
    sendvalue = new RCSmoother();

    updateReverbFromPattern();
    updateSendFromPattern();
    loadSettings();
}

REVERAudioProcessor::~REVERAudioProcessor()
{
    params.removeParameterListener("pattern", this);
}

void REVERAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "pattern") {
        int pat = (int)newValue;
        if (pat != pattern->index + 1 && pat != queuedPattern) {
            queuePattern(pat);
        }
    }
}

void REVERAudioProcessor::parameterValueChanged (int parameterIndex, float newValue)
{
    (void)newValue;
    (void)parameterIndex;
    paramChanged = true;
}

void REVERAudioProcessor::parameterGestureChanged (int parameterIndex, bool gestureIsStarting)
{
    (void)parameterIndex;
    (void)gestureIsStarting;
}

void REVERAudioProcessor::loadImpulse(String path)
{
    irFile = path;
    irDirty = true;
}

void REVERAudioProcessor::loadSettings ()
{
    settings.closeFiles(); // FIX files changed by other plugin instances not loading
    if (auto* file = settings.getUserSettings()) {
        scale = (float)file->getDoubleValue("scale", 1.0f);
        plugWidth = file->getIntValue("width", PLUG_WIDTH);
        plugHeight = file->getIntValue("height", PLUG_HEIGHT);
        if (!file->getValue("irdir", "").isEmpty()) {
            irDir = file->getValue("irdir");
        }
        else {
            File settingsFile = file->getFile();
            File settingsFolder = settingsFile.getParentDirectory();
            File impulsesDir = settingsFolder.getChildFile("impulses");
            if (!impulsesDir.exists()) {
                if (impulsesDir.createDirectory()) {
                    irDir = impulsesDir.getFullPathName();
                }
            } else {
                irDir = impulsesDir.getFullPathName();
            }
        }
        auto tensionparam = (double)params.getRawParameterValue("tension")->load();
        auto tensionatk = (double)params.getRawParameterValue("tensionatk")->load();
        auto tensionrel = (double)params.getRawParameterValue("tensionrel")->load();

        for (int i = 0; i < PAINT_PATS; ++i) {
            auto str = file->getValue("paintpat" + String(i),"").toStdString();
            if (!str.empty()) {
                paintPatterns[i]->clear();
                paintPatterns[i]->clearUndo();
                double x, y, tension;
                int type;
                std::istringstream iss(str);
                while (iss >> x >> y >> tension >> type) {
                    paintPatterns[i]->insertPoint(x,y,tension,type);
                }
                paintPatterns[i]->setTension(tensionparam, tensionatk, tensionrel, dualTension);
                paintPatterns[i]->buildSegments();
            }
        }
    }
}

void REVERAudioProcessor::saveSettings ()
{
    settings.closeFiles(); // FIX files changed by other plugin instances not loading
    if (auto* file = settings.getUserSettings()) {
        file->setValue("scale", scale);
        file->setValue("width", plugWidth);
        file->setValue("height", plugHeight);
        file->setValue("irdir", irDir);
        for (int i = 0; i < PAINT_PATS; ++i) {
            std::ostringstream oss;
            auto points = paintPatterns[i]->points;
            for (const auto& point : points) {
                oss << point.x << " " << point.y << " " << point.tension << " " << point.type << " ";
            }
            file->setValue("paintpat"+juce::String(i), var(oss.str()));
        }
    }
    settings.saveIfNeeded();
}

void REVERAudioProcessor::setScale(float s)
{
    scale = s;
    saveSettings();
}

int REVERAudioProcessor::getCurrentGrid()
{
    auto gridIndex = (int)params.getRawParameterValue("grid")->load();
    return GRID_SIZES[gridIndex];
}

int REVERAudioProcessor::getCurrentSeqStep()
{
    auto gridIndex = (int)params.getRawParameterValue("seqstep")->load();
    return GRID_SIZES[gridIndex];
}

void REVERAudioProcessor::createUndoPoint(int patindex)
{
    if (patindex == -1) {
        viewPattern->createUndo();
    }
    else {
        if (patindex < 12) {
            patterns[patindex]->createUndo();
        }
        else if (patindex < 24) {
            sendpatterns[patindex - 12]->createUndo();
        }
        else {
            paintPatterns[patindex - PAINT_PATS_IDX]->createUndo();
        }
    }
    updateSendFromPattern();
    updateReverbFromPattern();
    sendChangeMessage(); // UI repaint
}

/*
    Used to create an undo point from a previously saved state
    Assigns the snapshot points to the pattern temporarily
    Creates an undo point and finally replaces back the points
*/
void REVERAudioProcessor::createUndoPointFromSnapshot(std::vector<PPoint> snapshot)
{
    if (!Pattern::comparePoints(snapshot, viewPattern->points)) {
        auto points = viewPattern->points;
        viewPattern->points = snapshot;
        createUndoPoint();
        viewPattern->points = points;
        updateSendFromPattern();
        updateReverbFromPattern();
    }
}

void REVERAudioProcessor::setSendEditMode(bool isSend)
{
    MessageManager::callAsync([this, isSend] {
        if (sendEditMode == isSend) return;
        auto seqopen = sequencer->isOpen;
        if (seqopen) sequencer->close();

        sendEditMode = isSend;
        if (uimode != UIMode::PaintEdit) {
            viewPattern = sendEditMode ? sendpattern : pattern;
            viewSubPattern = sendEditMode ? pattern : sendpattern;
        }
        if (seqopen) sequencer->open();
        sendChangeMessage();
    });
}

void REVERAudioProcessor::setUIMode(UIMode mode)
{
    MessageManager::callAsync([this, mode]() {
        if ((mode != Seq && mode != PaintEdit) && sequencer->isOpen)
            sequencer->close();

        if (mode == UIMode::Normal) {
            viewPattern = sendEditMode ? sendpattern : pattern;
            viewSubPattern = sendEditMode ? pattern : sendpattern;
            showSequencer = false;
            showPaintWidget = false;
        }
        else if (mode == UIMode::Paint) {
            viewPattern = sendEditMode ? sendpattern : pattern;
            viewSubPattern = sendEditMode ? pattern : sendpattern;
            showPaintWidget = true;
            showSequencer = false;
        }
        else if (mode == UIMode::PaintEdit) {
            viewPattern = paintPatterns[paintTool];
            showPaintWidget = true;
            showSequencer = false;
        }
        else if (mode == UIMode::Seq) {
            if (sequencer->isOpen) {
                sequencer->close(); // just in case its changing from PaintEdit back to sequencer
            }
            sequencer->open();
            viewPattern = sendEditMode ? sendpattern : pattern;
            viewSubPattern = sendEditMode ? pattern : sendpattern;
            showPaintWidget = sequencer->selectedShape == CellShape::SPTool;
            showSequencer = true;
        }
        luimode = uimode;
        uimode = mode;
        sendChangeMessage();
    });
}

void REVERAudioProcessor::togglePaintMode()
{
    setUIMode(uimode == UIMode::Paint
        ? UIMode::Normal
        : UIMode::Paint
    );
}

void REVERAudioProcessor::togglePaintEditMode()
{
    setUIMode(uimode == UIMode::PaintEdit
        ? luimode
        : UIMode::PaintEdit
    );
}

void REVERAudioProcessor::toggleSequencerMode()
{
    setUIMode(uimode == UIMode::Seq
        ? UIMode::Normal
        : UIMode::Seq
    );
}

Pattern* REVERAudioProcessor::getPaintPatern(int index)
{
    return paintPatterns[index];
}

void REVERAudioProcessor::setViewPattern(int index)
{
    if (index >= 0 && index < 12) {
        viewPattern = patterns[index];
    }
    else if (index >= PAINT_PATS && index < PAINT_PATS_IDX + PAINT_PATS) {
        viewPattern = paintPatterns[index - PAINT_PATS_IDX];
    }
    sendChangeMessage();
}

void REVERAudioProcessor::restorePaintPatterns()
{
    for (int i = 0; i < 8; ++i) {
        paintPatterns[i]->clear();
        paintPatterns[i]->clearUndo();
        auto preset = Presets::getPaintPreset(i);
        for (auto& point : preset) {
            paintPatterns[i]->insertPoint(point.x, point.y, point.tension, point.type);
        }
        paintPatterns[i]->buildSegments();
    }
    sendChangeMessage();
}

//==============================================================================
const juce::String REVERAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool REVERAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool REVERAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool REVERAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double REVERAudioProcessor::getTailLengthSeconds() const
{
    // TODO
    return 0.0;
}

int REVERAudioProcessor::getNumPrograms()
{
    return 40;
}

int REVERAudioProcessor::getCurrentProgram()
{
    return currentProgram == -1 ? 0 : currentProgram;
}

void REVERAudioProcessor::setCurrentProgram (int index)
{
    if (currentProgram == index) return;
    loadProgram(index);
}

void REVERAudioProcessor::loadProgram (int index)
{
    if (sequencer->isOpen)
        sequencer->close();

    currentProgram = index;
    auto loadPreset = [](Pattern& pat, int idx) {
        auto preset = pat.index >= 12
            ? Presets::getResPreset(idx)
            : Presets::getCutPreset(idx);
        pat.clear();
        for (auto p = preset.begin(); p < preset.end(); ++p) {
            pat.insertPoint(p->x, p->y, p->tension, p->type);
        }
        pat.buildSegments();
        pat.clearUndo();
    };

    if (index == 0) { // Init
        for (int i = 0; i < 12; ++i) {
            patterns[i]->clear();
            patterns[i]->insertPoint(0.0, 0.0, 0, 1);
            patterns[i]->buildSegments();
            patterns[i]->clearUndo();

            sendpatterns[i]->clear();
            sendpatterns[i]->insertPoint(0.0, 1.0, 0, 1);
            sendpatterns[i]->buildSegments();
            sendpatterns[i]->clearUndo();
        }
    }
    else if (index == 1 || index == 14 || index == 27) {
        for (int i = 0; i < 12; ++i) {
            loadPreset(*patterns[i], index + i);
            loadPreset(*sendpatterns[i], index + i);
        }
    }
    else {
        loadPreset(*pattern, index - 1);
    }

    updateReverbFromPattern();
    updateSendFromPattern();
    setUIMode(UIMode::Normal);
    sendChangeMessage(); // UI Repaint
}

const juce::String REVERAudioProcessor::getProgramName (int index)
{
    static const std::array<juce::String, 40> progNames = {
        "Init",
        "Load Patterns 01-12", "Basic 1", "Basic 2", "Basic 3", "Basic 4", "Basic 5", "Basic 6", "Basic 7", "Basic 8", "Basic 9", "Basic 10", "Basic 11", "Basic 12",
        "Load Patterns 13-25", "Gate 1", "Gate 2", "Gate 3", "Gate 4", "Gate 5", "Gate 6", "Gate 7", "Gate 8", "Gate 9", "Gate 10", "Gate 11", "Gate 12",
        "Load Patterns 26-38", "Waves 1", "Waves 2", "Waves 3", "Waves 4", "Waves 5", "Waves 6", "FX 1", "FX 2", "FX 3", "FX 4", "FX 5", "FX 6"
    };
    return progNames.at(index);
}

void REVERAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    (void)index;
    (void)newName;
}

//==============================================================================
void REVERAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    warmer.setSize(2, (int)std::ceil(sampleRate)); // 1 second of warmup samples
    convolver->prepare(samplesPerBlock);
    loadConvolver->prepare(samplesPerBlock);
    yrevBuffer.resize(samplesPerBlock, 0.0f);
    ysendBuffer.resize(samplesPerBlock, 0.0f);
    xposBuffer.resize(samplesPerBlock, 0.0f);
    wetBuffer.setSize(2, samplesPerBlock);
    sendBuffer.setSize(2, samplesPerBlock);

    if (!init) {
        impulse->attack = params.getRawParameterValue("irattack")->load();
        impulse->decay = params.getRawParameterValue("irdecay")->load();
        impulse->trimLeft = params.getRawParameterValue("irtrimleft")->load();
        impulse->trimRight = params.getRawParameterValue("irtrimright")->load();
        impulse->stretch = params.getRawParameterValue("irstretch")->load();
        impulse->load(irFile);
    }

    convolver->loadImpulse(*impulse);

    revenvBuffer.resize(samplesPerBlock, 0.f);
    sendenvBuffer.resize(samplesPerBlock, 0.f);
    audioHighcutL.reset(0.0f);
    audioHighcutR.reset(0.0f);
    audioLowcutL.reset(0.0f);
    audioLowcutR.reset(0.0f);
    transDetectorL.clear(sampleRate);
    transDetectorR.clear(sampleRate);
    std::fill(monSamples.begin(), monSamples.end(), 0.0f);
    clearLatencyBuffers();
    onSlider(); // sets latency
    init = true;
}

void REVERAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool REVERAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void REVERAudioProcessor::onSlider()
{
    onSmoothChange();
    auto srate = getSampleRate();

    int trigger = (int)params.getRawParameterValue("trigger")->load();
    if (trigger != ltrigger) {
        auto latency = getLatencySamples();
        setLatencySamples(trigger == Trigger::Audio
            ? (int)std::ceil(getSampleRate() * LATENCY_MILLIS / 1000.0)
            :0
        );
        if (getLatencySamples() != latency && playing) {
            showLatencyWarning = true;
            MessageManager::callAsync([this]() { sendChangeMessage(); });
        }
        clearLatencyBuffers();
        ltrigger = trigger;
    }
    if (trigger == Trigger::Sync && alwaysPlaying)
        alwaysPlaying = false; // force alwaysPlaying off when trigger is not MIDI or Audio

    if (trigger != Trigger::MIDI && midiTrigger)
        midiTrigger = false;

    if (trigger != Trigger::Audio && audioTrigger)
        audioTrigger = false;

    if (trigger != Trigger::Audio && useMonitor)
        useMonitor = false;

    auto tension = (double)params.getRawParameterValue("tension")->load();
    auto tensionatk = (double)params.getRawParameterValue("tensionatk")->load();
    auto tensionrel = (double)params.getRawParameterValue("tensionrel")->load();
    if (tension != ltension || tensionatk != ltensionatk || tensionrel != ltensionrel) {
        onTensionChange();
        ltensionatk = tensionatk;
        ltensionrel = tensionrel;
        ltension = tension;
    }

    auto sync = (int)params.getRawParameterValue("sync")->load();
    if (sync == 0) syncQN = 1.; // not used
    else if (sync == 1) syncQN = 1./4.; // 1/16
    else if (sync == 2) syncQN = 1./2.; // 1/8
    else if (sync == 3) syncQN = 1./1.; // 1/4
    else if (sync == 4) syncQN = 1.*2.; // 1/2
    else if (sync == 5) syncQN = 1.*4.; // 1bar
    else if (sync == 6) syncQN = 1.*8.; // 2bar
    else if (sync == 7) syncQN = 1.*16.; // 4bar
    else if (sync == 8) syncQN = 1./6.; // 1/16t
    else if (sync == 9) syncQN = 1./3.; // 1/8t
    else if (sync == 10) syncQN = 2./3.; // 1/4t
    else if (sync == 11) syncQN = 4./3.; // 1/2t
    else if (sync == 12) syncQN = 8./3.; // 1/1t
    else if (sync == 13) syncQN = 1./4.*1.5; // 1/16.
    else if (sync == 14) syncQN = 1./2.*1.5; // 1/8.
    else if (sync == 15) syncQN = 1./1.*1.5; // 1/4.
    else if (sync == 16) syncQN = 2./1.*1.5; // 1/2.
    else if (sync == 17) syncQN = 4./1.*1.5; // 1/1.

    auto highcut = params.getRawParameterValue("highcut")->load();
    auto lowcut = params.getRawParameterValue("lowcut")->load();
    audioHighcutL.lp((float)srate, highcut, 0.707f);
    audioHighcutR.lp((float)srate, highcut, 0.707f);
    audioLowcutL.hp((float)srate, lowcut, 0.707f);
    audioLowcutR.hp((float)srate, lowcut, 0.707f);

    if (reverbDirty) {
        float avg = (float)pattern->getavgY();
        params.getParameter("reverb")->setValueNotifyingHost(avg);
        lreverb = (double)params.getRawParameterValue("reverb")->load();
        reverbDirty = false;
        reverbDirtyCooldown = 5; // ignore reverb updates for 5 blocks
    }

    if (sendDirty) {
        float avg = (float)sendpattern->getavgY();
        params.getParameter("send")->setValueNotifyingHost(avg);
        lsend = (double)params.getRawParameterValue("send")->load();
        sendDirty = false;
        sendDirtyCooldown = 5;
    }

    double reverb = (double)params.getRawParameterValue("reverb")->load();
    double send = (double)params.getRawParameterValue("send")->load();

    // Ignores DAW updates for reverb which was changed internally
    // DAW param updates are not reliable, on standalone works fine
    if (reverbDirtyCooldown > 0) {
        lreverb = reverb;
    }
    else if (reverb != lreverb) {
        updatePatternFromReverb();
        lreverb = reverb;
    }

    if (sendDirtyCooldown > 0) {
        lsend = send;
    }
    else if (send != lsend) {
        updateSendPatternFromSend();
        lsend = send;
    }

    bool sendenvOn = (bool)params.getRawParameterValue("sendenvon")->load();
    bool revenvOn = (bool)params.getRawParameterValue("revenvon")->load();

    if (revenvOn) {
        float thresh = params.getRawParameterValue("revenvthresh")->load();
        float attack = params.getRawParameterValue("revenvatk")->load();
        float release = params.getRawParameterValue("revenvrel")->load();
        float revenvLowCut = params.getRawParameterValue("revenvlowcut")->load();
        float revenvHighCut = params.getRawParameterValue("revenvhighcut")->load();
        revenv.prepare((float)srate, thresh, revenvAutoRel, attack, 0.0, release, revenvLowCut, revenvHighCut);
    }

    if (sendenvOn) {
        float thresh = params.getRawParameterValue("sendenvthresh")->load();
        float attack = params.getRawParameterValue("sendenvatk")->load();
        float release = params.getRawParameterValue("sendenvrel")->load();
        float sendenvLowCut = params.getRawParameterValue("sendenvlowcut")->load();
        float sendenvHighCut = params.getRawParameterValue("sendenvhighcut")->load();
        sendenv.prepare((float)srate, thresh, resenvAutoRel, attack, 0.0, release, sendenvLowCut, sendenvHighCut);
    }

    // convolver
    float irattack = params.getRawParameterValue("irattack")->load();
    float irdecay = params.getRawParameterValue("irdecay")->load();
    float irtrimleft = params.getRawParameterValue("irtrimleft")->load();
    float irtrimright = params.getRawParameterValue("irtrimright")->load();
    float irstretch = params.getRawParameterValue("irstretch")->load();

    if (irtrimleft > 1.0f - irtrimright) {
        params.getParameter("irtrimleft")->setValueNotifyingHost(1.0f-irtrimright);
    } 
    else if (irattack != impulse->attack 
        || irdecay != impulse->decay 
        || irtrimleft != impulse->trimLeft 
        || irtrimright != impulse->trimRight 
        || impulse->stretch != irstretch
    ) {
        impulse->attack = irattack;
        impulse->decay = irdecay;
        impulse->trimLeft = irtrimleft;
        impulse->trimRight = irtrimright;
        impulse->stretch = irstretch;
        irDirty = true;
    }
}

void REVERAudioProcessor::updatePatternFromReverb()
{
    float revnorm = params.getParameter("reverb")->getValue();
    pattern->transform(revnorm);
}

void REVERAudioProcessor::updateSendPatternFromSend()
{
    float sendnorm = params.getParameter("send")->getValue();
    sendpattern->transform(sendnorm);
}

void REVERAudioProcessor::updateReverbFromPattern()
{
    reverbDirty = true;
    paramChanged = true;
}

void REVERAudioProcessor::updateSendFromPattern()
{
    sendDirty = true;
    paramChanged = true;
}

void REVERAudioProcessor::onTensionChange()
{
    auto tension = (double)params.getRawParameterValue("tension")->load();
    auto tensionatk = (double)params.getRawParameterValue("tensionatk")->load();
    auto tensionrel = (double)params.getRawParameterValue("tensionrel")->load();
    pattern->setTension(tension, tensionatk, tensionrel, dualTension);
    sendpattern->setTension(tension, tensionatk, tensionrel, dualTension);
    pattern->buildSegments();
    sendpattern->buildSegments();
    for (int i = 0; i < PAINT_PATS; ++i) {
        paintPatterns[i]->setTension(tension, tensionatk, tensionrel, dualTension);
        paintPatterns[i]->buildSegments();
    }
}

void REVERAudioProcessor::onPlay()
{
    std::fill(revenvBuffer.begin(), revenvBuffer.end(), 0.f);
    std::fill(sendenvBuffer.begin(), sendenvBuffer.end(), 0.f);
    clearWaveBuffers();
    clearLatencyBuffers();
    sendenv.clear();
    revenv.clear();
    int trigger = (int)params.getRawParameterValue("trigger")->load();
    double ratehz = (double)params.getRawParameterValue("rate")->load();
    double phase = (double)params.getRawParameterValue("phase")->load();

    midiTrigger = false;
    audioTrigger = false;

    beatPos = ppqPosition;
    ratePos = beatPos * secondsPerBeat * ratehz;
    trigpos = 0.0;
    trigposSinceHit = 1.0;
    trigphase = phase;

    audioTriggerCountdown = -1;
    double srate = getSampleRate();
    transDetectorL.clear(srate);
    transDetectorR.clear(srate);

    if (trigger == 0 || alwaysPlaying) {
        restartEnv(false);
    }
}

void REVERAudioProcessor::restartEnv(bool fromZero)
{
    int sync = (int)params.getRawParameterValue("sync")->load();
    double min = (double)params.getRawParameterValue("min")->load();
    double max = (double)params.getRawParameterValue("max")->load();
    double phase = (double)params.getRawParameterValue("phase")->load();
    double revoffset = (double)params.getRawParameterValue("revoffset")->load();
    double sendoffset = (double)params.getRawParameterValue("sendoffset")->load();

    if (fromZero) { // restart from phase
        xpos = phase;
    }
    else { // restart from beat pos
        xpos = sync > 0
            ? beatPos / syncQN + phase
            : ratePos + phase;
        xpos -= std::floor(xpos);

        revvalue->reset(getYRev(xpos, min, max, revoffset));
        sendvalue->reset(getYSend(xpos, min, max, sendoffset));
    }
}

void REVERAudioProcessor::onStop()
{
    if (showLatencyWarning) {
        showLatencyWarning = false;
        MessageManager::callAsync([this]() { sendChangeMessage(); });
    }
}

void REVERAudioProcessor::clearWaveBuffers()
{
    std::fill(preSamples.begin(), preSamples.end(), 0.0f);
    std::fill(postSamples.begin(), postSamples.end(), 0.0f);
}

void REVERAudioProcessor::clearLatencyBuffers()
{
    int trigger = (int)params.getRawParameterValue("trigger")->load();
    auto latency = trigger == Trigger::Audio
        ? (int)std::ceil(getSampleRate() * LATENCY_MILLIS / 1000.0)
        : 0;
    latBufferL.resize(latency, 0.0f); // these are latency buffers for audio trigger only
    latBufferR.resize(latency, 0.0f);
    monLatBufferL.resize(getLatencySamples(), 0.0f);
    monLatBufferR.resize(getLatencySamples(), 0.0f);
    latpos = 0;
    monWritePos = 0;
}

double inline REVERAudioProcessor::getYRev(double x, double min, double max, double offset)
{
    return std::clamp(min + (max - min) * (1 - pattern->get_y_at(x)) + offset, 0.0, 1.0);
}

double inline REVERAudioProcessor::getYSend(double x, double min, double max, double offset)
{
    return std::clamp(min + (max - min) * (1 - sendpattern->get_y_at(x)) + offset, 0.0, 1.0);
}

void REVERAudioProcessor::onSmoothChange()
{
    auto srate = getSampleRate();
    if (dualSmooth) {
        double attack = (double)params.getRawParameterValue("attack")->load();
        double release = (double)params.getRawParameterValue("release")->load();
        attack *= attack;
        release *= release;
        revvalue->setup(attack * 0.25, release * 0.25, srate);
        sendvalue->setup(attack * 0.25, release * 0.25, srate);
    }
    else {
        double lfosmooth = (double)params.getRawParameterValue("smooth")->load();
        lfosmooth *= lfosmooth * 0.25;
        revvalue->setup(lfosmooth * 0.25, lfosmooth * 0.25, srate);
        sendvalue->setup(lfosmooth * 0.25, lfosmooth * 0.25, srate);
    }
}

void REVERAudioProcessor::queuePattern(int patidx)
{
    queuedPattern = patidx;
    queuedPatternCountdown = 0;
    int patsync = (int)params.getRawParameterValue("patsync")->load();

    if (playing && patsync != PatSync::Off) {
        int interval = samplesPerBeat;
        if (patsync == PatSync::QuarterBeat)
            interval = interval / 4;
        else if (patsync == PatSync::HalfBeat)
            interval = interval / 2;
        else if (patsync == PatSync::Beat_x2)
            interval = interval * 2;
        else if (patsync == PatSync::Beat_x4)
            interval = interval * 4;
        queuedPatternCountdown = (interval - timeInSamples % interval) % interval;
    }
}

bool REVERAudioProcessor::supportsDoublePrecisionProcessing() const
{
    return false;
}

void REVERAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals disableDenormals;
    double srate = getSampleRate();
    int samplesPerBlock = getBlockSize();
    bool looping = false;
    double loopStart = 0.0;
    double loopEnd = 0.0;

    // Get playhead info
    if (auto* phead = getPlayHead()) {
        if (auto pos = phead->getPosition()) {
            if (auto ppq = pos->getPpqPosition())
                ppqPosition = *ppq;
            if (auto tempo = pos->getBpm()) {
                beatsPerSecond = *tempo / 60.0;
                beatsPerSample = *tempo / (60.0 * srate);
                samplesPerBeat = (int)((60.0 / *tempo) * srate);
                secondsPerBeat = 60.0 / *tempo;
            }
            looping = pos->getIsLooping();
            if (auto loopPoints = pos->getLoopPoints()) {
                loopStart = loopPoints->ppqStart;
                loopEnd = loopPoints->ppqEnd;
            }
            auto play = pos->getIsPlaying();
            if (!playing && play) // playback started
                onPlay();
            else if (playing && !play) // playback stopped
                onStop();

            playing = play;
            if (playing) {
                if (auto samples = pos->getTimeInSamples()) {
                    timeInSamples = *samples;
                }
            }
        }
    }

    int inputBusCount = getBusCount(true);
    int audioOutputs = getTotalNumOutputChannels();
    int audioInputs = inputBusCount > 0 ? getChannelCountOfBus(true, 0) : 0;
    int sideInputs = inputBusCount > 1 ? getChannelCountOfBus(true, 1) : 0;
    int numSamples = buffer.getNumSamples();

    if (!audioInputs || !audioOutputs)
        return;

    // load params
    int trigger = (int)params.getRawParameterValue("trigger")->load();
    int sync = (int)params.getRawParameterValue("sync")->load();
    float min = params.getRawParameterValue("min")->load();
    float max = params.getRawParameterValue("max")->load();
    float ratehz = params.getRawParameterValue("rate")->load();
    float phase = params.getRawParameterValue("phase")->load();
    float lowcut = params.getRawParameterValue("lowcut")->load();
    float highcut = params.getRawParameterValue("highcut")->load();
    int algo = (int)params.getRawParameterValue("algo")->load();
    float threshold = params.getRawParameterValue("threshold")->load();
    float sense = 1.0f - params.getRawParameterValue("sense")->load();
    float revoffset = params.getRawParameterValue("revoffset")->load();
    float sendoffset = params.getRawParameterValue("sendoffset")->load();
    bool revenvon = (bool)params.getRawParameterValue("revenvon")->load();
    bool sendenvon = (bool)params.getRawParameterValue("sendenvon")->load();
    float revenvamt = params.getRawParameterValue("revenvamt")->load();
    float sendenvamt = params.getRawParameterValue("sendenvamt")->load();
    float drywet = params.getRawParameterValue("drywet")->load();
    float width = params.getRawParameterValue("width")->load();
    sense = std::powf(sense, 2); // make audio trigger sensitivity more responsive

    // process viewport background display wave samples
    auto processDisplaySample = [&](int sampidx, double xpos, float prelsamp, float prersamp) {
        auto preamp = std::max(std::fabs(prelsamp), std::fabs(prersamp));
        auto postlsamp = buffer.getSample(0, sampidx);
        auto postrsamp = audioInputs > 1 ? buffer.getSample(1, sampidx) : postlsamp;
        auto postamp = std::max(std::fabs(postlsamp), std::fabs(postrsamp));
        winpos = (int)std::floor(xpos * viewW);
        if (lwinpos != winpos) {
            preSamples[winpos] = 0.0;
            postSamples[winpos] = 0.0;
        }
        lwinpos = winpos;
        if (preSamples[winpos] < preamp)
            preSamples[winpos] = preamp;
        if (postSamples[winpos] < postamp)
            postSamples[winpos] = postamp;
    };

    // process audio monitor samples
    float monIncrementPerSample = 1.0f / float((srate * 2) / monW); // 2 seconds of audio displayed on monitor
    auto processMonitorSample = [&](float lsamp, float rsamp, bool hit) {
        float indexd = monpos.load();
        indexd += monIncrementPerSample;

        if (indexd >= monW)
            indexd -= monW;

        int index = (int)indexd;
        if (lmonpos != index)
            monSamples[index] = 0.0;
        lmonpos = index;

        float maxamp = std::max(std::fabs(lsamp), std::fabs(rsamp));
        if (hit || monSamples[index] >= 10.0)
            maxamp = std::max(maxamp + 10.0f, lastHitAmplitude + 10.0f); // encode hits by adding +10 to amp

        monSamples[index] = std::max(monSamples[index], maxamp);
        monpos.store(indexd);
    };

    // applies envelope to a sample index
    //auto applyFilter = [&](int sampidx, double env, double resenv, double lsample, double rsample) {
    //    double cutoff = Utils::normalToFreq(env);
    //    lFilter->init(srate * samplingFactor, cutoff, resenv);
    //    rFilter->init(srate * samplingFactor, cutoff, resenv);
    //    double outl = lFilter->eval(lsample) * gain;
    //    double outr = rFilter->eval(rsample) * gain;
    //    lFilter->tick();
    //    rFilter->tick();
    //
    //    for (int channel = 0; channel < audioOutputs; ++channel) {
    //        auto wet = channel == 0 ? outl : outr;
    //        auto dry = (double)upsampledBlock.getSample(channel, sampidx);
    //        if (outputCV) {
    //            upsampledBlock.setSample(channel, sampidx, env);
    //        }
    //        else {
    //            upsampledBlock.setSample(channel, sampidx, wet * mix + dry * (1.0 - mix));
    //        }
    //    }
    //};

    if (paramChanged) {
        onSlider();
        paramChanged = false;
    }

    if (reverbDirtyCooldown > 0)
        reverbDirtyCooldown--;
    if (sendDirtyCooldown > 0)
        sendDirtyCooldown--;

    // Process new MIDI messages
    for (const auto metadata : midiMessages) {
        juce::MidiMessage message = metadata.getMessage();
        if (message.isNoteOn() || message.isNoteOff()) {
            midiIn.push_back({ // queue midi message
                metadata.samplePosition,
                message.isNoteOn(),
                message.getNoteNumber(),
                message.getVelocity(),
                message.getChannel() - 1
            });
        }
    }
    midiMessages.clear();

    // Process midi out queue
    for (auto it = midiOut.begin(); it != midiOut.end();) {
        auto& [msg, offset] = *it;
        if (offset < samplesPerBlock) {
            midiMessages.addEvent(msg, offset);
            it = midiOut.erase(it);
        }
        else {
            offset -= samplesPerBlock;
            ++it;
        }
    }

    // remove midi in messages that have been processed
    midiIn.erase(std::remove_if(midiIn.begin(), midiIn.end(), [](const MidiInMsg& msg) {
        return msg.offset < 0;
    }), midiIn.end());

    // update outputs with last block information at the start of the new block
    if (outputCC > 0) {
        auto val = (int)std::round(yrev*127.0);
        if (bipolarCC) val -= 64;
        auto cc = MidiMessage::controllerEvent(outputCCChan + 1, outputCC-1, val);
        midiMessages.addEvent(cc, 0);
    }

    // keep beatPos in sync with playhead so plugin can be bypassed and return to its sync pos
    if (playing) {
        beatPos = ppqPosition;
        ratePos = beatPos * secondsPerBeat * ratehz;
    }

    // audio trigger transient detection and monitoring
    // direct audio buffer processing, not oversampled
    if (trigger == Audio) {
        for (int sample = 0; sample < numSamples; ++sample) {
            // read audio samples
            float lsample = buffer.getSample(0, sample);
            float rsample = buffer.getSample(audioInputs > 1 ? 1 : 0, sample);

            // read sidechain samples
            float lsidesample = 0.0;
            float rsidesample = 0.0;
            if (useSidechain && sideInputs) {
                lsidesample = buffer.getSample(audioInputs, sample);
                rsidesample = buffer.getSample(sideInputs > 1 ? audioInputs + 1 : audioInputs, sample);
            }

            // Detect audio transients
            auto monSampleL = useSidechain ? lsidesample : lsample;
            auto monSampleR = useSidechain ? rsidesample : rsample;
            if (lowcut > 20.0) {
                monSampleL = audioLowcutL.df1(monSampleL);
                monSampleR = audioLowcutR.df1(monSampleR);
            }
            if (highcut < 20000.0) {
                monSampleL = audioHighcutL.df1(monSampleL);
                monSampleR = audioHighcutR.df1(monSampleR);
            }

            if (transDetectorL.detect(algo, monSampleL, threshold, sense) ||
                transDetectorR.detect(algo, monSampleR, threshold, sense))
            {
                transDetectorL.startCooldown();
                transDetectorR.startCooldown();
                int offset = (int)(params.getRawParameterValue("offset")->load() * LATENCY_MILLIS / 1000.f * srate);
                audioTriggerCountdown = (sample + std::max(0, getLatencySamples() + offset));
                lastHitAmplitude = transDetectorL.hit ? std::fabs(monSampleL) : std::fabs(monSampleR);
                processMonitorSample(monSampleL, monSampleR, true);
            }
            else {
                processMonitorSample(monSampleL, monSampleR, false);
            }

            // monLatBuffers keep the wet signal from audio trigger processing
            // so it can be monitored, this could maybe be refactored some better way
            monLatBufferL[monWritePos] = monSampleL;
            monLatBufferR[monWritePos] = monSampleR;

            int latency = (int)monLatBufferL.size();
            auto monReadPos = (monWritePos + latency - 1) % latency;
            if (useMonitor) {
                monSampleL = monLatBufferL[monReadPos];
                monSampleR = monLatBufferR[monReadPos];
                for (int channel = 0; channel < audioOutputs; ++channel) {
                    buffer.setSample(channel, sample, channel == 0 ? monSampleL : monSampleR);
                }
            }

            monWritePos = (monWritePos + 1) % latency;
        }
    }

    // process convolver warmer
    // warmer is a simple circular buffer that stores the last second of audio
    int spaceToEnd = warmer.getNumSamples() - warmwritepos;
    if (numSamples <= spaceToEnd) {
        warmer.copyFrom(0, warmwritepos, buffer, 0, 0, numSamples);
        warmer.copyFrom(1, warmwritepos, buffer, audioInputs > 1 ? 1 : 0, 0, numSamples);
    }
    else {
        warmer.copyFrom(0, warmwritepos, buffer, 0, 0, spaceToEnd);
        warmer.copyFrom(0, 0, buffer, 0, spaceToEnd, numSamples - spaceToEnd);
        warmer.copyFrom(1, warmwritepos, buffer, audioInputs > 1 ? 1 : 0, 0, spaceToEnd);
        warmer.copyFrom(1, 0, buffer, audioInputs > 1 ? 1 : 0, spaceToEnd, numSamples - spaceToEnd);
    }
    warmwritepos = (warmwritepos + numSamples) %  warmer.getNumSamples();

    std::fill(revenvBuffer.begin(), revenvBuffer.end(), 0.0f);
    std::fill(sendenvBuffer.begin(), sendenvBuffer.end(), 0.0f);
    std::fill(yrevBuffer.begin(), yrevBuffer.end(), 0.0f);
    std::fill(ysendBuffer.begin(), ysendBuffer.end(), 0.0f);
    std::fill(xposBuffer.begin(), xposBuffer.end(), 0.0f);

    // envelope follower processing
    for (int sample = 0; sample < numSamples; ++sample) {
        if (revenvon || sendenvon) {
            float lsample = buffer.getSample(0, sample);
            float rsample = buffer.getSample(audioInputs > 1 ? 1 : 0, sample);
            float lsidesample = sideInputs ? buffer.getSample(audioInputs, sample) : 0.0f;
            float rsidesample = sideInputs ? buffer.getSample(sideInputs > 1 ? audioInputs + 1 : audioInputs, sample) : 0.0f;

            if (revenvon) {
                revenvBuffer[sample] = revenv.process(
                    revenvSidechain ? lsidesample : lsample,
                    revenvSidechain ? rsidesample : rsample
                );
                if (revenvMonitor) {
                    buffer.setSample(0, sample, revenv.outl);
                    if (audioInputs > 1) {
                        buffer.setSample(1, sample, revenv.outr);
                    }
                }
            }

            if (sendenvon) {
                sendenvBuffer[sample] = sendenv.process(
                    sendenvSidechain ? lsidesample : lsample,
                    sendenvSidechain ? rsidesample : rsample
                );
                if (sendenvMonitor) {
                    buffer.setSample(0, sample, sendenv.outl);
                    if (audioInputs > 1) {
                        buffer.setSample(1, sample, sendenv.outr);
                    }
                }
            }
        }
    }

    // ================================================= MAIN PROCESSING LOOP

    for (int sample = 0; sample < numSamples; ++sample) {
        if (playing && looping && beatPos >= loopEnd) {
            beatPos = loopStart + (beatPos - loopEnd);
            ratePos = beatPos * secondsPerBeat * ratehz;
        }

        // process midi in queue
        for (auto& msg : midiIn) {
            if (msg.offset == 0) {
                if (msg.isNoteon) {
                    if (msg.channel == triggerChn || triggerChn == 16) {
                        auto patidx = msg.note % 12;
                        queuePattern(patidx + 1);
                    }
                    else if (trigger == Trigger::MIDI) {
                        clearWaveBuffers();
                        midiTrigger = !alwaysPlaying;
                        trigpos = 0.0;
                        trigphase = phase;
                        restartEnv(true);
                    }
                }
            }
            msg.offset -= 1;
        }

        // process queued pattern
        if (queuedPattern) {
            if (!playing || queuedPatternCountdown == 0) {
                if (sequencer->isOpen) {
                    sequencer->close(); // sync call (required)
                    setUIMode(UIMode::Normal); // async call
                }
                pattern = patterns[queuedPattern - 1];
                sendpattern = sendpatterns[queuedPattern - 1];
                viewPattern = sendEditMode ? sendpattern : pattern;
                viewSubPattern = sendEditMode ? pattern : sendpattern;
                auto tension = (double)params.getRawParameterValue("tension")->load();
                auto tensionatk = (double)params.getRawParameterValue("tensionatk")->load();
                auto tensionrel = (double)params.getRawParameterValue("tensionrel")->load();
                pattern->setTension(tension, tensionatk, tensionrel, dualTension);
                pattern->buildSegments();
                sendpattern->setTension(tension, tensionatk, tensionrel, dualTension);
                sendpattern->buildSegments();
                updateReverbFromPattern();
                updateSendFromPattern();
                MessageManager::callAsync([this]() { sendChangeMessage();});
                queuedPattern = 0;
            }
            if (queuedPatternCountdown > 0) {
                queuedPatternCountdown -= 1;
            }
        }

        // Sync mode
        if (trigger == Trigger::Sync) {
            xpos = sync > 0
                ? beatPos / syncQN + phase
                : ratePos + phase;
            xpos -= std::floor(xpos);

            // read envelope follower offset contribution
            auto roffset = revoffset;
            auto soffset = sendoffset;
            if (revenvon)
                roffset += revenvBuffer[sample] * revenvamt;
            if (sendenvon)
                soffset += sendenvBuffer[sample] * sendenvamt;
            

            double newypos = getYRev(xpos, min, max, roffset);
            yrev = revvalue->process(newypos, newypos > yrev);
            double newysend = getYSend(xpos, min, max, soffset);
            ysend = sendvalue->process(newysend, newysend > ysend);

            yrevBuffer[sample] = (float)yrev;
            ysendBuffer[sample] = (float)ysend;
            xposBuffer[sample] = (float)xpos;
            //processDisplaySample(sample, xpos, lsample, rsample);
        }

        // MIDI mode
        else if (trigger == Trigger::MIDI) {
            auto inc = sync > 0
                ? beatsPerSample / syncQN
                : 1 / srate * ratehz;
            xpos += inc;
            trigpos += inc;
            xpos -= std::floor(xpos);

            if (!alwaysPlaying) {
                if (midiTrigger) {
                    if (trigpos >= 1.0) { // envelope finished, stop midiTrigger
                        midiTrigger = false;
                        xpos = phase ? phase : 1.0;
                    }
                }
                else {
                    xpos = phase ? phase : 1.0; // midiTrigger is stopped, hold last position
                }
            }

            // read envelope follower offset contribution
            auto roffset = revoffset;
            auto soffset = sendoffset;
            if (revenvon)
                roffset += revenvBuffer[sample] * revenvamt;
            if (sendenvon)
                soffset += sendenvBuffer[sample] * sendenvamt;

            double newypos = getYRev(xpos, min, max, roffset);
            yrev = revvalue->process(newypos, newypos > yrev);
            double newysend = getYSend(xpos, min, max, soffset);
            ysend = sendvalue->process(newysend, newysend > ysend);
            double viewx = (alwaysPlaying || midiTrigger) ? xpos : (trigpos + trigphase) - std::floor(trigpos + trigphase);

            yrevBuffer[sample] = (float)yrev;
            ysendBuffer[sample] = (float)ysend;
            xposBuffer[sample] = (float)viewx;
        }

        // Audio mode
        else if (trigger == Trigger::Audio) {
            // read the sample 'latency' samples ago
            int latency = (int)latBufferL.size();
            int readPos = (latpos + latency - 1) % latency;
            latBufferL[latpos] = buffer.getSample(0, sample);
            latBufferR[latpos] = buffer.getSample(1, sample);
            auto lsample = latBufferL[readPos];
            auto rsample = latBufferR[readPos];

            // write delayed samples to buffer to later apply dry/wet mix
            for (int channel = 0; channel < 2; ++channel) {
                buffer.setSample(channel, sample, channel == 0 ? lsample : rsample);
            }

            auto hit = audioTriggerCountdown == 0; // there was an audio transient trigger in this sample

            // envelope processing
            auto inc = sync > 0
                ? beatsPerSample / syncQN
                : 1 / srate * ratehz;
            xpos += inc;
            trigpos += inc;
            trigposSinceHit += inc;
            xpos -= std::floor(xpos);

            // send output midi notes on audio trigger hit
            if (hit && outputATMIDI > 0) {
                auto noteOn = MidiMessage::noteOn(1, outputATMIDI - 1, (float)lastHitAmplitude);
                midiMessages.addEvent(noteOn, sample);

                auto offnoteDelay = static_cast<int>(srate * AUDIO_NOTE_LENGTH_MILLIS / 1000.0);
                int noteOffSample = sample + offnoteDelay;
                auto noteOff = MidiMessage::noteOff(1, outputATMIDI - 1);

                if (noteOffSample < samplesPerBlock) {
                    midiMessages.addEvent(noteOff, noteOffSample);
                }
                else {
                    int offset = noteOffSample - samplesPerBlock;
                    midiOut.push_back({ noteOff, offset });
                }
            }

            if (hit && (alwaysPlaying || !audioIgnoreHitsWhilePlaying || trigposSinceHit > 0.98)) {
                clearWaveBuffers();
                audioTrigger = !alwaysPlaying;
                trigpos = 0.0;
                trigphase = phase;
                trigposSinceHit = 0.0;
                restartEnv(true);
            }

            if (!alwaysPlaying) {
                if (audioTrigger) {
                    if (trigpos >= 1.0) { // envelope finished, stop trigger
                        audioTrigger = false;
                        xpos = phase ? phase : 1.0;
                    }
                }
                else {
                    xpos = phase ? phase : 1.0; // audioTrigger is stopped, hold last position
                }
            }

            // read envelope follower offset contribution
            auto roffset = revoffset;
            auto soffset = sendoffset;
            if (revenvon)
                roffset += revenvBuffer[sample] * revenvamt;
            if (sendenvon)
                soffset += sendenvBuffer[sample] * sendenvamt;

            double newypos = getYRev(xpos, min, max, roffset);
            yrev = revvalue->process(newypos, newypos > yrev);
            double newysend = getYSend(xpos, min, max, soffset);
            ysend = sendvalue->process(newysend, newysend > ysend);
            double viewx = (alwaysPlaying || audioTrigger) ? xpos : (trigpos + trigphase) - std::floor(trigpos + trigphase);

            yrevBuffer[sample] = (float)yrev;
            ysendBuffer[sample] = (float)ysend;
            xposBuffer[sample] = (float)viewx;

            latpos = (latpos + 1) % latency;

            if (audioTriggerCountdown > -1)
                audioTriggerCountdown -= 1;
        }

        xenv.store(xpos);
        yenv.store(sendEditMode ? ysend : yrev);
        beatPos += beatsPerSample;
        ratePos += 1 / srate * ratehz;
        if (playing)
            timeInSamples += 1;

    } // ============================================== END OF SAMPLES PROCESSING

    drawSeek.store(playing && (trigger == Trigger::Sync || midiTrigger || audioTrigger)); // informs UI if it should seek or not, typically only during play

    // with the envelopes processed, mix the envelope signals into wet and finally dry wet signal
    wetBuffer.clear();
    sendBuffer.clear();
    auto lchannel = buffer.getReadPointer(0);
    auto rchannel = buffer.getReadPointer(audioInputs > 1 ? 1 : 0);

    // process send envelope
    for (int sample = 0; sample < numSamples; ++sample) {
        sendBuffer.setSample(0, sample, lchannel[sample] * ysendBuffer[sample]);
        sendBuffer.setSample(1, sample, rchannel[sample] * ysendBuffer[sample]);
    }

    // process convolver
    
    // IR load state machine 
    // if loadstate is idle and there is an update reload the IR into the load convolver
    if (irDirty && loadState.load() == kIdle && loadCooldown <= 0) {
        loadCooldown = (int)(CONV_LOAD_COOLDOWN / 1000.0 * srate);
        irDirty = false;
        loadState.store(kLoading);

        threadPool.addJob([this, numSamples]() {
            if (impulse->path != irFile.toStdString()) {
                impulse->load(irFile);
                irFile = String(impulse->path);
            }
            impulse->recalcImpulse();
            sendChangeMessage();
            loadConvolver->loadImpulse(*impulse);
            loadState.store(kReady);
        });
    }

    // if new IR is loaded, warmup load convolver and begin crossfade with current convolver
    if (loadState.load() == kReady) {
        // warmup convolver
        int numBlocks = warmer.getNumSamples() / convolver->size;
        int start = (warmwritepos + 1) % warmer.getNumSamples();
        AudioBuffer<float> chunk;
        chunk.setSize(2, convolver->size);

        // copy warmup buffer in chunks into the new convolver
        for (int i = 0; i < numBlocks; ++i) {
            int end = (start + convolver->size) % warmer.getNumSamples();

            if (start < end) {
                chunk.copyFrom(0, 0, warmer, 0, start, convolver->size);
                chunk.copyFrom(1, 0, warmer, 1, start, convolver->size);
            } else {
                int toEnd = warmer.getNumSamples() - start;
                int remaining = convolver->size - toEnd;

                chunk.copyFrom(0, 0, warmer, 0, start, toEnd);
                chunk.copyFrom(1, 0, warmer, 1, start, toEnd);
                chunk.copyFrom(0, toEnd, warmer, 0, 0, remaining);
                chunk.copyFrom(1, toEnd, warmer, 1, 0, remaining);
            }

            loadConvolver->process(chunk.getReadPointer(0, 0), chunk.getReadPointer(1, 0), convolver->size);
            start = (start + convolver->size) % warmer.getNumSamples();
        }

        // start new crossfade
        loadState.store(kFading);
        xfade = (int)std::ceil(srate * CONV_XFADE / 1000.0);
        xfadelen = xfade;
    }

    if (loadCooldown > 0)
        loadCooldown -= numSamples;

    // process send input into the convolver
    convolver->process(
        sendBuffer.getReadPointer(0), 
        sendBuffer.getReadPointer(1),
        numSamples
    );

    // crossfade load convolver with current convolver signal
    if (loadState.load() == kFading) {
        loadConvolver->process(
            sendBuffer.getReadPointer(0),
            sendBuffer.getReadPointer(1),
            numSamples
        );

        for (int i = 0; i < convolver->bufferL.size(); ++i) {
            float alpha = std::clamp((1.f - (float)xfade / (float)xfadelen), 0.f, 1.f);
            convolver->bufferL[i] *= 1.f - alpha;
            convolver->bufferR[i] *= 1.f - alpha;
            loadConvolver->bufferL[i] *= alpha;
            loadConvolver->bufferR[i] *= alpha;
            xfade--;
        }

        if (xfade <= 0) {
            loadState.store(kIdle);
            std::swap(loadConvolver, convolver);
        }

        wetBuffer.addFrom(0, 0, loadConvolver->bufferL.data(), numSamples, 1.f);
        wetBuffer.addFrom(1, 0, loadConvolver->bufferR.data(), numSamples, 1.f);
    }

    // apply the convolver to the wet buffer (after crossfade)
    wetBuffer.addFrom(0, 0, convolver->bufferL.data(), numSamples, 1.f);
    wetBuffer.addFrom(1, 0, convolver->bufferR.data(), numSamples, 1.f);

    // apply reverb envelope and stereo width to the wet buffer
    lchannel = wetBuffer.getReadPointer(0);
    rchannel = wetBuffer.getReadPointer(1);
    for (int sample = 0; sample < numSamples; ++sample) {
        auto lin = lchannel[sample] * yrevBuffer[sample];
        auto rin = rchannel[sample] * yrevBuffer[sample];

        auto mid = (lin + rin) * 0.5f;
        auto side = (lin - rin) * 0.5f;

        auto lout = mid + side * (width * 2.f);
        auto rout = mid - side * (width * 2.f);

        wetBuffer.setSample(0, sample, lout);
        wetBuffer.setSample(1, sample, rout);
    }

    // finally mix the dry and wet signals
    if (!revenvMonitor && !sendenvMonitor && !useMonitor) {
        float dryGain, wetGain;

        if (drywet <= 0.5f) {
            dryGain = 1.0f;
            wetGain = drywet * 2.0f;
        } else {
            dryGain = (1.0f - drywet) * 2.0f;
            wetGain = 1.0f;
        }

        buffer.applyGain(dryGain);
        wetBuffer.applyGain(wetGain);

        buffer.addFrom(0, 0, wetBuffer.getReadPointer(0), numSamples);
        if (audioOutputs > 1) {
            buffer.addFrom(1, 0, wetBuffer.getReadPointer(1), numSamples);
        }
    }
}

//==============================================================================
bool REVERAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* REVERAudioProcessor::createEditor()
{
    return new REVERAudioProcessorEditor (*this);
}

//==============================================================================
void REVERAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = ValueTree("PluginState");
    state.appendChild(params.copyState(), nullptr);
    state.setProperty("version", PROJECT_VERSION, nullptr);
    state.setProperty("currentProgram", currentProgram, nullptr);
    state.setProperty("alwaysPlaying",alwaysPlaying, nullptr);
    state.setProperty("dualSmooth",dualSmooth, nullptr);
    state.setProperty("dualTension",dualTension, nullptr);
    state.setProperty("triggerChn",triggerChn, nullptr);
    state.setProperty("useSidechain",useSidechain, nullptr);
    state.setProperty("outputCC", outputCC, nullptr);
    state.setProperty("outputCCChan", outputCCChan, nullptr);
    state.setProperty("outputCV", outputCV, nullptr);
    state.setProperty("outputATMIDI", outputATMIDI, nullptr);
    state.setProperty("bipolarCC", bipolarCC, nullptr);
    state.setProperty("paintTool", paintTool, nullptr);
    state.setProperty("paintPage", paintPage, nullptr);
    state.setProperty("pointMode", pointMode, nullptr);
    state.setProperty("audioIgnoreHitsWhilePlaying", audioIgnoreHitsWhilePlaying, nullptr);
    state.setProperty("revenvSidechain", revenvSidechain, nullptr);
    state.setProperty("revenvAutoRel", revenvAutoRel, nullptr);
    state.setProperty("sendenvSidechain", sendenvSidechain, nullptr);
    state.setProperty("sendenvAutoRel", resenvAutoRel, nullptr);
    state.setProperty("linkSeqToGrid", linkSeqToGrid, nullptr);
    state.setProperty("currpattern", pattern->index + 1, nullptr);
    state.setProperty("currsendpattern", sendpattern->index - 12 + 1, nullptr);
    state.setProperty("irfile", irFile, nullptr);

    for (int i = 0; i < 12; ++i) {
        std::ostringstream oss;
        std::ostringstream ossres;
        auto points = patterns[i]->points;

        if (sequencer->isOpen && i == sequencer->patternIdx) {
            points = sequencer->backup;
        }

        for (const auto& point : points) {
            oss << point.x << " " << point.y << " " << point.tension << " " << point.type << " ";
        }
        state.setProperty("pattern" + juce::String(i), var(oss.str()), nullptr);

        points = sendpatterns[i]->points;

        if (sequencer->isOpen && i == sequencer->patternIdx - 12) {
            points = sequencer->backup;
        }

        for (const auto& point : points) {
            ossres << point.x << " " << point.y << " " << point.tension << " " << point.type << " ";
        }
        state.setProperty("sendpattern" + juce::String(i), var(ossres.str()), nullptr);
    }

    // serialize sequencer cells
    std::ostringstream oss;
    for (const auto& cell : sequencer->cells) {
        oss << cell.shape << ' '
            << cell.lshape << ' '
            << cell.ptool << ' '
            << cell.invertx << ' '
            << cell.minx << ' '
            << cell.maxx << ' '
            << cell.miny << ' '
            << cell.maxy << ' '
            << cell.tenatt << ' '
            << cell.tenrel << ' '
            << cell.skew << '\n';
    }
    state.setProperty("seqcells", var(oss.str()), nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void REVERAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (sequencer->isOpen) {
        sequencer->close();
    }

    std::unique_ptr<juce::XmlElement>xmlState (getXmlFromBinary (data, sizeInBytes));
    if (!xmlState) return;
    auto state = ValueTree::fromXml (*xmlState);
    if (!state.isValid()) return;

    params.replaceState(state.getChild(0));
    if (state.hasProperty("version")) {
        currentProgram = (int)state.getProperty("currentProgram");
        alwaysPlaying = (bool)state.getProperty("alwaysPlaying");
        dualSmooth = (bool)state.getProperty("dualSmooth");
        dualTension = (bool)state.getProperty("dualTension");
        triggerChn = (int)state.getProperty("triggerChn");
        useSidechain = (bool)state.getProperty("useSidechain");
        outputCC = (int)state.getProperty("outputCC");
        outputCCChan = (int)state.getProperty("outputCCChan");
        bipolarCC = (bool)state.getProperty("bipolarCC");
        outputCV = (bool)state.getProperty("outputCV");
        outputATMIDI = (int)state.getProperty("outputATMIDI");
        paintTool = (int)state.getProperty("paintTool");
        paintPage = (int)state.getProperty("paintPage");
        pointMode = state.hasProperty("pointMode") ? (int)state.getProperty("pointMode") : 1;
        audioIgnoreHitsWhilePlaying = (bool)state.getProperty("audioIgnoreHitsWhilePlaying");
        revenvSidechain = (bool)state.getProperty("revenvSidechain");
        revenvAutoRel = (bool)state.getProperty("revenvAutoRel");
        sendenvSidechain = (bool)state.getProperty("sendenvSidechain");
        resenvAutoRel = (bool)state.getProperty("sendenvAutoRel");
        linkSeqToGrid = state.hasProperty("linkSeqToGrid") ? (bool)state.getProperty("linkSeqToGrid") : true;
        if (state.hasProperty("irfile")) irFile = state.getProperty("irfile");

        int currpattern = state.hasProperty("currpattern")
            ? state.getProperty("currpattern")
            : (int)params.getRawParameterValue("pattern")->load();
        queuePattern(currpattern);
        auto param = params.getParameter("pattern");
        param->setValueNotifyingHost(param->convertTo0to1((float)currpattern));

        for (int i = 0; i < 12; ++i) {
            patterns[i]->clear();
            patterns[i]->clearUndo();
            sendpatterns[i]->clear();
            sendpatterns[i]->clearUndo();

            auto str = state.getProperty("pattern" + String(i)).toString().toStdString();
            if (!str.empty()) {
                double x, y, tension;
                int type;
                std::istringstream iss(str);
                while (iss >> x >> y >> tension >> type) {
                    patterns[i]->insertPoint(x,y,tension,type);
                }
            }

            str = state.getProperty("sendpattern" + String(i)).toString().toStdString();
            if (!str.empty()) {
                double x, y, tension;
                int type;
                std::istringstream iss(str);
                while (iss >> x >> y >> tension >> type) {
                    sendpatterns[i]->insertPoint(x,y,tension,type);
                }
            }

            auto tension = (double)params.getRawParameterValue("tension")->load();
            auto tensionatk = (double)params.getRawParameterValue("tensionatk")->load();
            auto tensionrel = (double)params.getRawParameterValue("tensionrel")->load();
            patterns[i]->setTension(tension, tensionatk, tensionrel, dualTension);
            patterns[i]->buildSegments();
            sendpatterns[i]->setTension(tension, tensionatk, tensionrel, dualTension);
            sendpatterns[i]->buildSegments();
        }

        updateReverbFromPattern();
        updateSendFromPattern();

        if (state.hasProperty("seqcells")) {
            auto str = state.getProperty("seqcells").toString().toStdString();
            sequencer->cells.clear();
            std::istringstream iss(str);
            Cell cell;
            int shape, lshape;
            while (iss >> shape >> lshape >> cell.ptool >> cell.invertx
                >> cell.minx >> cell.maxx >> cell.miny >> cell.maxy >> cell.tenatt
                >> cell.tenrel >> cell.skew)
            {
                cell.shape = static_cast<CellShape>(shape);
                cell.lshape = static_cast<CellShape>(lshape);
                sequencer->cells.push_back(cell);
            }
        }
    }

    setUIMode(Normal);
    irDirty = true;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new REVERAudioProcessor();
}

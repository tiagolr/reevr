/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include "dsp/Pattern.h"
#include "dsp/Transient.h"
#include "dsp/RBJ.h"
#include "Presets.h"
#include <atomic>
#include <deque>
#include "Globals.h"
#include "ui/Sequencer.h"
#include "dsp/Utils.h"
#include "dsp/Follower.h"
#include "dsp/StereoConvolver.h"
#include "dsp/Impulse.h"

using namespace globals;

struct MidiInMsg {
    int offset;
    int isNoteon;
    int note;
    int vel;
    int channel;
};

struct MidiOutMsg {
    MidiMessage msg;
    int offset;
};

enum Trigger {
    Sync,
    MIDI,
    Audio
};

enum PatSync {
    Off,
    QuarterBeat,
    HalfBeat,
    Beat_x1,
    Beat_x2,
    Beat_x4
};

enum UIMode {
    Normal,
    Paint,
    PaintEdit,
    Seq
};

enum LoadState {
    kIdle,
    kLoading,
    kReady,
    kFading,
};

/*
    RC lowpass filter with two resitances a or b
    Used for attack release smooth of ypos
*/
class RCSmoother
{
public:
    double a; // resistance a
    double b; // resistance b
    double state;
    double output;

    void setup(double ra, double rb, double srate)
    {
        a = 1.0 / (ra * srate + 1);
        b = 1.0 / (rb * srate + 1);
    }

    double process(double input, bool useAorB)
    {
        state += (useAorB ? a : b) * (input - state);
        output = state;
        return output;
    }

    void reset(double value = 0.0)
    {
        output = state = value;
    }
};

//==============================================================================
/**
*/
class REVERAudioProcessor
    : public AudioProcessor
    , public AudioProcessorParameter::Listener
    , public ChangeBroadcaster
    , private AudioProcessorValueTreeState::Listener
{
public:
    static constexpr int GRID_SIZES[] = {
        4, 8, 16, 32, 64, // Straight
        6, 12, 24, 48,  // Triplet
    };

    // Plugin settings
    float scale = 1.0f; // UI scale factor
    int plugWidth = PLUG_WIDTH;
    int plugHeight = PLUG_HEIGHT;

    // Instance Settings
    int currentProgram = -1;
    bool alwaysPlaying = false;
    bool dualSmooth = true; // use either single smooth or attack and release
    bool dualTension = false;
    int triggerChn = 9; // Midi pattern trigger channel, defaults to channel 10
    bool useMonitor = false;
    bool useSidechain = false;
    bool audioIgnoreHitsWhilePlaying = false;
    int outputCC = 0; // output CC, 0 is off, channel is outputCC - 1
    int outputCCChan = 0; // output CC channel, 0 is channel 1
    int outputATMIDI = 0; // audio trigger midi note output, 0 is off, 60 is C4
    bool bipolarCC = false;
    bool outputCV = false;
    int paintTool = 0; // index of pattern used for paint mode
    int paintPage = 0;
    int pointMode = 1; // Hold, Curve, S-curve, Pulse, Wave etc..
    int linkSeqToGrid = true; // sequencer step linked to grid size

    // State
    Pattern* pattern; // current pattern used for audio processing
    Pattern* sendpattern; // current resonance pattern used for audio processing
    Pattern* viewPattern; // pattern being edited on the view, usually the audio pattern but can also be a paint mode pattern
    Pattern* viewSubPattern; // secondary pattern drawn on view, usually the resonance pattern
    Sequencer* sequencer;
    int queuedPattern = 0; // queued pat index, 0 = off
    int64_t queuedPatternCountdown = 0; // samples counter until queued pattern is applied
    int queuedResPattern = 0; // queued pat index, 0 = off
    int64_t queuedResPatternCountdown = 0; // samples counter until queued pattern is applied
    double xpos = 0.0; // envelope x pos (0..1)
    double ypos = 0.0; // envelope y pos (0..1)
    double yres = 0.0; // resonance envelope y pos
    double trigpos = 0.0; // used by trigger (Audio and MIDI) to detect one one shot envelope play
    double trigposSinceHit = 1.0; // used by audioIgnoreHitsWhilePlaying option
    double trigphase = 0.0; // phase when trigger occurs, used to sync the background wave draw
    double syncQN = 1.0; // sync quarter notes
    int ltrigger = -1; // last trigger mode
    bool midiTrigger = false; // flag midi has triggered envelope
    int winpos = 0;
    int lwinpos = 0;
    double ltension = -10.0;
    double ltensionatk = -10.0;
    double ltensionrel = -10.0;
    double lreverb = 0.0; // last cutoff
    double lsend = 0.0; // last q resonance
    RCSmoother* value; // smooths cutoff envelope value
    RCSmoother* sendvalue; // smooths resonance envelope value
    bool showLatencyWarning = false;

    // Convolver state
    Impulse* impulse;
    std::unique_ptr<StereoConvolver> convolver;
    std::unique_ptr<StereoConvolver> loadConvolver; // convolver used to load IRs and crossfade
    AudioBuffer<float> warmer;
    int loadCooldown = 0;
    int warmwritepos = 0;
    bool init = false;
    bool irDirty = false;
    std::atomic<LoadState> loadState = kIdle;
    int xfade = 0;
    int xfadelen = 0;

    // Reverb State
    bool reverbDirty = false;
    int reverbDirtyCooldown = 0; // DAW param updates are not reliable, when the cutoff is changed internally ignore DAW param updates for a short time
    bool sendDirty = false;
    int sendDirtyCooldown = 0;

    // Audio mode state
    bool audioTrigger = false; // flag audio has triggered envelope
    int audioTriggerCountdown = -1; // samples until audio envelope starts
    std::vector<float> latBufferL; // latency buffer left
    std::vector<float> latBufferR; // latency buffer right
    std::vector<float> monLatBufferL; // latency monitor buffer left
    std::vector<float> monLatBufferR; // latency monitor buffer right
    int latpos = 0; // latency buffer pos
    int monWritePos = 0; // monitor latency pos
    RBJ audioHighcutL{};
    RBJ audioHighcutR{};
    RBJ audioLowcutL{};
    RBJ audioLowcutR{};
    float lastHitAmplitude = 0.0f; // used to display transient hits on monitor view

    // Envelope follower State
    Follower revenv;
    Follower sendenv;
    bool revenvMonitor = false;
    bool sendenvMonitor = false;
    // Envelope follower Settings
    bool revenvSidechain = false;
    bool revenvAutoRel = true;
    bool sendenvSidechain = false;
    bool resenvAutoRel = true;
    std::vector<float> revenvBuffer;
    std::vector<float> sendenvBuffer;

    // PlayHead state
    bool playing = false;
    int64_t timeInSamples = 0;
    double beatPos = 0.0; // position in quarter notes
    double ratePos = 0.0; // position in hertz
    double ppqPosition = 0.0;
    double beatsPerSample = 0.00005;
    double beatsPerSecond = 1.0;
    int samplesPerBeat = 44100;
    double secondsPerBeat = 0.1;

    // UI State
    std::vector<float> preSamples; // used by view to draw pre audio
    std::vector<float> postSamples; // used by view to draw post audio
    int viewW = 1; // viewport width, used for buffers of samples to draw waveforms
    std::atomic<double> xenv = 0.0; // xpos copy using atomic, read by UI thread - attempt to fix rare crash
    std::atomic<double> yenv = 0.0; // ypos copy using atomic, read by UI thread - attempt to fix rare crash
    std::atomic<bool> drawSeek = false;
    std::vector<float> monSamples; // used to draw transients + waveform preview
    std::atomic<float> monpos = 0.0; // write index of monitor circular buf
    int lmonpos = 0; // last index
    int monW = 1; // audio monitor width used to rotate monitor samples buffer
    UIMode uimode = UIMode::Normal; // ui mode
    UIMode luimode = UIMode::Normal; // last ui mode
    bool showAudioKnobs = false; // used by UI to toggle audio knobs
    bool showPaintWidget = false;
    bool showSequencer = false;
    bool sendEditMode = false;
    bool showEnvelopeKnobs = false;

    //==============================================================================
    REVERAudioProcessor();
    ~REVERAudioProcessor() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void loadSettings();
    void saveSettings();
    void setScale(float value);
    int getCurrentGrid();
    int getCurrentSeqStep();
    void createUndoPoint(int patindex = -1);
    void createUndoPointFromSnapshot(std::vector<PPoint> snapshot);
    void setUIMode(UIMode mode);
    void togglePaintEditMode();
    void togglePaintMode();
    void toggleSequencerMode();
    Pattern* getPaintPatern(int index);
    void setViewPattern(int index);
    void restorePaintPatterns();
    void setSendEditMode(bool isSend);

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void parameterValueChanged (int parameterIndex, float newValue) override;
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override;
    bool supportsDoublePrecisionProcessing() const override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    //==============================================================================
    void onSlider ();
    void updatePatternFromReverb();
    void updateSendPatternFromSend();
    void updateReverbFromPattern();
    void updateSendFromPattern();
    void onTensionChange();
    void onPlay ();
    void onStop ();
    void restartEnv (bool fromZero = false);
    void onSmoothChange();
    void clearWaveBuffers();
    void clearLatencyBuffers();
    double getYRev(double x, double min, double max, double offset);
    double getYSend(double x, double min, double max, double offset);
    void queuePattern(int patidx);

    //==============================================================================
    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    void loadProgram(int index);
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //=========================================================

    AudioProcessorValueTreeState params;
    UndoManager undoManager;

private:
    Pattern* patterns[12]; // audio process reverb patterns
    Pattern* sendpatterns[12]; // audio process send patterns
    Pattern* paintPatterns[PAINT_PATS]; // paint mode patterns
    Transient transDetectorL;
    Transient transDetectorR;
    bool paramChanged = false; // flag that triggers on any param change
    ApplicationProperties settings;
    std::vector<MidiInMsg> midiIn; // midi buffer used to process midi messages offset
    std::vector<MidiOutMsg> midiOut;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (REVERAudioProcessor)
};

/*
  ==============================================================================

    View.h
    Author:  tiagolr

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <deque>
#include "../Globals.h"
#include "../dsp/Pattern.h"
#include "Multiselect.h"
#include "PaintTool.h"

class REEVRAudioProcessor;
using namespace globals;

class View : public juce::Component, private juce::Timer
{
public:
    int winx = 0;
    int winy = 0;
    int winw = 0;
    int winh = 0;

    View(REEVRAudioProcessor&);
    ~View() override;
    void resized() override;
    void timerCallback() override;

    void paint(Graphics& g) override;
    void drawWave(Graphics& g, std::vector<float>& samples, Colour color) const;
    void drawGrid(Graphics& g);
    void drawSegments(Graphics& g, bool isMainPattern);
    void drawMidPoints(Graphics& g);
    void drawPoints(Graphics& g);
    void drawSeek(Graphics& g);
    void drawPreSelection(Graphics& g);

    int getPointIndex(uint64_t id);
    PPoint& getPoint(uint64_t id);
    std::vector<double> getMidpointXY(Segment seg);
    uint64_t getHoveredPoint(int x, int y);
    uint64_t getHoveredMidpoint(int x, int y);
    PPoint& getPointFromSegmentIndex(int midpoint);

    // events
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseExit (const MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void insertNewPoint(const MouseEvent& event);

    void showPointContextMenu(const juce::MouseEvent& event);
    void showContextMenu(const MouseEvent& e);

    bool isSnapping(const MouseEvent& e);
    bool isCollinear(Segment seg);
    bool pointInRect(int x, int y, int xx, int yy, int w, int h);

private:
    uint64_t selectedPoint = 0;
    uint64_t selectedMidpoint = 0;
    uint64_t hoverPoint = 0;
    uint64_t hoverMidpoint = 0;
    uint64_t rmousePoint = 0;
    int luimode = false;

    REEVRAudioProcessor& audioProcessor;
    double origTension = 0;
    int dragStartY = 0; // used for midpoint dragging
    uint64_t patternID = 0; // used to detect pattern changes
    std::vector<PPoint> snapshot; // used for undo after drag
    int snapshotIdx = 0; // used for undo after drag
    PPoint dummyPoint{ 0, 0.f, 0.f, 0.f, 1 };

    // Multiselect
    Multiselect multiSelect;
    Point<int> preSelectionStart = Point(-1,-1);
    Point<int> preSelectionEnd = Point(-1,-1);

    // PaintTool
    PaintTool paintTool;

    // Draw seek trail
    std::deque<Point<double>> seekTrail;
};
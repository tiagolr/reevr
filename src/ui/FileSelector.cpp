#include "FileSelector.h"
#include "../PluginProcessor.h"

FileSelector::FileSelector(REVERAudioProcessor& p)
    : audioProcessor(p)
{
}

FileSelector::~FileSelector()
{
}

void FileSelector::paint(juce::Graphics& g) {
    g.setColour(Colour(COLOR_BG));
    g.fillAll();
}

void FileSelector::mouseDown(const juce::MouseEvent& e)
{
    (void)e;
}

void FileSelector::mouseUp(const juce::MouseEvent& e) {
    (void)e;
}

void FileSelector::mouseDrag(const juce::MouseEvent& e) {
    (void)e;
}

void FileSelector::mouseDoubleClick(const juce::MouseEvent& e)
{
    (void)e;
};

void FileSelector::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    (void)event;
    (void)wheel;
}
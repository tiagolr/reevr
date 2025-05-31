#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;
class REVERAudioProcessor;

class FileSelector : public juce::Component, public juce::FileBrowserListener {
public:
    FileSelector(REVERAudioProcessor& p);
    ~FileSelector() override;
    void readDir();

    void selectionChanged() override;
    void fileClicked(const juce::File &file, const juce::MouseEvent &e) override;
    void fileDoubleClicked(const juce::File &file) override;
    void browserRootChanged(const juce::File &newRoot) override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;


private:
    std::unique_ptr<FileTreeComponent> fileTree;
    std::unique_ptr<TimeSliceThread> timeSliceThread;
    std::unique_ptr<FileFilter> fileFilter;
    std::unique_ptr<DirectoryContentsList> dirContents;
    std::unique_ptr<FileChooser> dirPicker;
    TextButton closeButton;
    TextButton changedirButton;

    REVERAudioProcessor& audioProcessor;

};
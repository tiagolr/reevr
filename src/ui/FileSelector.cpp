#include "FileSelector.h"
#include "../PluginProcessor.h"

FileSelector::FileSelector(REEVRAudioProcessor& p, std::function<void()> onClose)
    : audioProcessor(p)
    , timeSliceThread()
    , fileFilter()
    , dirContents()
    , fileTree()
    , onClose(onClose)
{
    auto bounds = getLocalBounds().expanded(-PLUG_PADDING, - PLUG_PADDING);
    auto col = bounds.getX();
    auto row = bounds.getY();
    addAndMakeVisible(closeButton);
    closeButton.setButtonText("Close");
    closeButton.setComponentID("button");
    closeButton.setBounds(col, row, 60, 25);
    closeButton.onClick = [this, onClose]{ onClose(); };
    col += 70;

    addAndMakeVisible(changedirButton);
    changedirButton.setButtonText("Change Dir");
    changedirButton.setComponentID("button");
    changedirButton.setBounds(col, row, 90, 25);
    changedirButton.setToggleState(true, dontSendNotification);
    changedirButton.onClick = [this] {
        dirPicker = std::make_unique<juce::FileChooser>("Select a directory", File(audioProcessor.irDir), "*", true);
        dirPicker->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
            [this](const juce::FileChooser& chooser) {
                juce::File selected = chooser.getResult();
                if (selected.isDirectory()) {
                    audioProcessor.irDir = selected.getFullPathName();
                    audioProcessor.saveSettings();
                    readDir();
                }
            });
    };

    if (!timeSliceThread) {
        timeSliceThread.reset(new juce::TimeSliceThread("IRBrowserThread"));
        timeSliceThread->startThread();
    }

    AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    fileFilter.reset(new juce::WildcardFileFilter(formatManager.getWildcardForAllFormats(), "*", "Audio Files"));
    dirContents.reset(new juce::DirectoryContentsList(fileFilter.get(), *timeSliceThread));

    fileTree.reset(new juce::FileTreeComponent(*dirContents));
    fileTree->addListener(this);
    fileTree->setColour(FileTreeComponent::highlightColourId, Colour(COLOR_ACTIVE).darker(0.5f));
    addAndMakeVisible(*fileTree);

    readDir();
}

FileSelector::~FileSelector()
{
    fileTree = nullptr;
    dirContents = nullptr;
    if (timeSliceThread) {
        timeSliceThread->stopThread(2000);
        timeSliceThread = nullptr;
    }
}

void FileSelector::readDir() 
{
    dirContents->setDirectory(audioProcessor.irDir.isEmpty() ? File() : File(audioProcessor.irDir), true, true);
}

void FileSelector::selectionChanged()
{
    if (!isVisible()) return;
    auto file = fileTree->getSelectedFile();
    auto& path = file.getFullPathName();
    if (path.isNotEmpty() && path != audioProcessor.irFile)
        audioProcessor.loadImpulse(path);
}
void FileSelector::fileClicked(const juce::File &file, const juce::MouseEvent &e)
{
    (void)file;
    (void)e;
}
void FileSelector::fileDoubleClicked(const juce::File &file)
{
    (void)file;
}
void FileSelector::browserRootChanged(const juce::File &newRoot)
{
    (void)newRoot;
}

void FileSelector::paint(juce::Graphics& g) {
    g.setColour(Colour(COLOR_BG));
    g.fillAll();
    g.setColour(Colours::white);
    g.setFont(FontOptions(16.f));

    auto bounds = getLocalBounds().expanded(-PLUG_PADDING, -PLUG_PADDING);

    Rectangle<int> dirbounds = changedirButton.getBounds();
    dirbounds = dirbounds.withLeft(dirbounds.getRight() + 10).withRight(bounds.getRight());
    g.drawFittedText(audioProcessor.irDir, dirbounds, Justification::centredLeft, 2, 1.0f);

    bounds.removeFromTop(35);
}

void FileSelector::resized()
{
    auto bounds = getLocalBounds().expanded(-PLUG_PADDING, -PLUG_PADDING);
    fileTree->setBounds(bounds.withTrimmedTop(35));
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
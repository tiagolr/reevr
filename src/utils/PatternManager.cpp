#include "PatternManager.h"
#include "../dsp/Pattern.h"
#include "../ui/Sequencer.h"
#include "../Globals.h"
#include "../PluginProcessor.h"
#include <sstream>

constexpr int PATTERN_COUNT{ 12 };
void PatternManager::importPatterns(Pattern* patterns[PATTERN_COUNT],Pattern* sendpatterns[PATTERN_COUNT], const TensionParameters& tensionParameters)
{
	mFileChooser.reset(new juce::FileChooser(importWindowTitle, juce::File(), patternExtension));

	mFileChooser->launchAsync(juce::FileBrowserComponent::openMode |
		juce::FileBrowserComponent::canSelectFiles,
		[this, patterns, sendpatterns, tensionParameters](const juce::FileChooser& fc)
		{
			if (fc.getURLResults().size() > 0)
			{
				const auto u = fc.getURLResult();
				auto file = u.getLocalFile();

				if (!file.existsAsFile())
					return;

				juce::String fileContent = file.loadFileAsString();
				std::istringstream iss(fileContent.toStdString());

				for (int i = 0; i < PATTERN_COUNT; ++i)
				{
					double x, y, tension;
					int type;
					std::string line;

					if (!std::getline(iss, line))
						break;

					patterns[i]->clear();
					patterns[i]->clearUndo();

					std::istringstream lineStream(line);
					while (lineStream >> x >> y >> tension >> type)
					{
						patterns[i]->insertPoint(x, y, tension, type, false);
					}
					patterns[i]->setTension(tensionParameters.tension, tensionParameters.tensionAtk, tensionParameters.tensionRel, tensionParameters.dualTension);
					patterns[i]->buildSegments();
				}

				for (int i = 0; i < PATTERN_COUNT; ++i)
				{
					double x, y, tension;
					int type;
					std::string line;

					if (!std::getline(iss, line))
						break;

					sendpatterns[i]->clear();
					sendpatterns[i]->clearUndo();

					std::istringstream lineStream(line);
					while (lineStream >> x >> y >> tension >> type)
					{
						sendpatterns[i]->insertPoint(x, y, tension, type, false);
					}
					sendpatterns[i]->setTension(tensionParameters.tension, tensionParameters.tensionAtk, tensionParameters.tensionRel, tensionParameters.dualTension);
					sendpatterns[i]->buildSegments();
				}

				std::string clearTailsLine;
				if (std::getline(iss, clearTailsLine)) {
					if (String(clearTailsLine).startsWith("CLEARTAILS")) 
					{
						std::string line;

						if (std::getline(iss, line)) 
						{
							int patidx;
							int pointidx;
							std::istringstream lineStream(line);
							while (lineStream >> patidx >> pointidx)
							{
								if (patidx < 12) {
									if (patterns[patidx]->points.size() > pointidx) {
										patterns[patidx]->points[pointidx].clearsTails = true;
										patterns[patidx]->buildSegments();
									}
								}
							}
						}
					}
				}

			}

			mFileChooser = nullptr;
		}, nullptr);
}

void PatternManager::exportPatterns(Pattern* patterns[PATTERN_COUNT], Pattern* sendpatterns[PATTERN_COUNT])
{
	mFileChooser.reset(new juce::FileChooser(exportWindowTitle, juce::File::getSpecialLocation(juce::File::commonDocumentsDirectory), patternExtension));

	mFileChooser->launchAsync(juce::FileBrowserComponent::saveMode |
		juce::FileBrowserComponent::canSelectFiles |
		juce::FileBrowserComponent::warnAboutOverwriting, [this, patterns, sendpatterns](const juce::FileChooser& fc)
		{
			auto file = fc.getResult();
			if (file == juce::File{})
				return;

			std::vector<clearTailsPoint> clearTailsPts;

			std::ostringstream oss;
			for (int i = 0; i < PATTERN_COUNT; ++i)
			{
				auto points = patterns[i]->points;
				int j = 0;
				for (const auto& point : points)
				{
					if (point.clearsTails) {
						clearTailsPts.push_back({ patterns[i]->index, j });
					}
					oss << point.x << " " << point.y << " " << point.tension << " " << point.type << " ";
					j += 1;
				}
				oss << "\n";
			}

			for (int i = 0; i < PATTERN_COUNT; ++i)
			{
				auto points = sendpatterns[i]->points;
				for (const auto& point : points)
				{
					oss << point.x << " " << point.y << " " << point.tension << " " << point.type << " ";
				}
				oss << "\n";
			}
			
			if (!clearTailsPts.empty()) {
				oss << "CLEARTAILS\n";
				for (auto& point : clearTailsPts) {
					oss << point.patindex << " " << point.pointindex << " ";
				}
				oss << "\n";
			}

			if (file.replaceWithText(oss.str()))
			{
				auto options = juce::MessageBoxOptions().withIconType(juce::MessageBoxIconType::InfoIcon)
					.withTitle("Export Successful")
					.withMessage("Patterns exported successfully to:\n" + file.getFullPathName())
					.withButton("OK");
				messageBox = juce::NativeMessageBox::showScopedAsync(options, nullptr);
			}
			else
			{
				auto options = juce::MessageBoxOptions().withIconType(juce::MessageBoxIconType::WarningIcon)
					.withTitle("Export Failed")
					.withMessage("Failed to write pattern file:\n" + file.getFullPathName())
					.withButton("OK");
				messageBox = juce::NativeMessageBox::showScopedAsync(options, nullptr);
			}
		});
}

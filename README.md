<h1 align="center">
  <!-- <img src="doc/logo.png" width="200" style="padding: 5px;" /> -->
  REEV-R
  <br>
</h1>
<div align="center">

[![Windows Support](https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)](https://github.com/tiagolr/reevr/releases)
[![Ubuntu Support](https://img.shields.io/badge/Linux-E95420?style=for-the-badge&logo=linux&logoColor=white)](https://github.com/tiagolr/reevr/releases)
[![Mac Support](https://img.shields.io/badge/MACOS-adb8c5?style=for-the-badge&logo=macos&logoColor=white)](https://github.com/tiagolr/reevr/releases)

</div>
<div align="center">

[![GitHub package.json version](https://img.shields.io/github/v/release/tiagolr/reevr?color=%40&label=latest)](https://github.com/tiagolr/reevr/releases/latest)
![GitHub issues](https://img.shields.io/github/issues-raw/tiagolr/reevr)
![GitHub all releases](https://img.shields.io/github/downloads/tiagolr/reevr/total)
![Github license](https://img.shields.io/github/license/tiagolr/reevr)

</div>

**REEV-R** is a cross-platform convolution reverb modulator where both the pre/send and post reverb signals can be modulated using MSEGs. It's a very close implementation to ShaperBox reverb module, see notes below.

<div align="center">

![Screenshot](./doc/reevr.png)

</div>


Also check other plugins from this collection:

* [GATE-12](https://github.com/tiagolr/gate12) - An envelope generator for volume/gate control.
* [TIME-12](https://github.com/tiagolr/time12) - An envelope based delay modulator.

## Features

  * High performance Convolution Reverb thanks to KlangFalter library
  * IR manipulations like stretch, trim, reverse, attack and decay
  * Pre/send and post/reverb MSEG modulation
  * 12 patterns triggered by midi notes
  * Paint mode with user defined shapes
  * Point type - hold, curve, s-curve, stairs ..
  * Built-in sequencer and randomizer
  * Envelope followers for resonance and cutoff
  * Tempo sync or lfo rate (Hz)
  * Attack and release smooth
  * Pre and post waveform display
  * MIDI trigger mode
  * Audio trigger mode

## Download

* [Download latest release](https://github.com/tiagolr/reevr/releases)
* Current builds include VST3 and LV2 for Windows, Linux and macOS plus AU for macOS.
* Clap is planned when there is official [JUCE support](https://juce.com/blog/juce-roadmap-update-q3-2024/).

## Install

REEV does not come with impulse response files (IRs) other than the default one, you should either install convolution IRs on the plugin impulses folder or change the plugin IR directory to another  folder containing IRs.

Here are some links to download reverb IRs:

* [Bricastis M7](https://samplicity.com/bricasti-m7-impulse-response-files/)
* [Voxengo](https://oramics.github.io/sampled/IR/Voxengo/)
* [Reaper Stash](https://stash.reaper.fm/tag/Reverb-Impulses)
* [Open Air](https://www.openair.hosted.york.ac.uk/?page_id=36)

These links may be broken, search for reverb IR files to find free ones on the internet.


## About

REEV-R is a convolution reverb with modulation capabilities, it is possible thanks to [KlangFalter](https://github.com/HiFi-LoFi/KlangFalter) FFT Convolution library, JUCE stock reverb was consuming 20% CPU usage on debug builds while this one takes less than 1%, this was the only library I found that perfectly fits this plugin.

### How it works

REEV supports IR manipulation like stretching, reversing, trimming, attack and decay. Because loading IRs is expensive this plugin uses two reverbs, one active and one idle, and loads the modified impulses on the idle reverb on a separate thread. Once the secondary reverb is ready it warms it with one second of buffered audio, so that it achieves the same state as the main reverb, finally both reverbs are cross-faded for a smooth transition before swapping the two.

### A note on ShaperBox

This plugin, more so than the others, is feature-by-feature almost a copy of ShaperBox - from the send/reverb modulations to the envelope followers to the knobs that move the average of the points.. this was not intentional or planned, there are not many references and I didn't want to stay behind some very well thought features of CableGuys plugins so I ended replicating them.

If you like this plugin consider checking ShaperBox Reverb, it's probably more studio grade, more polished, the reverb sounds a bit more defined on the low-end and the price tag is still affordable.

### Features

#### Point types

Like GrossBeat, REEV-R includes 8 point types that can be changed by right clicking a point, the default point type can also be changed in the tools-bar.

![curves](/doc/curves.png)

#### Paint mode

Paint mode was remade in version 2, it now works with custom patterns that can be edited like the main audio patterns and rescaled before placing on view.

This mode has to be activated by clicking the `Paint` button, a list of patterns will show above the editor window.

![patterns](/doc/patterns.png)

There are 32 patterns that can be used and edited, these patterns are saved in the settings file and shared by any instances of the plugin.

#### Self repeating patterns

Patterns are not constrained by start and endpoints like they used to, they can have only one point or no points, this mainly allows patterns to be seamlessly rotated to create new rhythm variations.

#### Multi-point selections

Multiple points can be selected and moved around, the selections can also be scaled and skewed by holding `Alt + drag` the leftmost or rightmost handles, this allows to quickly make rising or falling wedges as well as compress or expand sets of points.

![rising-wedge](/doc/rising-wedge.png)

#### Audio Trigger

Audio transients can be used to trigger the envelope or restart its cycle, its heavily based on ShaperBox and includes two modes of detection: *Simple* - based on envelope following and *Drums* - based on total energy during a small time window. This feature is also heavily based on ShaperBox and also allows to use sidechain as input.

#### MIDI Trigger

MIDI can be used to trigger the envelope or restart its cycle, it can also be used to change patterns (defaults to channel 10).

#### Pattern sync

Pattern changes can be synced to the playback beat position, this allows for to make timely transitions in real time in sync with the song position.

#### Sequencer

Clicking the `Seq` button toggles the sequencer mode where patterns can be built and randomized with ease.

![sequencer](/doc/sequencer.png)

The left buttons are the edit mode used when clicking the view. On the right there is a row of shapes used to draw on the view and below is the randomization of the current selected mode.

After creating a sequence click `Apply` to save it as the current pattern and edit from there.

#### Clearing Reverb Tails

Reverb tails can be cleared on any point of the reverb pattern, to do so, `Right Click` a point and select `Clear Tails`.

## Tips

- `Shift` for fine slider adjustments.
- `Shift` toggles snap on/off.
- `Mouse wheel` on view changes grid size
- `Right click` points changes point type.
- `Alt + drag` selection handles skews selected points.
- `Right click + drag` in paint mode changes paint tool tension.
- `Shift + mouse wheel` on sequencer to change step size.

## MacOS

MacOS builds are untested and unsigned, please let me know of any issues by opening a ticket, don't expect a promptly fix as I don't have access to that OS.

Because the builds are unsigned you may have to run the following commands:

```bash
sudo xattr -dr com.apple.quarantine /path/to/your/plugins/reevr.component
sudo xattr -dr com.apple.quarantine /path/to/your/plugins/reevr.vst3
sudo xattr -dr com.apple.quarantine /path/to/your/plugins/reevr.lv3
```

The commands above will recursively remove the quarantine flag from the plugins.

## Build

```bash
git clone --recurse-submodules https://github.com/tiagolr/reevr.git

# windows
cmake -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release -S . -B ./build

# linux
sudo apt update
sudo apt-get install libx11-dev libfreetype-dev libfontconfig1-dev libasound2-dev libxrandr-dev libxinerama-dev libxcursor-dev
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -S . -B ./build
cmake --build ./build --config Release

# macOS
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -S . -B ./build
cmake --build ./build --config Release
```

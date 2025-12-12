# JUCEbox

A JUCE-based MIDI looper synthesizer plugin with 16-voice polyphonic sine wave synthesis, loop recording, and built-in metronome.

## Features

- **16-Voice Polyphonic Synthesizer** - Sine wave synthesis with velocity sensitivity and natural note release
- **MIDI Loop Recording** - Record and playback MIDI patterns in a loop
- **Built-in Metronome** - Accented downbeats to keep time while recording
- **Tempo Control** - Adjustable from 60-200 BPM
- **On-screen Keyboard** - Play notes directly in the plugin UI
- **Visual Feedback** - Loop progress bar and beat/bar indicator

## Building

### Requirements

- [JUCE Framework](https://juce.com/) (7.x recommended)
- Xcode (macOS)

### macOS

1. Open `JUCEbox.jucer` in Projucer
2. Export to Xcode or open `Builds/MacOSX/JUCEbox.xcodeproj`
3. Build the Standalone or AU target

## Usage

1. **Play notes** using the on-screen keyboard or a connected MIDI controller
2. **Set tempo** with the Tempo knob (60-200 BPM)
3. **Enable metronome** (optional) to hear the beat while recording
4. **Press Record/Play** to start recording - play your pattern
5. **Press again** to stop recording - your loop will continue playing
6. **Clear Loop** to start over

## Plugin Formats

- Standalone application
- AU (Audio Unit) for macOS DAWs

## License

MIT

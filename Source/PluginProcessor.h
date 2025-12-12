#pragma once
#include <JuceHeader.h>

class SineWaveVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    double currentAngle = 0.0;
    double angleDelta = 0.0;
    double level = 0.0;
    double tailOff = 0.0;
};

class SineWaveSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

struct RecordedNote
{
    int noteNumber;
    float velocity;
    int64_t startSample;
    int64_t endSample;
};

class JUCEboxAudioProcessor : public juce::AudioProcessor
{
public:
    JUCEboxAudioProcessor();
    ~JUCEboxAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }
    
    // Looper functions
    void toggleRecording();
    void clearLoop();
    bool isRecording() const { return recording; }
    bool isPlaying() const { return loopPlaying; }
    double getLoopPosition() const;
    int getCurrentBeat() const;
    
    // Metronome
    void toggleMetronome();
    bool isMetronomeOn() const { return metronomeOn; }
    
    // Tempo
    void setTempo (double bpm);
    double getTempo() const { return tempo; }

    juce::AudioProcessorValueTreeState apvts;
    
private:
    juce::Synthesiser synth;
    juce::Synthesiser metronomeSynth;
    juce::MidiKeyboardState keyboardState;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Looper state
    bool recording = false;
    bool loopPlaying = false;
    std::vector<RecordedNote> recordedNotes;
    std::map<int, int64_t> activeNoteStarts;
    int64_t loopLengthSamples = 0;
    int64_t loopPositionSamples = 0;
    double sampleRate = 44100.0;
    
    // Metronome state
    bool metronomeOn = false;
    int64_t lastMetronomeBeat = -1;
    
    // Tempo
    double tempo = 120.0;
    int beatsPerBar = 4;
    int numBars = 4;
    
    void processMetronome (juce::MidiBuffer& midiMessages, int numSamples);
    void processLoopPlayback (juce::MidiBuffer& midiMessages, int numSamples);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JUCEboxAudioProcessor)
};

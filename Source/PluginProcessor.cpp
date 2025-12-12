#include "PluginProcessor.h"
#include "PluginEditor.h"

bool SineWaveVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<SineWaveSound*> (sound) != nullptr;
}

void SineWaveVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    currentAngle = 0.0;
    level = velocity * 0.25;
    tailOff = 0.0;
    
    auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    auto cyclesPerSample = cyclesPerSecond / getSampleRate();
    angleDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;
}

void SineWaveVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff)
    {
        if (tailOff == 0.0)
            tailOff = 1.0;
    }
    else
    {
        clearCurrentNote();
        angleDelta = 0.0;
    }
}

void SineWaveVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (angleDelta != 0.0)
    {
        while (--numSamples >= 0)
        {
            auto currentSample = (float) (std::sin (currentAngle) * level);
            
            if (tailOff > 0.0)
            {
                currentSample *= (float) tailOff;
                tailOff *= 0.9995;
                
                if (tailOff <= 0.005)
                {
                    clearCurrentNote();
                    angleDelta = 0.0;
                    break;
                }
            }
            
            for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                outputBuffer.addSample (i, startSample, currentSample);
            
            currentAngle += angleDelta;
            ++startSample;
        }
    }
}

JUCEboxAudioProcessor::JUCEboxAudioProcessor()
     : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    for (auto i = 0; i < 16; ++i)
        synth.addVoice (new SineWaveVoice());
    synth.addSound (new SineWaveSound());
    
    for (auto i = 0; i < 2; ++i)
        metronomeSynth.addVoice (new SineWaveVoice());
    metronomeSynth.addSound (new SineWaveSound());
}

JUCEboxAudioProcessor::~JUCEboxAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout JUCEboxAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "GAIN", 1 }, "Gain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
    return { params.begin(), params.end() };
}

const juce::String JUCEboxAudioProcessor::getName() const { return JucePlugin_Name; }
bool JUCEboxAudioProcessor::acceptsMidi() const { return true; }
bool JUCEboxAudioProcessor::producesMidi() const { return false; }
bool JUCEboxAudioProcessor::isMidiEffect() const { return false; }
double JUCEboxAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int JUCEboxAudioProcessor::getNumPrograms() { return 1; }
int JUCEboxAudioProcessor::getCurrentProgram() { return 0; }
void JUCEboxAudioProcessor::setCurrentProgram (int) {}
const juce::String JUCEboxAudioProcessor::getProgramName (int) { return {}; }
void JUCEboxAudioProcessor::changeProgramName (int, const juce::String&) {}

void JUCEboxAudioProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    synth.setCurrentPlaybackSampleRate (sr);
    metronomeSynth.setCurrentPlaybackSampleRate (sr);
    
    double secondsPerBeat = 60.0 / tempo;
    loopLengthSamples = (int64_t)(secondsPerBeat * beatsPerBar * numBars * sampleRate);
}

void JUCEboxAudioProcessor::releaseResources() {}

bool JUCEboxAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void JUCEboxAudioProcessor::setTempo (double bpm)
{
    tempo = bpm;
    double secondsPerBeat = 60.0 / tempo;
    loopLengthSamples = (int64_t)(secondsPerBeat * beatsPerBar * numBars * sampleRate);
}

void JUCEboxAudioProcessor::toggleRecording()
{
    if (!recording && !loopPlaying)
    {
        recording = true;
        loopPlaying = true;
        loopPositionSamples = 0;
        lastMetronomeBeat = -1;
    }
    else if (recording)
    {
        recording = false;
    }
    else
    {
        loopPlaying = !loopPlaying;
        if (loopPlaying)
        {
            loopPositionSamples = 0;
            lastMetronomeBeat = -1;
        }
    }
}

void JUCEboxAudioProcessor::clearLoop()
{
    recording = false;
    loopPlaying = false;
    recordedNotes.clear();
    activeNoteStarts.clear();
    loopPositionSamples = 0;
    lastMetronomeBeat = -1;
}

void JUCEboxAudioProcessor::toggleMetronome()
{
    metronomeOn = !metronomeOn;
}

double JUCEboxAudioProcessor::getLoopPosition() const
{
    if (loopLengthSamples == 0) return 0.0;
    return (double)loopPositionSamples / (double)loopLengthSamples;
}

int JUCEboxAudioProcessor::getCurrentBeat() const
{
    if (!loopPlaying) return -1;
    double secondsPerBeat = 60.0 / tempo;
    int64_t samplesPerBeat = (int64_t)(secondsPerBeat * sampleRate);
    if (samplesPerBeat == 0) return 0;
    return (int)(loopPositionSamples / samplesPerBeat) % (beatsPerBar * numBars);
}

void JUCEboxAudioProcessor::processMetronome (juce::MidiBuffer& midiMessages, int numSamples)
{
    if (!metronomeOn || !loopPlaying) return;
    
    double secondsPerBeat = 60.0 / tempo;
    int64_t samplesPerBeat = (int64_t)(secondsPerBeat * sampleRate);
    
    for (int i = 0; i < numSamples; ++i)
    {
        int64_t currentSample = loopPositionSamples + i;
        int64_t currentBeat = currentSample / samplesPerBeat;
        
        if (currentBeat != lastMetronomeBeat)
        {
            lastMetronomeBeat = currentBeat;
            int beatInBar = (int)(currentBeat % beatsPerBar);
            int noteNumber = (beatInBar == 0) ? 84 : 72;
            
            midiMessages.addEvent (juce::MidiMessage::noteOn (10, noteNumber, 0.7f), i);
            midiMessages.addEvent (juce::MidiMessage::noteOff (10, noteNumber), i + 1000);
        }
    }
}

void JUCEboxAudioProcessor::processLoopPlayback (juce::MidiBuffer& midiMessages, int numSamples)
{
    if (!loopPlaying || recordedNotes.empty()) return;
    
    int64_t blockStart = loopPositionSamples;
    int64_t blockEnd = loopPositionSamples + numSamples;
    
    for (const auto& note : recordedNotes)
    {
        if (note.startSample >= blockStart && note.startSample < blockEnd)
        {
            int offset = (int)(note.startSample - blockStart);
            midiMessages.addEvent (juce::MidiMessage::noteOn (1, note.noteNumber, note.velocity), offset);
        }
        
        if (note.endSample >= blockStart && note.endSample < blockEnd)
        {
            int offset = (int)(note.endSample - blockStart);
            midiMessages.addEvent (juce::MidiMessage::noteOff (1, note.noteNumber), offset);
        }
    }
}

void JUCEboxAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);
    
    if (recording)
    {
        for (const auto metadata : midiMessages)
        {
            auto msg = metadata.getMessage();
            int samplePos = metadata.samplePosition;
            int64_t absoluteSample = (loopPositionSamples + samplePos) % loopLengthSamples;
            
            if (msg.isNoteOn())
            {
                activeNoteStarts[msg.getNoteNumber()] = absoluteSample;
            }
            else if (msg.isNoteOff())
            {
                auto it = activeNoteStarts.find (msg.getNoteNumber());
                if (it != activeNoteStarts.end())
                {
                    RecordedNote note;
                    note.noteNumber = msg.getNoteNumber();
                    note.velocity = 0.8f;
                    note.startSample = it->second;
                    note.endSample = absoluteSample;
                    recordedNotes.push_back (note);
                    activeNoteStarts.erase (it);
                }
            }
        }
    }
    
    processLoopPlayback (midiMessages, buffer.getNumSamples());
    
    juce::MidiBuffer metronomeMidi;
    processMetronome (metronomeMidi, buffer.getNumSamples());
    
    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    
    juce::AudioBuffer<float> metronomeBuffer (buffer.getNumChannels(), buffer.getNumSamples());
    metronomeBuffer.clear();
    metronomeSynth.renderNextBlock (metronomeBuffer, metronomeMidi, 0, buffer.getNumSamples());
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.addFrom (ch, 0, metronomeBuffer, ch, 0, buffer.getNumSamples(), 0.3f);
    
    if (loopPlaying)
    {
        loopPositionSamples += buffer.getNumSamples();
        if (loopPositionSamples >= loopLengthSamples)
            loopPositionSamples = 0;
    }
    
    auto gain = apvts.getRawParameterValue ("GAIN")->load();
    buffer.applyGain (gain);
}

bool JUCEboxAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* JUCEboxAudioProcessor::createEditor() { return new JUCEboxAudioProcessorEditor (*this); }
void JUCEboxAudioProcessor::getStateInformation (juce::MemoryBlock&) {}
void JUCEboxAudioProcessor::setStateInformation (const void*, int) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new JUCEboxAudioProcessor(); }

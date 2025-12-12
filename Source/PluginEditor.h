#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        setDefaultSansSerifTypefaceName ("Inter");
    }
    
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
    {
        return juce::Font ("Inter", juce::jmin (16.0f, (float) buttonHeight * 0.6f), juce::Font::bold);
    }
    
    juce::Font getLabelFont (juce::Label&) override
    {
        return juce::Font ("Inter", 14.0f, juce::Font::bold);
    }
};

class JUCEboxAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    JUCEboxAudioProcessorEditor (JUCEboxAudioProcessor&);
    ~JUCEboxAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    JUCEboxAudioProcessor& audioProcessor;
    
    CustomLookAndFeel customLookAndFeel;
    
    juce::Slider gainSlider;
    juce::Label titleLabel;
    juce::Label gainLabel;
    
    juce::MidiKeyboardComponent keyboardComponent;
    
    juce::TextButton recordButton;
    juce::TextButton clearButton;
    juce::TextButton metronomeButton;
    juce::Label tempoLabel;
    juce::Slider tempoSlider;
    juce::Label beatLabel;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JUCEboxAudioProcessorEditor)
};

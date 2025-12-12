#include "PluginProcessor.h"
#include "PluginEditor.h"

JUCEboxAudioProcessorEditor::JUCEboxAudioProcessorEditor (JUCEboxAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      keyboardComponent (p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setSize (700, 500);
    
    // Apply custom look and feel to entire editor
    setLookAndFeel (&customLookAndFeel);
    
    // Title
    titleLabel.setText ("JUCEbox Synth", juce::dontSendNotification);
    titleLabel.setFont (juce::Font ("Inter", 32.0f, juce::Font::bold));
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::cyan);
    addAndMakeVisible (titleLabel);
    
    // Gain Slider
    gainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    gainSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::cyan);
    gainSlider.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible (gainSlider);
    
    gainLabel.setText ("Gain", juce::dontSendNotification);
    gainLabel.setFont (juce::Font ("Inter", 14.0f, juce::Font::bold));
    gainLabel.setJustificationType (juce::Justification::centred);
    gainLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (gainLabel);
    
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "GAIN", gainSlider);
    
    // Record Button
    recordButton.setButtonText ("Record / Play");
    recordButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2d4a3e));
    recordButton.onClick = [this] { audioProcessor.toggleRecording(); };
    addAndMakeVisible (recordButton);
    
    // Clear Button
    clearButton.setButtonText ("Clear Loop");
    clearButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff4a2d2d));
    clearButton.onClick = [this] { audioProcessor.clearLoop(); };
    addAndMakeVisible (clearButton);
    
    // Metronome Button
    metronomeButton.setButtonText ("Metronome: OFF");
    metronomeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff3d3d4a));
    metronomeButton.onClick = [this] { 
        audioProcessor.toggleMetronome();
        metronomeButton.setButtonText (audioProcessor.isMetronomeOn() ? "Metronome: ON" : "Metronome: OFF");
        metronomeButton.setColour (juce::TextButton::buttonColourId, 
            audioProcessor.isMetronomeOn() ? juce::Colour (0xff4a4a2d) : juce::Colour (0xff3d3d4a));
    };
    addAndMakeVisible (metronomeButton);
    
    // Tempo Slider
    tempoLabel.setText ("Tempo", juce::dontSendNotification);
    tempoLabel.setFont (juce::Font ("Inter", 14.0f, juce::Font::bold));
    tempoLabel.setJustificationType (juce::Justification::centred);
    tempoLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (tempoLabel);
    
    tempoSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    tempoSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    tempoSlider.setRange (60.0, 200.0, 1.0);
    tempoSlider.setValue (120.0);
    tempoSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::orange);
    tempoSlider.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    tempoSlider.onValueChange = [this] { audioProcessor.setTempo (tempoSlider.getValue()); };
    addAndMakeVisible (tempoSlider);
    
    // Beat indicator label
    beatLabel.setText ("Beat: -", juce::dontSendNotification);
    beatLabel.setFont (juce::Font ("Inter", 18.0f, juce::Font::bold));
    beatLabel.setJustificationType (juce::Justification::centred);
    beatLabel.setColour (juce::Label::textColourId, juce::Colours::yellow);
    addAndMakeVisible (beatLabel);
    
    // Keyboard
    keyboardComponent.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colours::cyan);
    keyboardComponent.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colours::cyan.withAlpha (0.3f));
    addAndMakeVisible (keyboardComponent);
    
    startTimerHz (30);
}

JUCEboxAudioProcessorEditor::~JUCEboxAudioProcessorEditor() 
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void JUCEboxAudioProcessorEditor::timerCallback()
{
    // Update record button appearance
    if (audioProcessor.isRecording())
    {
        recordButton.setButtonText ("Recording...");
        recordButton.setColour (juce::TextButton::buttonColourId, juce::Colours::red);
    }
    else if (audioProcessor.isPlaying())
    {
        recordButton.setButtonText ("Playing (click to stop)");
        recordButton.setColour (juce::TextButton::buttonColourId, juce::Colours::green);
    }
    else
    {
        recordButton.setButtonText ("Record / Play");
        recordButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2d4a3e));
    }
    
    // Update beat indicator
    int beat = audioProcessor.getCurrentBeat();
    if (beat >= 0)
    {
        int bar = (beat / 4) + 1;
        int beatInBar = (beat % 4) + 1;
        beatLabel.setText ("Bar " + juce::String(bar) + " - Beat " + juce::String(beatInBar), juce::dontSendNotification);
        
        if (beatInBar == 1)
            beatLabel.setColour (juce::Label::textColourId, juce::Colours::orange);
        else
            beatLabel.setColour (juce::Label::textColourId, juce::Colours::yellow);
    }
    else
    {
        beatLabel.setText ("Beat: -", juce::dontSendNotification);
        beatLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    }
    
    repaint();
}

void JUCEboxAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient gradient (juce::Colour (0xff16213e), 0, 0,
                                    juce::Colour (0xff0f3460), 0, (float) getHeight(), false);
    g.setGradientFill (gradient);
    g.fillRect (getLocalBounds());
    
    // Draw loop progress bar
    if (audioProcessor.isPlaying())
    {
        g.setColour (juce::Colour (0xff2a2a4a));
        g.fillRoundedRectangle (20.0f, 320.0f, getWidth() - 40.0f, 20.0f, 5.0f);
        
        g.setColour (juce::Colours::cyan);
        float progress = (float) audioProcessor.getLoopPosition();
        g.fillRoundedRectangle (20.0f, 320.0f, (getWidth() - 40.0f) * progress, 20.0f, 5.0f);
    }
}

void JUCEboxAudioProcessorEditor::resized()
{
    titleLabel.setBounds (0, 10, getWidth(), 40);
    
    // Left side - Gain
    gainSlider.setBounds (50, 70, 100, 100);
    gainLabel.setBounds (50, 170, 100, 25);
    
    // Center - Looper controls
    recordButton.setBounds (180, 80, 120, 40);
    clearButton.setBounds (180, 130, 120, 40);
    beatLabel.setBounds (180, 180, 120, 30);
    
    // Right side - Tempo & Metronome
    tempoSlider.setBounds (350, 70, 100, 100);
    tempoLabel.setBounds (350, 170, 100, 25);
    metronomeButton.setBounds (480, 100, 140, 40);
    
    // Keyboard at bottom
    keyboardComponent.setBounds (10, 360, getWidth() - 20, 120);
}

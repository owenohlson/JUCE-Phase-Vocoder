#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PhaseVocoderAudioProcessorEditor::PhaseVocoderAudioProcessorEditor (PhaseVocoderAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    auto& apvts = processorRef.apvts;

    // Mode selector
    modeSelector.addItem("Pitch Shift", 1);
    modeSelector.addItem("Robotize", 2);
    modeSelector.addItem("Whisperize", 3);
    modeSelector.setSelectedId(1); // default mode
    addAndMakeVisible(modeSelector);

    modeLabel.setText("Mode", juce::dontSendNotification);
    modeLabel.attachToComponent(&modeSelector, true); // attach to left
    addAndMakeVisible(modeLabel);

    // Connect to APVTS for parameter automation
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts,
        "MODE",
        modeSelector
    );

    // Reactive behavior when user changes mode
    modeSelector.onChange = [this]() { updateModeUI(); };

    // Call it once to set initial visibility
    updateModeUI();

    // Pitch shift slider
    pitchShiftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts,              // Reference to the APVTS member
        "PITCH_SHIFT",                // Parameter ID string
        pitchShiftSlider              // The UI component to connect
    );

    pitchShiftSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    pitchShiftSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 20);
    addAndMakeVisible(pitchShiftSlider);

    pitchShiftLabel.setText("Pitch Shift (semitones)", juce::dontSendNotification);
    pitchShiftLabel.setJustificationType(juce::Justification::centred);
    pitchShiftLabel.attachToComponent(&pitchShiftSlider, false);
    addAndMakeVisible(pitchShiftLabel);

    fftSizeComboBox.addItem("1024", 1);
    fftSizeComboBox.addItem("2048", 2);
    fftSizeComboBox.addItem("4096", 3);
    

    // FFT size combo box
    fftSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts,
        "FFT_SIZE",                   // Parameter ID string
        fftSizeComboBox               // The UI component to connect
    );
    
    fftSizeComboBox.setTextWhenNoChoicesAvailable("No Sizes Available!");
    addAndMakeVisible(fftSizeComboBox);

    fftSizeLabel.setText("FFT Size", juce::dontSendNotification);
    fftSizeLabel.attachToComponent(&fftSizeComboBox, true); // Attach to the left
    addAndMakeVisible(fftSizeLabel);

    setSize (400, 300);
}

PhaseVocoderAudioProcessorEditor::~PhaseVocoderAudioProcessorEditor()
{
}

//==============================================================================
void PhaseVocoderAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Phase Vocoder", getLocalBounds(), juce::Justification::topLeft, 1);
}

void PhaseVocoderAudioProcessorEditor::resized()
{
    int margin = 20;
    int controlWidth = (getWidth() - 3 * margin) / 2;
    int controlHeight = 150;
    int comboHeight = 25;

    if (pitchShiftSlider.isVisible())
    {
        pitchShiftSlider.setBounds(
            margin, 
            margin + 20, 
            controlWidth, 
            controlHeight - 20
        );
    }

    fftSizeComboBox.setBounds(
        margin * 2 + controlWidth, 
        margin + 20, 
        controlWidth, 
        comboHeight
    );

    modeSelector.setBounds(
        margin * 2 + controlWidth, 
        margin + 60, 
        controlWidth, 
        comboHeight
    );
}

void PhaseVocoderAudioProcessorEditor::updateModeUI()
{
    int selectedId = modeSelector.getSelectedId();

    pitchShiftSlider.setVisible(selectedId == 1);
    pitchShiftLabel.setVisible(selectedId == 1);

    resized();
}

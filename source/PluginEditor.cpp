#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PhaseVocoderAudioProcessorEditor::PhaseVocoderAudioProcessorEditor (PhaseVocoderAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    auto& apvts = processorRef.apvts;

    // Pitch shift ratio slider
    pitchShiftRatioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts,              // Reference to the APVTS member
        "PITCH_SHIFT_RATIO",                // Parameter ID string
        pitchShiftRatioSlider              // The UI component to connect
    );

    pitchShiftRatioSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    pitchShiftRatioSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 20);
    addAndMakeVisible(pitchShiftRatioSlider);

    pitchShiftRatioLabel.setText("Pitch Shift Ratio", juce::dontSendNotification);
    pitchShiftRatioLabel.setJustificationType(juce::Justification::centred);
    pitchShiftRatioLabel.attachToComponent(&pitchShiftRatioSlider, false);
    addAndMakeVisible(pitchShiftRatioLabel);

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
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Phase Vocoder", getLocalBounds(), juce::Justification::topLeft, 1);
}

void PhaseVocoderAudioProcessorEditor::resized()
{
    // Define bounds for the UI components
    int margin = 20;
    int controlWidth = (getWidth() - 3 * margin) / 2;
    int controlHeight = 150;
    int comboHeight = 25;

    // Pitch shift ratio Slider
    pitchShiftRatioSlider.setBounds(
        margin, 
        margin + 20, 
        controlWidth, 
        controlHeight - 20
    );

    // FFT Size ComboBox
    fftSizeComboBox.setBounds(
        margin * 2 + controlWidth, 
        margin + 20, 
        controlWidth, 
        comboHeight
    );
}

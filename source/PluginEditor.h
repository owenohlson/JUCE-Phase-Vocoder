#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class PhaseVocoderAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PhaseVocoderAudioProcessorEditor (PhaseVocoderAudioProcessor&);
    ~PhaseVocoderAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void updateModeUI();

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PhaseVocoderAudioProcessor& processorRef;

    juce::ComboBox modeSelector;
    juce::Slider pitchShiftRatioSlider;
    juce::ComboBox fftSizeComboBox;

    juce::Label pitchShiftRatioLabel, fftSizeLabel, modeLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchShiftRatioAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> fftSizeAttachment, modeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaseVocoderAudioProcessorEditor)
};

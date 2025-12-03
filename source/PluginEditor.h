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
    PhaseVocoderAudioProcessor& processorRef;

    juce::ComboBox modeSelector;
    juce::Slider pitchShiftSlider;
    juce::ComboBox fftSizeComboBox;

    juce::Label pitchShiftLabel, fftSizeLabel, modeLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchShiftAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> fftSizeAttachment, modeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaseVocoderAudioProcessorEditor)
};

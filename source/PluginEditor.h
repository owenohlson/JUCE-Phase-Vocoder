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

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PhaseVocoderAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaseVocoderAudioProcessorEditor)
};

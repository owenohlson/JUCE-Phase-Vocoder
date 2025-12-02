#pragma once

#include <JuceHeader.h>
#include "PhaseVocoder.h"

class PhaseVocoder;

//=================================================================================
class VocoderBuilderThread : public juce::Thread
{
public:
    VocoderBuilderThread(std::function<void()> taskToRun)
        : juce::Thread("Vocoder Builder Thread"), task(taskToRun) {}

    void run() override
    {
        while (! threadShouldExit())
        {
            wait(-1); // Wait until signaled
            if (threadShouldExit()) break;

            if (task) task(); // Build vocoder
        }
    }

private:
    std::function<void()> task;
};

//==============================================================================
class PhaseVocoderAudioProcessor final : public juce::AudioProcessor,
                                         public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    PhaseVocoderAudioProcessor();
    ~PhaseVocoderAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRateIn, int samplesPerBlockIn) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================

    juce::UndoManager undoManager;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts {*this, &undoManager, "Parameters", createParameterLayout()};

    std::atomic<bool> fftResizePending { false };
    int newFFTSize;

    void parameterChanged(const String& parameterID, float newValue) override
    {
        if (parameterID == "FFT_SIZE")
        {
            fftResizePending = true;

            auto* p = dynamic_cast<AudioParameterChoice*>(apvts.getParameter("FFT_SIZE"));
            newFFTSize = p->getCurrentChoiceName().getIntValue();
            DBG("Parameter " << parameterID << " changed to " << newFFTSize);
        }
    }

    //==============================================================================
    std::unique_ptr<PhaseVocoder> engine;

private:
    double sampleRate;
    int samplesPerBlock;
    int numChannels;
    int N;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaseVocoderAudioProcessor)
};

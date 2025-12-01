#pragma once
#include <JuceHeader.h>

class PhaseVocoder
{
public:
    PhaseVocoder(int fftSizeIn, double sampleRateIn, int numChannelsIn);
    void prepare(int fftSizeIn, double sampleRateIn, int numChannelsIn);
    void process(juce::AudioBuffer<float>& buffer);

    int N = 2048; // FFT size
    juce::SmoothedValue<float> pitchShiftRatioSmoothed = 1.0f;
    float lastpitchShiftRatio = pitchShiftRatioSmoothed.getCurrentValue();
    
private:
    // === CONFIG === //
    double sampleRate = 44100.0;
    int analysisHopSize = N / 4;
    int synthesisHopSize;

    int numChannels;
    
    // === FFT + WINDOWS === //
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window;
    std::vector<float> dataTemp;
    std::vector<float> tempResampled;
    std::vector<float> centerFreqs;
    float normFactor;
    float sumSquared;
    
    // === CIRCULAR BUFFERS === //
    juce::AudioBuffer<float> inputCircBuff;
    juce::AudioBuffer<float> outputCircBuff;

    int inputWritePos, 
        inputReadPos, 
        outputWritePos, 
        outputReadPos, 
        samplesAccumulated = 0;

    // === PHASE ARRAYS === //
    std::vector<std::vector<float>> phasePrev;
    std::vector<std::vector<float>> magPrev;
    std::vector<std::vector<float>> deltaPhase;
    std::vector<std::vector<float>> synthesisPhase;
    
    // === HELPERS === //
    float princArg(float x) {return std::fmod(x + pi, 2 * pi) - pi; }
    
    // === CONSTANTS === //
    const float pi = juce::MathConstants<float>::pi;
};

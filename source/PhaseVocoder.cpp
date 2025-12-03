#include "PhaseVocoder.h"

PhaseVocoder::PhaseVocoder(int fftSizeIn, double sampleRateIn, int numChannelsIn)
{
    N = fftSizeIn;
    sampleRate = sampleRateIn;
    numChannels = numChannelsIn;
    prepare(N, sampleRate, numChannels);
}

void PhaseVocoder::prepare(int fftSizeIn, double sampleRateIn, int numChannelsIn)
{
    inputCircBuff.clear();
    outputCircBuff.clear();

    N = fftSizeIn;
    sampleRate = sampleRateIn;
    numChannels = numChannelsIn;

    float smoothPSR = pitchShiftRatioSmoothed.getCurrentValue();

    analysisHopSize  = N / 4;
    synthesisHopSize = int(analysisHopSize * smoothPSR);

    pitchShiftRatioSmoothed.reset(sampleRate, 0.0005f);

    int minBufferSize = 2 * N + analysisHopSize * 4 * 2; // extra *2 is from max(synthesisHopSize, analysisHopSize) = 2* analysisHopSize

    int fftOrder = (int) std::round(std::log2(N));
    fft = std::make_unique<juce::dsp::FFT>(fftOrder);

    // Resize arrays
    window.resize(N);
    synthWindow.resize(N);

    centerFreqs.resize(N/2 + 1);

    analysisFrame.assign(numChannels, std::vector<float>(2 * N));
    phasePrev.assign(numChannels, std::vector<float>(N/2 + 1));
    magPrev.assign(numChannels, std::vector<float>(N/2 + 1));
    mag.assign(numChannels, std::vector<float>(N/2 + 1));
    phase.assign(numChannels, std::vector<float>(N/2 + 1));
    synthesisSpectrum.assign(numChannels, std::vector<float>(N/2 + 1));
    deltaPhase.assign(numChannels, std::vector<float>(N/2 + 1));
    synthesisPhase.assign(numChannels, std::vector<float>(N/2 + 1));

    for (int ch = 0; ch < numChannels; ++ch)
    {
        std::fill(analysisFrame[ch].begin(), analysisFrame[ch].end(), 0.0f);
        std::fill(synthesisPhase[ch].begin(), synthesisPhase[ch].end(), 0.0f);
        std::fill(phasePrev[ch].begin(),      phasePrev[ch].end(),      0.0f);
        std::fill(deltaPhase[ch].begin(),     deltaPhase[ch].end(),     0.0f);
        std::fill(magPrev[ch].begin(),        magPrev[ch].end(),        0.0f);
    }

    inputCircBuff.setSize(numChannels, minBufferSize);
    outputCircBuff.setSize(numChannels, minBufferSize);

    inputWritePos = inputReadPos = 0;
    outputWritePos = 0;
    outputReadPos = 2 * N;
    samplesAccumulated = 0;

    // Hann window
    // Add choice of window function?
    sumSquared = 0.0f;
    for (int n = 0; n < N; ++n)
    {
        float win = sqrt(0.5f * (1.0f - std::cos(2.0f * pi * n / (N - 1))));
        window[n] = win;
        sumSquared += win * win;
    }

    float normFactor = 1.0f / (sumSquared / synthesisHopSize);

    // for (int n = 0; n < N; ++n)
    //     synthWindow[n] = window[n] * normFactor;

    for (int k = 0; k <= N/2; ++k)
        centerFreqs[k] = (2.0f * pi * k) / N; // in rad/sample
}

void PhaseVocoder::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int circSize   = inputCircBuff.getNumSamples();
    const int channels   = buffer.getNumChannels();

    // DBG("inputWritePos = " << inputWritePos <<
    //     ",  inputReadPos = " << inputReadPos <<
    //     ",  outputWritePos = " << outputWritePos <<
    //     ",  outputReadPos = " << outputReadPos);

    float smoothPSR = pitchShiftRatioSmoothed.getCurrentValue();

    // DBG("SmoothPSR = " << smoothPSR);
    // float smoothPSR = pitchShiftRatioSmoothed.getNextValue();
    synthesisHopSize = int(analysisHopSize * smoothPSR);
    // normFactor = analysisHopSize / sumSquared;
    normFactor = 1.0f / (sumSquared / synthesisHopSize);

    // Write input into circular buffer
    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < channels; ++ch)
            inputCircBuff.setSample(ch, inputWritePos, buffer.getSample(ch, i));

        inputWritePos = (inputWritePos + 1) % circSize;
        samplesAccumulated++;
    }

    // Do vocoder analysis/synthesis whenever enough samples accumulated
    while (samplesAccumulated >= analysisHopSize)
    {
        smoothPSR = pitchShiftRatioSmoothed.getNextValue();

        for (int ch = 0; ch < channels; ++ch)
        {
            // Load frame
            for (int n = 0; n < N; ++n)
            {
                int pos = (inputReadPos + n) % circSize;
                analysisFrame[ch][n] = inputCircBuff.getSample(ch, pos) * window[n];
            }

            std::fill(analysisFrame[ch].begin() + N, analysisFrame[ch].end(), 0.0f);

            // 180 degree cyclic shift
            // std::rotate(analysisFrame[ch].begin(), analysisFrame[ch].begin() + N/2, analysisFrame[ch].begin() + N);

            // FFT
            fft->performRealOnlyForwardTransform(analysisFrame[ch].data());

            // Process bins
            for (int k = 0; k <= N/2; ++k)
            {
                float real = analysisFrame[ch][2*k];
                float imag = analysisFrame[ch][2*k + 1];

                float mag   = std::sqrt(real*real + imag*imag);
                float phase = std::atan2(imag, real);
                
                float omega = centerFreqs[k];
                float targetPhase = phasePrev[ch][k] + analysisHopSize * omega;
                
                float deviation = phase - targetPhase;
                float deltaPhi = omega * analysisHopSize + princArg(deviation);
                
                // deltaPhase[ch][k] = deltaPhi;
                
                switch (currentMode)
                {
                    case VocoderMode::PitchShift:
                        synthesisPhase[ch][k] = princArg(synthesisPhase[ch][k] + deltaPhi * smoothPSR);
                        break;

                    case VocoderMode::Robotize:
                        synthesisPhase[ch][k] = 0.0f;
                        break;

                    case VocoderMode::Whisperize:
                        synthesisPhase[ch][k] = juce::Random::getSystemRandom().nextFloat() * 2 * pi;
                        break;

                    default:
                        synthesisPhase[ch][k] = princArg(synthesisPhase[ch][k] + deltaPhi * smoothPSR);
                        break;
                }
                // float instFreq = deltaPhi / analysisHopSize;
                
                // Comment these out for no-op
                if (k == 0 || k == N/2)
                {
                    // DC and Nyquist: keep real, imaginary should be zero
                    analysisFrame[ch][2*k] = mag;
                    analysisFrame[ch][2*k + 1] = 0.0f;
                }
                else
                {
                    analysisFrame[ch][2*k] = mag * std::cos(synthesisPhase[ch][k]);
                    analysisFrame[ch][2*k + 1] = mag * std::sin(synthesisPhase[ch][k]);
                }

                phasePrev[ch][k] = phase;
            }

            // IFFT
            fft->performRealOnlyInverseTransform(analysisFrame[ch].data());

            // Undo cyclic shift
            // std::rotate(analysisFrame[ch].begin(), analysisFrame[ch].begin() + N/2, analysisFrame[ch].begin() + N);

            juce::FloatVectorOperations::multiply (analysisFrame[ch].data(), window.data(), N);

            if (currentMode == VocoderMode::PitchShift /*&& smoothPSR - 1.0f > 0.001*/)
            {
                // Resample to match original duration
                double outputLength = floor(N / smoothPSR);
                if ((int)tempResampled.size() < outputLength)
                    tempResampled.resize(outputLength);

                for (int n = 0; n < outputLength; ++n)
                {
                    double x = double(n) * N / outputLength;
                    int ix = (int)std::floor(x);
                    float dx = float(x - ix);

                    float s0 = analysisFrame[ch][ix];
                    float s1 = analysisFrame[ch][(ix + 1) % N];

                    tempResampled[n] = s0 + dx * (s1 - s0);
                }

                // Pitch shift OLA
                for (int n = 0; n < outputLength; ++n)
                {
                    int pos = (outputWritePos + n) % circSize;
                    float prev = outputCircBuff.getSample(ch, pos);

                    // Window applied using n mapped to Hann domain
                    // Must index window proportionally
                    int winIndex = int((double)n / outputLength * N);
                    winIndex = juce::jlimit(0, N-1, winIndex);
                    outputCircBuff.setSample(ch, pos, prev + tempResampled[n] * normFactor);
                }
            }
            else
            {
                // Regular OLA for other modes
                for (int n = 0; n < N; ++n)
                {
                    int pos = (outputWritePos + n) % circSize;
                    float prev = outputCircBuff.getSample(ch, pos);
                    outputCircBuff.setSample(ch, pos, prev + analysisFrame[ch][n] * normFactor);
                }
            }
        }

        inputReadPos  = (inputReadPos  + analysisHopSize)  % circSize;
        outputWritePos = (outputWritePos + analysisHopSize) % circSize;
        samplesAccumulated -= analysisHopSize;
    }

    // Output ready samples
    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < channels; ++ch)
        {
            float v = outputCircBuff.getSample(ch, outputReadPos);
            buffer.setSample(ch, i, v);
            outputCircBuff.setSample(ch, outputReadPos, 0.0f);
        }

        outputReadPos = (outputReadPos + 1) % circSize;
    }
}

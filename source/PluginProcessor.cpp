#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PhaseVocoderAudioProcessor::PhaseVocoderAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), 
                       apvts(*this, &undoManager, "Parameters", createParameterLayout())
{
    apvts.addParameterListener("FFT_SIZE", this);
}

PhaseVocoderAudioProcessor::~PhaseVocoderAudioProcessor()
{
}

//==============================================================================
const juce::String PhaseVocoderAudioProcessor::getName() const
{
    return "PhaseVocoder";
}

bool PhaseVocoderAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PhaseVocoderAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PhaseVocoderAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PhaseVocoderAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PhaseVocoderAudioProcessor::getNumPrograms()
{
    return 1;
}

int PhaseVocoderAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PhaseVocoderAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PhaseVocoderAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PhaseVocoderAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void PhaseVocoderAudioProcessor::prepareToPlay (double sampleRateIn, int samplesPerBlockIn)
{
    sampleRate = sampleRateIn;

    numChannels = getTotalNumInputChannels();

    DBG("PP prepare called");

    // Get choice indices for fftSize
    auto* p = dynamic_cast<AudioParameterChoice*>(apvts.getParameter("FFT_SIZE"));
    N = p->getCurrentChoiceName().getIntValue();

    // Rebuild the engine synchronously
    engine = std::make_unique<PhaseVocoder>(N, sampleRate, numChannels);

    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void PhaseVocoderAudioProcessor::releaseResources()
{
}

bool PhaseVocoderAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void PhaseVocoderAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    if (engine == nullptr)
    {
        buffer.clear();
        return;
    }

    // Update pitch shift ratio
    float pitchShiftSemitones = *apvts.getRawParameterValue("PITCH_SHIFT");
    float psr = std::pow(2.0f, pitchShiftSemitones / 12.0f);
    engine->pitchShiftRatioSmoothed.setTargetValue(psr);

    // Update vocoder mode
    int modeIndex = static_cast<int>(*apvts.getRawParameterValue("MODE"));
    engine->setMode(modeIndex);

    if (fftResizePending.exchange(false))
        engine->prepare(newFFTSize, sampleRate, numChannels);
    
    engine->process(buffer);
}

//==============================================================================
bool PhaseVocoderAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PhaseVocoderAudioProcessor::createEditor()
{
    return new PhaseVocoderAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PhaseVocoderAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParameterID {"PITCH_SHIFT", 1},  // param ID
        "Pitch Shift", // parameter name
        -12.0f, // min value
        12.0f, // max value
        0.0f // default value
    ));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParameterID {"FFT_SIZE", 1},  // param ID
        "FFT Size", // param name
        StringArray {"1024", "2048", "4096"}, // choices
        1 // default choice
    ));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
    ParameterID {"MODE", 1},  // parameter ID
    "Mode",            // parameter name
    juce::StringArray{"Pitch Shift", "Robotize", "Whisperize"}, // choices
    0                  // default index
    ));


    return { params.begin(), params.end() };
}

//==============================================================================
void PhaseVocoderAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void PhaseVocoderAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhaseVocoderAudioProcessor();
}
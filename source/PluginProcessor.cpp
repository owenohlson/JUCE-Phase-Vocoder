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
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
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

    // Get choice indices for fftSize
    auto* p = dynamic_cast<AudioParameterChoice*>(apvts.getParameter("FFT_SIZE"));
    N = p->getCurrentChoiceName().getIntValue();

    // Rebuild the engine synchronously
    engine = std::make_unique<PhaseVocoder>(N, sampleRate, numChannels);

    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void PhaseVocoderAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool PhaseVocoderAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
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
    float psr = *apvts.getRawParameterValue("PITCH_SHIFT_RATIO");
    engine->pitchShiftRatioSmoothed.setTargetValue(psr);

    if (fftResizePending.exchange(false))
        engine->prepare(N, sampleRate, numChannels);
    
    engine->process(buffer);
}

//==============================================================================
bool PhaseVocoderAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PhaseVocoderAudioProcessor::createEditor()
{
    return new PhaseVocoderAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PhaseVocoderAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    // layout.add (std::make_unique<AudioParameterChoice> ("mode", "PV Mode", StringArray {"Time Stretch", "Pitch Shift"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParameterID {"PITCH_SHIFT_RATIO", 1},  // param ID
        "Pitch Shift Ratio", // parameter name
        0.5f, // min value
        2.0f, // max value
        1.0f // default value
    ));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParameterID {"FFT_SIZE", 1},  // param ID
        "FFT Size", // param name
        StringArray {"1024", "2048", "4096"}, // choices
        1 // default choice
    ));

    // ...

    return { params.begin(), params.end() };
}

//==============================================================================
void PhaseVocoderAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void PhaseVocoderAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhaseVocoderAudioProcessor();
}
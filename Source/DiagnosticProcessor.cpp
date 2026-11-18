#include <JuceHeader.h>

// Minimal diagnostic processor.
// Keeps the production VST3 identity and stereo/mono bus contract,
// but deliberately excludes the custom editor, parameter tree, capture,
// render worker, DSP engine and custom state serialization.
class StutterLabDiagnosticProcessor final : public juce::AudioProcessor
{
public:
    StutterLabDiagnosticProcessor()
        : juce::AudioProcessor (BusesProperties()
            .withInput ("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    {
    }

    const juce::String getName() const override { return JucePlugin_Name; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        const auto input = layouts.getMainInputChannelSet();
        return (input == juce::AudioChannelSet::mono()
             || input == juce::AudioChannelSet::stereo())
            && input == layouts.getMainOutputChannelSet();
    }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StutterLabDiagnosticProcessor();
}

#pragma once
#include <JuceHeader.h>
#include <map>
#include "PluginProcessor.h"

// STUTTER LAB 0.5 - hardware-style control surface (graphite/beige panel, analogue VU meters,
// illuminated push buttons, red LED read-out). Original artwork; layout inspired by classic studio units.
namespace slui
{
class HardwareLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    HardwareLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getLabelFont (juce::Label&) override;
};

// Analogue-style needle meter with ballistic smoothing (UI thread only).
class VuMeter final : public juce::Component
{
public:
    explicit VuMeter (const juce::String& name) : title (name) {}
    void setLevel (float linearPeak);
    void paint (juce::Graphics&) override;
private:
    juce::String title;
    float needle = 0.0f;       // 0..1 on the scale
    float clipHold = 0.0f;
};
} // namespace slui

class StutterLabAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer, private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit StutterLabAudioProcessorEditor (StutterLabAudioProcessor&);
    ~StutterLabAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob  { juce::Slider s { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox }; juce::Label l; std::unique_ptr<SliderAttachment> att; };
    struct Toggle { juce::TextButton b; std::unique_ptr<ButtonAttachment> att; };

    Knob& addKnob (const juce::String& id, const juce::String& label);
    Toggle& addToggle (const juce::String& id, const juce::String& label);
    juce::TextButton& addPush (const juce::String& label);
    void placeKnobs (juce::Rectangle<int> area, std::initializer_list<const char*> ids);
    void setChoice (const char* id, int index);
    void timerCallback() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void refreshStatus();
    void doGenerate();
    void drawPeaks (juce::Graphics&, juce::Rectangle<int>, const std::vector<float>& minMax, juce::Colour);
    void drawDisplay (juce::Graphics&);
    void drawEnvelope (juce::Graphics&, juce::Rectangle<int>);
    void drawGroup (juce::Graphics&, juce::Rectangle<int>, const juce::String& title);
    int stepAt (juce::Point<int>) const;
    void setStep (int index, float y01);

    StutterLabAudioProcessor& processor;
    slui::HardwareLookAndFeel lnf;
    slui::VuMeter vuIn { "INPUT" }, vuOut { "OUTPUT" };

    std::map<juce::String, std::unique_ptr<Knob>> knobs;
    std::map<juce::String, std::unique_ptr<Toggle>> toggles;
    std::vector<std::unique_ptr<juce::TextButton>> pushes;
    juce::TextButton *captureButton = nullptr, *stopButton = nullptr, *clearButton = nullptr,
                     *previewButton = nullptr, *newSeedButton = nullptr,
                     *dirButtons[4] = {}, *filterTypeButtons[2] = {};
    juce::ComboBox divisionBox, delayRateBox;
    std::unique_ptr<ComboAttachment> divisionAttachment, delayRateAttachment;
    juce::Label statusLabel;

    juce::Rectangle<int> headerArea, displayArea, waveArea, renderArea, patternArea, ledArea, envArea, dragArea;
    juce::Rectangle<int> stutterGroup, moveGroup, toneGroup, outGroup;
    std::vector<float> capturePeaks, renderPeaks;
    int lastCaptureSamples = -1, lastW = 0;
    uint64_t shownRenderId = 0;
    bool wasRecording = false, draggingSelection = false, dragStarted = false, draggingSteps = false;
    std::atomic<bool> autoRenderPending { false };
    int autoRenderCountdown = 0;
    bool autoRenderArmed = false;
    uint16_t touchedSteps = 0;
    juce::Point<int> pressPoint;
    double dragAnchor = 0.0;
    juce::String infoText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StutterLabAudioProcessorEditor)
};

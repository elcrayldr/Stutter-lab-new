#include "PluginEditor.h"
#include <cmath>

namespace
{
const juce::Colour kPanelTop (0xffb9b6ab), kPanelBot (0xff9c998f), kBezel (0xff17181b), kInk (0xff25241f), kInkSoft (0xff5a574d),
                   kAmberLit (0xfff6c24a), kAmberLitLo (0xffd9921a), kAmberOff (0xffab7e25), kAmberOffLo (0xff7d5a16),
                   kGreenLit (0xff4fe08a), kGreenOff (0xff2e7f4f), kRedLit (0xffff5a46), kRedOff (0xff8a3328),
                   kLedBg (0xff1b0505), kLedFg (0xffff4a33), kScreen (0xff0d1014), kCream (0xffeae6d6);

juce::Font fnt (float h, bool bold = true) { return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain)); }
juce::Font mono (float h) { return juce::Font (juce::FontOptions ("Consolas", h, juce::Font::bold)); }
} // namespace

//==============================================================================
slui::HardwareLookAndFeel::HardwareLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff202226));
    setColour (juce::PopupMenu::textColourId, kCream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kAmberOff);
    setColour (juce::PopupMenu::highlightedTextColourId, kInk);
    setColour (juce::ComboBox::textColourId, kCream);
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff202226));
    setColour (juce::TooltipWindow::textColourId, kCream);
}

void slui::HardwareLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float d = juce::jmin (b.getWidth(), b.getHeight());
    const auto c = b.getCentre();
    const auto disc = juce::Rectangle<float> (d, d).withCentre (c);
    const float angle = a0 + pos * (a1 - a0);

    // scale ticks engraved on the panel
    g.setColour (kInkSoft);
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 10.0f;
        const float r0 = d * 0.5f + 1.0f, r1 = r0 + (i % 5 == 0 ? 5.0f : 3.0f);
        g.drawLine (c.x + std::sin (a) * r0, c.y - std::cos (a) * r0, c.x + std::sin (a) * r1, c.y - std::cos (a) * r1, 1.0f);
    }
    const auto body = disc.reduced (d * 0.06f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));                                   // drop shadow
    g.fillEllipse (body.translated (1.5f, 2.5f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe9e9ea), body.getX(), body.getY(), juce::Colour (0xff6d6e72), body.getRight(), body.getBottom(), false));
    g.fillEllipse (body);                                                                   // silver ring
    const auto cap = body.reduced (d * 0.09f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4b4c50), cap.getX(), cap.getY(), juce::Colour (0xff0e0f11), cap.getRight(), cap.getBottom(), false));
    g.fillEllipse (cap);                                                                    // black cap
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (cap.reduced (1.0f), 1.0f);
    juce::Path p;                                                                           // white pointer
    p.addRoundedRectangle (-1.6f, -cap.getHeight() * 0.48f, 3.2f, cap.getHeight() * 0.34f, 1.2f);
    g.setColour (s.isEnabled() ? juce::Colours::white : juce::Colours::grey);
    g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c));
}

void slui::HardwareLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& bt, const juce::Colour&, bool highlighted, bool down)
{
    auto b = bt.getLocalBounds().toFloat().reduced (1.5f);
    const bool lit = bt.getToggleState();
    const auto tone = bt.getProperties()["tone"].toString();
    juce::Colour hi = kAmberLit, lo = kAmberLitLo, offHi = kAmberOff, offLo = kAmberOffLo;
    if (tone == "green") { hi = kGreenLit; lo = kGreenLit.darker (0.35f); offHi = kGreenOff; offLo = kGreenOff.darker (0.4f); }
    if (tone == "red")   { hi = kRedLit;   lo = kRedLit.darker (0.35f);   offHi = kRedOff;   offLo = kRedOff.darker (0.4f); }
    if (! bt.isEnabled()) { hi = lo = offHi = offLo = juce::Colour (0xff77756c); }

    g.setColour (juce::Colours::black.withAlpha (0.45f));                                    // bezel
    g.fillRoundedRectangle (b.expanded (1.5f).translated (0, 1.5f), 4.0f);
    g.setColour (juce::Colour (0xff3a372f));
    g.fillRoundedRectangle (b.expanded (1.0f), 4.0f);
    auto face = b.reduced (1.5f);
    if (down) face = face.translated (0, 1.0f);
    g.setGradientFill (juce::ColourGradient (lit ? hi : offHi, face.getX(), face.getY(), lit ? lo : offLo, face.getX(), face.getBottom(), false));
    g.fillRoundedRectangle (face, 3.0f);
    if (lit)
    {
        g.setColour (hi.withAlpha (0.22f));                                                  // glow
        g.fillRoundedRectangle (face.expanded (2.0f), 5.0f);
    }
    if (highlighted) { g.setColour (juce::Colours::white.withAlpha (0.12f)); g.fillRoundedRectangle (face, 3.0f); }
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawLine (face.getX() + 3, face.getY() + 1.5f, face.getRight() - 3, face.getY() + 1.5f, 1.0f);   // top sheen
}

void slui::HardwareLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& bt, bool, bool)
{
    g.setColour (bt.isEnabled() ? juce::Colour (0xff2a1c04) : juce::Colour (0xff55534b));
    g.setFont (fnt (juce::jlimit (10.0f, 13.0f, (float) bt.getHeight() * 0.42f)));
    g.drawText (bt.getButtonText(), bt.getLocalBounds().reduced (2, 0), juce::Justification::centred, true);
}

void slui::HardwareLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    const auto b = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f);
    g.setColour (juce::Colour (0xff4a463c)); g.fillRoundedRectangle (b.expanded (1.0f), 4.0f);
    g.setColour (juce::Colour (0xff15161a)); g.fillRoundedRectangle (b, 3.0f);
    juce::Path t;
    t.addTriangle ((float) w - 16.0f, (float) h * 0.42f, (float) w - 8.0f, (float) h * 0.42f, (float) w - 12.0f, (float) h * 0.62f);
    g.setColour (kAmberLit); g.fillPath (t);
}
juce::Font slui::HardwareLookAndFeel::getComboBoxFont (juce::ComboBox&) { return fnt (12.0f); }
juce::Font slui::HardwareLookAndFeel::getLabelFont (juce::Label& l) { return l.getFont(); }

//==============================================================================
void slui::VuMeter::setLevel (float peak)
{
    const float db = juce::Decibels::gainToDecibels (peak, -60.0f);
    const float target = juce::jlimit (0.0f, 1.0f, (db + 30.0f) / 33.0f);                  // -30 dBFS .. +3 dBFS
    needle += (target - needle) * (target > needle ? 0.5f : 0.14f);                       // fast attack, slow release
    clipHold = peak >= 0.999f ? 1.0f : clipHold * 0.96f;
    repaint();
}

void slui::VuMeter::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colour (0xff2c2a25)); g.fillRoundedRectangle (b, 6.0f);                // bezel
    auto face = b.reduced (5.0f);
    g.setGradientFill (juce::ColourGradient (kCream, face.getX(), face.getY(), juce::Colour (0xffcfcab4), face.getX(), face.getBottom(), false));
    g.fillRoundedRectangle (face, 3.0f);

    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (face.toNearestInt());
    const auto pivot = juce::Point<float> (face.getCentreX(), face.getBottom() + face.getHeight() * 0.10f);
    const float R = face.getHeight() * 0.92f, sweep = 0.92f;
    auto at = [&] (float v, float r) { const float a = -sweep + 2.0f * sweep * v; return juce::Point<float> (pivot.x + std::sin (a) * r, pivot.y - std::cos (a) * r); };

    const float v0 = 30.0f / 33.0f;                                                         // 0 dBFS
    juce::Path red;                                                                         // red zone
    for (int i = 0; i <= 12; ++i) { const auto p = at (v0 + (1.0f - v0) * (float) i / 12.0f, R); if (i == 0) red.startNewSubPath (p); else red.lineTo (p); }
    g.setColour (juce::Colour (0xffc23a2b)); g.strokePath (red, juce::PathStrokeType (3.0f));
    juce::Path arc;
    for (int i = 0; i <= 40; ++i) { const auto p = at (v0 * (float) i / 40.0f, R); if (i == 0) arc.startNewSubPath (p); else arc.lineTo (p); }
    g.setColour (kInk); g.strokePath (arc, juce::PathStrokeType (1.4f));

    const struct { float db; const char* txt; } ticks[] = { { -30, "-30" }, { -20, "-20" }, { -10, "-10" }, { -6, "-6" }, { -3, "-3" }, { 0, "0" }, { 3, "+3" } };
    g.setFont (fnt (10.0f, false));
    for (auto& t : ticks)
    {
        const float v = (t.db + 30.0f) / 33.0f;
        const auto p0 = at (v, R), p1 = at (v, R - 7.0f), tp = at (v, R - 18.0f);
        g.setColour (t.db > 0.5f ? juce::Colour (0xffc23a2b) : kInk);
        g.drawLine (p0.x, p0.y, p1.x, p1.y, 1.4f);
        g.drawText (t.txt, juce::Rectangle<float> (26, 12).withCentre (tp).toNearestInt(), juce::Justification::centred);
    }
    g.setColour (kInk); g.setFont (fnt (12.0f));
    g.drawText (title, face.withTrimmedTop (face.getHeight() * 0.58f).toNearestInt(), juce::Justification::centredTop);
    g.setFont (fnt (9.0f, false)); g.setColour (kInkSoft);
    g.drawText ("dBFS PEAK", face.withTrimmedTop (face.getHeight() * 0.76f).toNearestInt(), juce::Justification::centredTop);

    const auto tip = at (needle, R * 0.97f);                                                // needle
    g.setColour (juce::Colours::black.withAlpha (0.25f)); g.drawLine (pivot.x + 1, pivot.y + 1, tip.x + 1.5f, tip.y + 1.5f, 1.6f);
    g.setColour (juce::Colour (0xff111111)); g.drawLine (pivot.x, pivot.y, tip.x, tip.y, 1.5f);
    g.setColour (clipHold > 0.05f ? kRedLit.withAlpha (clipHold) : juce::Colour (0x00000000));
    g.fillEllipse (juce::Rectangle<float> (7, 7).withCentre ({ face.getRight() - 12.0f, face.getY() + 12.0f }));   // clip lamp
}

//==============================================================================
StutterLabAudioProcessorEditor::StutterLabAudioProcessorEditor (StutterLabAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lnf);
    setSize (1180, 780);
    setResizable (true, true);
    setResizeLimits (980, 680, 1900, 1250);

    addAndMakeVisible (vuIn); addAndMakeVisible (vuOut);

    // --- source / transport
    captureButton = &addPush ("CAPTURE"); captureButton->getProperties().set ("tone", "red");
    stopButton = &addPush ("STOP");
    clearButton = &addPush ("CLEAR");
    captureButton->onClick = [this] { processor.startCapture(); };
    stopButton->onClick = [this]
    {
        if (processor.getRenderState() == StutterLabAudioProcessor::RenderState::Rendering) processor.cancelRender();
        else { processor.stopCapture(); processor.stopPreview(); }
    };
    clearButton->onClick = [this] { processor.clearCapture(); processor.resetClip(); };

    // --- header: presets
    presetBox.addItemList ({ "INIT", "TIGHT STUTTER", "TRAP ROLLS", "VOCAL GLITCH", "REVERSE PULSE", "PROGRESSIVE RISER", "RHYTHMIC GATE", "FAST REPEATS", "BROKEN RHYTHM" }, 1);
    presetBox.setSelectedId (1, juce::dontSendNotification);
    presetBox.onChange = [this] { processor.loadFactoryPreset (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);
    savePresetButton = &addPush ("SAVE"); loadPresetButton = &addPush ("LOAD"); undoButton = &addPush ("UNDO");
    undoButton->onClick = [this] { if (! processor.undoParameterChange()) infoText = "Nothing to undo."; };
    savePresetButton->onClick = [this]
    {
        const auto def = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("STUTTER_LAB_Preset.slpreset");
        fileChooser = std::make_unique<juce::FileChooser> ("Save STUTTER LAB preset", def, "*.slpreset");
        juce::Component::SafePointer<StutterLabAudioProcessorEditor> safe (this);
        fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
            [safe] (const juce::FileChooser& fc)
            {
                if (safe == nullptr || fc.getResult() == juce::File()) return;
                juce::String err;
                safe->infoText = safe->processor.saveUserPreset (fc.getResult().withFileExtension ("slpreset"), err) ? "Preset saved." : err;
            });
    };
    loadPresetButton->onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser> ("Load STUTTER LAB preset", juce::File(), "*.slpreset");
        juce::Component::SafePointer<StutterLabAudioProcessorEditor> safe (this);
        fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safe] (const juce::FileChooser& fc)
            {
                if (safe == nullptr || fc.getResult() == juce::File()) return;
                juce::String err;
                safe->infoText = safe->processor.loadUserPreset (fc.getResult(), err) ? "Preset loaded." : err;
            });
    };
    statusLabel.setFont (fnt (12.0f)); statusLabel.setColour (juce::Label::textColourId, kInk);
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (statusLabel);
    addKnob ("bpm", "BPM");

    // --- STUTTER ENGINE
    for (int i = 0; i < 4; ++i)
    {
        static const char* names[4] = { "FWD", "REV", "PING", "RAND" };
        dirButtons[i] = &addPush (names[i]);
        dirButtons[i]->onClick = [this, i] { setChoice ("direction", i); };
    }
    for (int i = 0; i < cray::kNumDivisions; ++i) { divisionBox.addItem (cray::kDivisionNames[i], i + 1); delayRateBox.addItem (cray::kDivisionNames[i], i + 1); }
    divisionBox.setTooltip ("Stutter rate (note value at the host tempo)");
    delayRateBox.setTooltip ("Delay time (note value at the host tempo)");
    addAndMakeVisible (divisionBox); addAndMakeVisible (delayRateBox);
    divisionAttachment = std::make_unique<ComboAttachment> (processor.getParameters(), "division", divisionBox);
    delayRateAttachment = std::make_unique<ComboAttachment> (processor.getParameters(), "delayDivision", delayRateBox);
    addKnob ("repeats", "REPEATS"); addKnob ("gate", "GATE"); addKnob ("intensity", "INTENSITY");
    addKnob ("swing", "SWING"); addKnob ("humanize", "HUMANIZE"); addKnob ("steps", "LENGTH");
    addToggle ("scan", "SCAN");
    newSeedButton = &addPush ("NEW SEED");
    newSeedButton->onClick = [this] { processor.seed.store ((int) juce::Random::getSystemRandom().nextInt()); };

    // --- MOVEMENT
    addKnob ("rampStart", "START"); addKnob ("rampEnd", "END"); addKnob ("rampCurve", "CURVE");
    addKnob ("fadeIn", "FADE IN"); addKnob ("fadeOut", "FADE OUT"); addKnob ("drive", "DRIVE");
    addKnob ("pitch", "PITCH"); addKnob ("formant", "FORMANT"); addToggle ("phone", "PHONE");

    // --- TONE & SPACE
    addToggle ("filterOn", "FILTER");
    filterTypeButtons[0] = &addPush ("LP"); filterTypeButtons[1] = &addPush ("HP");
    for (int i = 0; i < 2; ++i) filterTypeButtons[i]->onClick = [this, i] { setChoice ("filterType", i); };
    addKnob ("filterCutoff", "CUTOFF"); addKnob ("filterRes", "RESO");
    addToggle ("delayOn", "DELAY"); addKnob ("delayFeedback", "FEEDBACK"); addKnob ("delayMix", "MIX");
    addToggle ("reverbOn", "REVERB"); addKnob ("reverbSize", "SIZE"); addKnob ("reverbDecay", "DECAY"); addKnob ("reverbMix", "MIX ");

    // --- OUTPUT
    generateButton = &addPush ("GENERATE"); generateButton->getProperties().set ("tone", "green");
    playOrigButton = &addPush ("PLAY ORIGINAL"); playProcButton = &addPush ("PLAY PROCESSED");
    exportButton = &addPush ("EXPORT WAV");
    addToggle ("loop", "LOOP");
    generateButton->onClick = [this] { doGenerate(); };
    playOrigButton->onClick = [this] { if (processor.isPreviewPlaying() && previewIsOriginal) processor.stopPreview(); else { previewIsOriginal = true; processor.startPreview (true); } };
    playProcButton->onClick = [this] { if (processor.isPreviewPlaying() && ! previewIsOriginal) processor.stopPreview(); else { previewIsOriginal = false; processor.startPreview (false); } };
    exportButton->onClick = [this] { doExport(); };

    refreshStatus();
    startTimerHz (20);
}

StutterLabAudioProcessorEditor::~StutterLabAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

StutterLabAudioProcessorEditor::Knob& StutterLabAudioProcessorEditor::addKnob (const juce::String& id, const juce::String& label)
{
    auto k = std::make_unique<Knob>();
    k->s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    k->s.setPopupDisplayEnabled (true, true, this);
    if (auto* parameter = processor.getParameters().getParameter (id))
        k->s.setDoubleClickReturnValue (true, parameter->getDefaultValue());
    k->l.setText (label.trim(), juce::dontSendNotification);
    k->l.setFont (fnt (11.0f)); k->l.setColour (juce::Label::textColourId, kInk);
    k->l.setJustificationType (juce::Justification::centred); k->l.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (k->s); addAndMakeVisible (k->l);
    k->att = std::make_unique<SliderAttachment> (processor.getParameters(), id, k->s);
    auto& ref = *k; knobs[id] = std::move (k); return ref;
}

StutterLabAudioProcessorEditor::Toggle& StutterLabAudioProcessorEditor::addToggle (const juce::String& id, const juce::String& label)
{
    auto t = std::make_unique<Toggle>();
    t->b.setButtonText (label); t->b.setClickingTogglesState (true);
    addAndMakeVisible (t->b);
    t->att = std::make_unique<ButtonAttachment> (processor.getParameters(), id, t->b);
    auto& ref = *t; toggles[id] = std::move (t); return ref;
}

juce::TextButton& StutterLabAudioProcessorEditor::addPush (const juce::String& label)
{
    pushes.push_back (std::make_unique<juce::TextButton> (label));
    addAndMakeVisible (*pushes.back());
    return *pushes.back();
}

void StutterLabAudioProcessorEditor::setChoice (const char* id, int index)
{
    if (auto* prm = processor.getParameters().getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) index));
        prm->endChangeGesture();
    }
}

void StutterLabAudioProcessorEditor::placeKnobs (juce::Rectangle<int> area, std::initializer_list<const char*> ids)
{
    const int n = (int) ids.size();
    if (n == 0) return;
    const int cw = area.getWidth() / n;
    int i = 0;
    for (auto* id : ids)
    {
        auto cell = juce::Rectangle<int> (area.getX() + i * cw, area.getY(), cw, area.getHeight());
        auto& k = *knobs.at (id);
        auto lab = cell.removeFromBottom (16);
        k.l.setBounds (lab);
        const int d = juce::jmin (cell.getWidth() - 6, cell.getHeight());
        k.s.setBounds (juce::Rectangle<int> (d, d).withCentre (cell.getCentre()));
        ++i;
    }
}

//==============================================================================
void StutterLabAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (16);
    headerArea = r.removeFromTop (juce::jmax (58, (int) (r.getHeight() * 0.085f)));
    r.removeFromTop (8);
    displayArea = r.removeFromTop ((int) (r.getHeight() * 0.38f));
    r.removeFromTop (10);
    auto controls = r;

    // header
    {
        auto h = headerArea;
        h.removeFromLeft (290);                                                              // silkscreen title is painted
        ledArea = h.removeFromRight (230).reduced (0, 4);
        auto bpm = h.removeFromRight (62); knobs.at ("bpm")->s.setBounds (bpm.withTrimmedBottom (14).reduced (4)); knobs.at ("bpm")->l.setBounds (bpm.removeFromBottom (14));
        presetBox.setBounds (h.removeFromLeft (190).reduced (2, 12));
        savePresetButton->setBounds (h.removeFromLeft (54).reduced (2, 12));
        loadPresetButton->setBounds (h.removeFromLeft (54).reduced (2, 12));
        undoButton->setBounds (h.removeFromLeft (54).reduced (2, 12));
        statusLabel.setBounds (h.reduced (8, 10));
    }

    // display row: [VU in] [source + pattern screen] [VU out]
    {
        auto d = displayArea;
        const int vuW = juce::jmin (230, (int) (d.getWidth() * 0.19f));
        vuIn.setBounds (d.removeFromLeft (vuW));
        vuOut.setBounds (d.removeFromRight (vuW));
        d.reduce (8, 0);
        auto top = d.removeFromTop (34);                                                     // source buttons
        captureButton->setBounds (top.removeFromLeft (110).reduced (2));
        stopButton->setBounds (top.removeFromLeft (80).reduced (2));
        clearButton->setBounds (top.removeFromLeft (80).reduced (2));
        d.removeFromTop (4);
        auto screen = d;
        patternArea = screen.removeFromBottom ((int) (screen.getHeight() * 0.36f)).reduced (6, 4);
        renderArea = screen.removeFromBottom (screen.getHeight() / 2).reduced (6, 2);
        waveArea = screen.reduced (6, 2);
    }

    // control groups
    const int gap = 10;
    const int total = controls.getWidth() - 3 * gap;
    stutterGroup = controls.removeFromLeft ((int) (total * 0.33f)); controls.removeFromLeft (gap);
    moveGroup = controls.removeFromLeft ((int) (total * 0.21f));    controls.removeFromLeft (gap);
    toneGroup = controls.removeFromLeft ((int) (total * 0.32f));    controls.removeFromLeft (gap);
    outGroup = controls;

    auto content = [] (juce::Rectangle<int> g) { return g.withTrimmedTop (28).reduced (8, 4); };

    {   // STUTTER ENGINE
        auto c = content (stutterGroup);
        auto row = c.removeFromTop (34);
        divisionBox.setBounds (row.removeFromLeft (76).reduced (2));
        for (int i = 0; i < 4; ++i) dirButtons[i]->setBounds (row.removeFromLeft (row.getWidth() / (4 - i)).reduced (2));
        auto bottom = c.removeFromBottom (34);
        toggles.at ("scan")->b.setBounds (bottom.removeFromLeft (bottom.getWidth() / 2).reduced (2));
        newSeedButton->setBounds (bottom.reduced (2));
        const int h = c.getHeight() / 2;
        placeKnobs (c.removeFromTop (h), { "repeats", "gate", "intensity" });
        placeKnobs (c, { "swing", "humanize", "steps" });
    }
    {   // MOVEMENT
        auto c = content (moveGroup);
        envArea = c.removeFromTop (juce::jmax (48, c.getHeight() / 5)).reduced (2, 2);
        const int h = c.getHeight() / 3;
        placeKnobs (c.removeFromTop (h), { "rampStart", "rampEnd", "rampCurve" });
        placeKnobs (c.removeFromTop (h), { "fadeIn", "fadeOut", "drive" });
        auto last = c;
        const int cw = last.getWidth() / 3;
        placeKnobs (last.removeFromLeft (cw * 2), { "pitch", "formant" });
        toggles.at ("phone")->b.setBounds (last.withSizeKeepingCentre (last.getWidth() - 6, 30));
    }
    {   // TONE & SPACE
        auto c = content (toneGroup);
        const int h = c.getHeight() / 3;
        auto row = [&] (juce::Rectangle<int> a, const juce::String& toggleId, juce::Rectangle<int>& rest)
        {
            auto t = a.removeFromLeft (62);
            toggles.at (toggleId)->b.setBounds (t.withSizeKeepingCentre (t.getWidth() - 4, 30));
            rest = a;
        };
        juce::Rectangle<int> rest;
        row (c.removeFromTop (h), "filterOn", rest);
        auto sel = rest.removeFromLeft (64);
        filterTypeButtons[0]->setBounds (sel.withSizeKeepingCentre (sel.getWidth() - 4, 28).removeFromTop (28).translated (0, -17));
        filterTypeButtons[1]->setBounds (sel.withSizeKeepingCentre (sel.getWidth() - 4, 28).translated (0, 17));
        placeKnobs (rest, { "filterCutoff", "filterRes" });
        row (c.removeFromTop (h), "delayOn", rest);
        sel = rest.removeFromLeft (64);
        delayRateBox.setBounds (sel.withSizeKeepingCentre (sel.getWidth() - 4, 28));
        placeKnobs (rest, { "delayFeedback", "delayMix" });
        row (c, "reverbOn", rest);
        placeKnobs (rest, { "reverbSize", "reverbDecay", "reverbMix" });
    }
    {   // OUTPUT
        auto c = content (outGroup);
        const int bh = juce::jmin (44, c.getHeight() / 6);
        generateButton->setBounds (c.removeFromTop (bh + 8).reduced (2));
        c.removeFromTop (4);
        playOrigButton->setBounds (c.removeFromTop (bh).reduced (2));
        playProcButton->setBounds (c.removeFromTop (bh).reduced (2));
        toggles.at ("loop")->b.setBounds (c.removeFromTop (bh).reduced (2));
        exportButton->setBounds (c.removeFromTop (bh).reduced (2));
        dragArea = c.reduced (2, 6);
    }
}

//==============================================================================
void StutterLabAudioProcessorEditor::drawGroup (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    const auto f = r.toFloat().reduced (1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.28f)); g.drawRoundedRectangle (f, 5.0f, 1.2f);             // engraved outline
    g.setColour (juce::Colours::white.withAlpha (0.35f)); g.drawRoundedRectangle (f.translated (0.0f, 1.0f), 5.0f, 1.0f);
    g.setFont (fnt (12.0f));
    const int tw = (int) fnt (12.0f).getStringWidthFloat (title) + 18;
    const auto t = juce::Rectangle<int> (tw, 18).withCentre ({ r.getCentreX(), r.getY() + 14 });
    g.setColour (kInk); g.drawText (title, t, juce::Justification::centred);
    g.setColour (kInkSoft);
    g.drawLine ((float) r.getX() + 14, (float) t.getCentreY(), (float) t.getX(), (float) t.getCentreY(), 1.0f);
    g.drawLine ((float) t.getRight(), (float) t.getCentreY(), (float) r.getRight() - 14, (float) t.getCentreY(), 1.0f);
}

void StutterLabAudioProcessorEditor::drawPeaks (juce::Graphics& g, juce::Rectangle<int> r, const std::vector<float>& mm, juce::Colour c)
{
    if (mm.size() < 2 || r.isEmpty()) return;
    const int bins = (int) (mm.size() / 2);
    const float mid = (float) r.getCentreY(), half = (float) r.getHeight() * 0.48f;
    g.setColour (c);
    for (int x = 0; x < r.getWidth(); ++x)
    {
        const int b = juce::jlimit (0, bins - 1, x * bins / juce::jmax (1, r.getWidth()));
        const float lo = mm[(size_t) b * 2], hi = mm[(size_t) b * 2 + 1];
        g.drawVerticalLine (r.getX() + x, mid - juce::jlimit (0.0f, 1.0f, hi) * half, mid + juce::jlimit (0.0f, 1.0f, -lo) * half + 1.0f);
    }
}

void StutterLabAudioProcessorEditor::drawEnvelope (juce::Graphics& g, juce::Rectangle<int> r)
{
    g.setColour (kScreen); g.fillRoundedRectangle (r.toFloat(), 3.0f);
    auto raw = [this] (const char* id) { return processor.getParameters().getRawParameterValue (id)->load(); };
    const float g0 = raw ("rampStart"), g1 = raw ("rampEnd"), curve = raw ("rampCurve");
    const double bodySec = juce::jmax (0.2, cray::divisionSamples ((int) raw ("division"), processor.getEffectiveBpm(), 1000.0) / 1000.0 * raw ("repeats"));
    const float fi = juce::jlimit (0.0f, 0.5f, (float) (raw ("fadeIn") * 0.001 / bodySec));
    const float fo = juce::jlimit (0.0f, 0.5f, (float) (raw ("fadeOut") * 0.001 / bodySec));
    const double k = std::exp2 ((double) curve * 2.0);
    const auto in = r.toFloat().reduced (5.0f, 5.0f);
    juce::Path path;
    for (int i = 0; i <= 80; ++i)
    {
        const float x = (float) i / 80.0f;
        float v = g0 + (g1 - g0) * (float) std::pow ((double) x, k);
        if (fi > 0.0f && x < fi) v *= std::sin (0.5f * juce::MathConstants<float>::pi * x / fi);
        if (fo > 0.0f && x > 1.0f - fo) v *= std::sin (0.5f * juce::MathConstants<float>::pi * (1.0f - x) / fo);
        const float px = in.getX() + x * in.getWidth(), py = in.getBottom() - juce::jlimit (0.0f, 1.5f, v) / 1.5f * in.getHeight();
        if (i == 0) path.startNewSubPath (px, py); else path.lineTo (px, py);
    }
    g.setColour (juce::Colour (0x22ffffff)); g.drawHorizontalLine ((int) (in.getBottom() - in.getHeight() / 1.5f), in.getX(), in.getRight());   // unity
    g.setColour (kAmberLit); g.strokePath (path, juce::PathStrokeType (1.8f));
}

void StutterLabAudioProcessorEditor::drawDisplay (juce::Graphics& g)
{
    auto d = juce::Rectangle<int> (waveArea.getX() - 6, waveArea.getY() - 2, waveArea.getWidth() + 12, patternArea.getBottom() - waveArea.getY() + 6);
    g.setColour (juce::Colour (0xff2c2a25)); g.fillRoundedRectangle (d.toFloat().expanded (3.0f), 7.0f);
    g.setColour (kScreen);                   g.fillRoundedRectangle (d.toFloat(), 5.0f);
    g.setColour (juce::Colour (0x14ffffff));
    for (int y = d.getY() + 3; y < d.getBottom(); y += 3) g.drawHorizontalLine (y, (float) d.getX() + 2, (float) d.getRight() - 2);   // scan lines

    // SOURCE
    g.setColour (juce::Colour (0x22ffffff)); g.drawHorizontalLine (waveArea.getCentreY(), (float) waveArea.getX(), (float) waveArea.getRight());
    drawPeaks (g, waveArea, capturePeaks, kCream.withAlpha (0.9f));
    const double dur = processor.getCaptureSeconds();
    if (processor.isCapturing())
    {
        g.setColour (kRedLit); g.setFont (fnt (13.0f));
        g.drawText ("REC  " + juce::String (dur, 1) + " s", waveArea.reduced (8, 4), juce::Justification::topLeft);
    }
    else if (dur <= 0.0)
    {
        g.setColour (juce::Colour (0xff8d8f95)); g.setFont (fnt (13.0f, false));
        g.drawText ("PRESS CAPTURE AND PLAY AUDIO THROUGH THE TRACK", waveArea, juce::Justification::centred);
    }
    else
    {
        const float x0 = (float) waveArea.getX() + (float) (processor.getSelectionStart() / dur) * waveArea.getWidth();
        const float x1 = (float) waveArea.getX() + (float) (processor.getSelectionEnd() / dur) * waveArea.getWidth();
        g.setColour (kAmberLit.withAlpha (0.20f)); g.fillRect (juce::Rectangle<float> (x0, (float) waveArea.getY(), juce::jmax (2.0f, x1 - x0), (float) waveArea.getHeight()));
        g.setColour (kAmberLit); g.drawVerticalLine ((int) x0, (float) waveArea.getY(), (float) waveArea.getBottom()); g.drawVerticalLine ((int) x1, (float) waveArea.getY(), (float) waveArea.getBottom());
    }
    g.setColour (juce::Colour (0xff8d8f95)); g.setFont (fnt (9.0f));
    g.drawText ("SOURCE", waveArea.reduced (4, 2), juce::Justification::topRight);

    // RESULT
    g.setColour (juce::Colour (0x22ffffff)); g.drawHorizontalLine (renderArea.getCentreY(), (float) renderArea.getX(), (float) renderArea.getRight());
    drawPeaks (g, renderArea, renderPeaks, kAmberLit.withAlpha (0.95f));
    if (processor.isPreviewPlaying())
    {
        const int px = renderArea.getX() + (int) (processor.getPreviewProgress() * (float) renderArea.getWidth());
        g.setColour (juce::Colours::white); g.drawVerticalLine (px, (float) renderArea.getY(), (float) renderArea.getBottom());
    }
    g.setColour (juce::Colour (0xff8d8f95)); g.setFont (fnt (9.0f));
    g.drawText (processor.hasRender() ? "RESULT  (drag to DAW)" : "RESULT", renderArea.reduced (4, 2), juce::Justification::topRight);

    // PATTERN (visible before playing; click / drag to edit)
    auto raw = [this] (const juce::String& id) { return processor.getParameters().getRawParameterValue (id)->load(); };
    const int len = juce::jlimit (1, 16, (int) raw ("steps"));
    const float cw = (float) patternArea.getWidth() / 16.0f;
    for (int i = 0; i < 16; ++i)
    {
        const float lv = raw ("step" + juce::String (i + 1));
        const bool active = i < len;
        auto col = juce::Rectangle<float> ((float) patternArea.getX() + cw * (float) i + 2.0f, (float) patternArea.getY(), cw - 4.0f, (float) patternArea.getHeight());
        g.setColour (juce::Colour (active ? 0x30ffffff : 0x12ffffff)); g.fillRoundedRectangle (col, 2.0f);
        if (lv > 0.001f)
        {
            auto bar = col.withTrimmedTop (col.getHeight() * (1.0f - lv));
            g.setColour (active ? kAmberLit.withAlpha (0.55f + 0.45f * lv) : juce::Colour (0xff55575c)); g.fillRoundedRectangle (bar, 2.0f);
        }
        if (i % 4 == 0) { g.setColour (juce::Colour (0xff8d8f95)); g.setFont (fnt (9.0f, false)); g.drawText (juce::String (i + 1), col.toNearestInt().withHeight (11), juce::Justification::centredLeft); }
    }
}

void StutterLabAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (kBezel); g.fillAll();
    auto panel = b.reduced (6.0f);
    g.setGradientFill (juce::ColourGradient (kPanelTop, 0, panel.getY(), kPanelBot, 0, panel.getBottom(), false));
    g.fillRoundedRectangle (panel, 6.0f);
    g.setColour (juce::Colours::white.withAlpha (0.25f)); g.drawRoundedRectangle (panel.reduced (1.0f), 6.0f, 1.0f);
    for (auto p : { juce::Point<float> (16, 16), { b.getRight() - 16, 16 }, { 16, b.getBottom() - 16 }, { b.getRight() - 16, b.getBottom() - 16 } })   // screws
    {
        g.setColour (juce::Colour (0xff55565a)); g.fillEllipse (juce::Rectangle<float> (9, 9).withCentre (p));
        g.setColour (juce::Colour (0xff222326)); g.drawLine (p.x - 3, p.y + 2, p.x + 3, p.y - 2, 1.4f);
    }

    // header silkscreen
    g.setColour (kInk); g.setFont (fnt (30.0f));
    g.drawText ("STUTTER LAB", headerArea.withWidth (290).withTrimmedBottom (18), juce::Justification::centredLeft);
    g.setColour (kInkSoft); g.setFont (fnt (10.0f, false));
    g.drawText ("CRAY LDR AUDIO SOFTWARE  -  VOCAL STUTTER INSTRUMENT", headerArea.withWidth (320).withTrimmedTop (headerArea.getHeight() - 20), juce::Justification::centredLeft);

    // LED read-out
    g.setColour (juce::Colour (0xff2c2a25)); g.fillRoundedRectangle (ledArea.toFloat().expanded (2.0f), 5.0f);
    g.setColour (kLedBg); g.fillRoundedRectangle (ledArea.toFloat(), 4.0f);
    const auto ri = processor.getRenderInfo();
    const juce::String bpmTxt = juce::String (processor.getEffectiveBpm(), 1).paddedLeft (' ', 5) + (processor.hostTempoAvailable() ? " HOST" : " MAN ");
    const juce::String lenTxt = processor.hasRender() ? juce::String (ri.seconds, 3) + " s" : "--.--- s";
    g.setColour (kLedFg); g.setFont (mono (17.0f));
    g.drawText ("BPM " + bpmTxt, ledArea.reduced (8, 2).removeFromTop (ledArea.getHeight() / 2), juce::Justification::centredLeft);
    g.drawText ("LEN " + lenTxt, ledArea.reduced (8, 2).removeFromBottom (ledArea.getHeight() / 2), juce::Justification::centredLeft);

    drawDisplay (g);
    drawGroup (g, stutterGroup, "STUTTER ENGINE");
    drawGroup (g, moveGroup, "MOVEMENT");
    drawGroup (g, toneGroup, "TONE & SPACE");
    drawGroup (g, outGroup, "OUTPUT");
    drawEnvelope (g, envArea);

    // OUTPUT drag handle + render info
    const bool ready = processor.hasRender();
    g.setColour (ready ? juce::Colour (0xff2c2a25) : juce::Colour (0xff77756c)); g.fillRoundedRectangle (dragArea.toFloat(), 5.0f);
    g.setColour (ready ? kAmberLit : juce::Colour (0xff9a988f)); g.setFont (fnt (12.0f));
    g.drawText ("DRAG TO DAW", dragArea.withTrimmedBottom (dragArea.getHeight() / 3), juce::Justification::centred);
    if (ready)
    {
        g.setColour (kCream); g.setFont (fnt (9.5f, false));
        g.drawText (juce::String (ri.sampleRate / 1000.0, 1) + " kHz / " + juce::String (ri.bitDepth) + " bit / peak "
                    + juce::String (juce::Decibels::gainToDecibels ((float) ri.peak), 1) + " dB",
                    dragArea.withTrimmedTop (dragArea.getHeight() * 2 / 3), juce::Justification::centred);
    }
}

//==============================================================================
void StutterLabAudioProcessorEditor::timerCallback()
{
    vuIn.setLevel (processor.getInputPeak());
    vuOut.setLevel (processor.getOutputPeak());

    const bool rec = processor.isCapturing();
    if (wasRecording && ! rec)                                            // take just ended: select all of it
    {
        processor.setSelection (0.0, processor.getCaptureSeconds());
        lastCaptureSamples = -1;
    }
    wasRecording = rec;

    const int samples = processor.getCapturedSamples();
    if (! rec && (samples != lastCaptureSamples || waveArea.getWidth() != lastW))
    {
        const int bins = juce::jmax (1, waveArea.getWidth());
        std::vector<float> mn ((size_t) bins), mx ((size_t) bins);
        processor.getCapture().peaks (0, (size_t) samples, bins, mn.data(), mx.data());
        capturePeaks.assign ((size_t) bins * 2, 0.0f);
        for (int i = 0; i < bins; ++i) { capturePeaks[(size_t) i * 2] = mn[(size_t) i]; capturePeaks[(size_t) i * 2 + 1] = mx[(size_t) i]; }
        lastCaptureSamples = samples; lastW = waveArea.getWidth();
    }
    const uint64_t rid = processor.getRenderId();
    if (rid != shownRenderId)
    {
        renderPeaks = processor.hasRender() ? processor.getRenderInfo().peaks : std::vector<float>{};
        shownRenderId = rid;
    }

    auto raw = [this] (const char* id) { return processor.getParameters().getRawParameterValue (id)->load(); };
    const int dir = (int) raw ("direction"), ft = (int) raw ("filterType");
    for (int i = 0; i < 4; ++i) dirButtons[i]->setToggleState (dir == i, juce::dontSendNotification);
    for (int i = 0; i < 2; ++i) filterTypeButtons[i]->setToggleState (ft == i, juce::dontSendNotification);
    captureButton->setToggleState (rec, juce::dontSendNotification);
    const bool playing = processor.isPreviewPlaying();
    playOrigButton->setToggleState (playing && previewIsOriginal, juce::dontSendNotification);
    playProcButton->setToggleState (playing && ! previewIsOriginal, juce::dontSendNotification);
    const bool rendering = processor.getRenderState() == StutterLabAudioProcessor::RenderState::Rendering;
    generateButton->setToggleState (rendering, juce::dontSendNotification);
    generateButton->setButtonText (rendering ? "RENDERING " + juce::String ((int) (processor.getRenderProgress() * 100.0f)) + "%" : "GENERATE");
    stopButton->setButtonText (rendering ? "CANCEL" : "STOP");
    playOrigButton->setEnabled (processor.hasRender());
    playProcButton->setEnabled (processor.hasRender());
    exportButton->setEnabled (processor.hasRender());
    generateButton->setEnabled (! rec);
    knobs.at ("bpm")->s.setEnabled (! processor.hostTempoAvailable());
    refreshStatus();
    repaint();
}

void StutterLabAudioProcessorEditor::refreshStatus()
{
    using RS = StutterLabAudioProcessor::RenderState;
    juce::String s;
    if (processor.isCapturing()) s = "RECORDING  " + juce::String (processor.getCaptureSeconds(), 1) + " s";
    else if (processor.getRenderState() == RS::Rendering) s = "RENDERING";
    else if (processor.isPreviewPlaying()) s = "PLAYING";
    else s = "READY  |  take " + juce::String (processor.getCaptureSeconds(), 2) + " s  |  selection "
             + juce::String (processor.getSelectionEnd() - processor.getSelectionStart(), 3) + " s";
    if (processor.captureHitLimit()) s += "  |  60 s LIMIT";
    if (processor.inputClipped()) s += "  |  INPUT CLIP";
    if (processor.getRenderState() == RS::Failed) s = processor.getRenderInfo().message;
    if (infoText.isNotEmpty()) s += "  |  " + infoText;
    statusLabel.setText (s, juce::dontSendNotification);
}

void StutterLabAudioProcessorEditor::doGenerate()
{
    infoText.clear();
    juce::String err;
    if (! processor.generate (err)) infoText = err;
}

void StutterLabAudioProcessorEditor::doExport()
{
    if (! processor.hasRender()) return;
    const auto def = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("STUTTER_LAB_Render.wav");
    fileChooser = std::make_unique<juce::FileChooser> ("Export WAV", def, "*.wav");
    juce::Component::SafePointer<StutterLabAudioProcessorEditor> safe (this);
    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
        [safe] (const juce::FileChooser& fc)
        {
            if (safe == nullptr) return;
            auto dest = fc.getResult();
            if (dest == juce::File()) return;
            juce::String err;
            safe->infoText = safe->processor.exportRenderTo (dest.withFileExtension ("wav"), err) ? "Exported " + dest.getFileName() : err;
        });
}

//==============================================================================
int StutterLabAudioProcessorEditor::stepAt (juce::Point<int> p) const
{
    if (! patternArea.expanded (0, 6).contains (p)) return -1;
    return juce::jlimit (0, 15, (p.x - patternArea.getX()) * 16 / juce::jmax (1, patternArea.getWidth()));
}

void StutterLabAudioProcessorEditor::setStep (int i, float y01)
{
    if (auto* prm = processor.getParameters().getParameter ("step" + juce::String (i + 1)))
    {
        if ((touchedSteps & (1u << i)) == 0) { prm->beginChangeGesture(); touchedSteps = (uint16_t) (touchedSteps | (1u << i)); }
        const float v = y01 < 0.1f ? 0.0f : juce::jlimit (0.1f, 1.0f, y01);                      // bottom 10 % = rest
        prm->setValueNotifyingHost (v);
    }
}

void StutterLabAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    pressPoint = e.getPosition(); dragStarted = false;
    if (const int s = stepAt (e.getPosition()); s >= 0)
    {
        draggingSteps = true; touchedSteps = 0;
        setStep (s, 1.0f - (float) (e.y - patternArea.getY()) / (float) juce::jmax (1, patternArea.getHeight()));
        return;
    }
    if (waveArea.contains (e.getPosition()) && processor.getCapturedSamples() > 0 && ! processor.isCapturing())
    {
        draggingSelection = true;
        dragAnchor = (double) (e.x - waveArea.getX()) / waveArea.getWidth() * processor.getCaptureSeconds();
    }
}

void StutterLabAudioProcessorEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingSteps)
    {
        if (const int s = stepAt (e.getPosition()); s >= 0)
            setStep (s, 1.0f - (float) (e.y - patternArea.getY()) / (float) juce::jmax (1, patternArea.getHeight()));
        return;
    }
    if (draggingSelection)
    {
        const double t = juce::jlimit (0.0, 1.0, (double) (e.x - waveArea.getX()) / waveArea.getWidth()) * processor.getCaptureSeconds();
        processor.setSelection (dragAnchor, t);
        return;
    }
    // Drag-out starts only from the DRAG TO DAW handle or the result waveform, and only the exact rendered file.
    if (! dragStarted && processor.hasRender() && e.getDistanceFromDragStart() > 6
        && (dragArea.contains (pressPoint) || renderArea.contains (pressPoint)))
    {
        const auto file = processor.getRenderInfo().file;
        if (file.existsAsFile())
        {
            dragStarted = true;
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
        }
    }
}

void StutterLabAudioProcessorEditor::mouseUp (const juce::MouseEvent&)
{
    if (draggingSteps)
        for (int i = 0; i < 16; ++i)
            if (touchedSteps & (1u << i))
                if (auto* prm = processor.getParameters().getParameter ("step" + juce::String (i + 1))) prm->endChangeGesture();
    draggingSteps = false; touchedSteps = 0;
    draggingSelection = false; dragStarted = false;
}

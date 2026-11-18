#include <cstring>
#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Background render worker: one job at a time, newest job wins, cooperative cancel.
class StutterLabAudioProcessor::RenderWorker final : public juce::Thread
{
public:
    explicit RenderWorker (StutterLabAudioProcessor& o) : juce::Thread ("STUTTER LAB render"), owner (o) {}
    ~RenderWorker() override
    {
        owner.cancelFlag.store (true, std::memory_order_release);
        signalThreadShouldExit();
        wake.signal();
        // The processor owns every object used by this worker. Never allow the
        // thread to outlive the processor if a long render exceeds an arbitrary timeout.
        waitForThreadToExit (-1);
    }

    void post (Job&& j)
    {
        j.cancelEpoch = owner.cancelEpoch.fetch_add (1, std::memory_order_acq_rel) + 1;
        owner.cancelFlag.store (true, std::memory_order_release);
        { const juce::ScopedLock sl (lock); pending = std::make_unique<Job> (std::move (j)); }
        wake.signal();
    }
    void cancelAll()
    {
        owner.cancelEpoch.fetch_add (1, std::memory_order_acq_rel);
        owner.cancelFlag.store (true, std::memory_order_release);
        const juce::ScopedLock sl (lock); pending.reset();
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            wake.wait (200);
            if (threadShouldExit()) break;
            std::unique_ptr<Job> job;
            { const juce::ScopedLock sl (lock); job = std::move (pending); }
            if (job == nullptr) continue;

            if (job->cancelEpoch != owner.cancelEpoch.load (std::memory_order_acquire)) continue;
            owner.cancelFlag.store (false, std::memory_order_release);
            if (job->cancelEpoch != owner.cancelEpoch.load (std::memory_order_acquire))
                owner.cancelFlag.store (true, std::memory_order_release);
            cray::RenderStats stats;
            auto audio = cray::renderStutter (job->selection, job->settings, &stats, &owner.cancelFlag, &owner.renderProgress);
            if (audio.empty() || threadShouldExit())
            {
                // cancelled (or invalid): only report if no newer job is waiting
                const juce::ScopedLock sl (lock);
                if (pending == nullptr && owner.renderState.load() == RenderState::Rendering)
                {
                    owner.renderProgress.store (0.0f, std::memory_order_release);
                    owner.renderState.store (RenderState::Idle);
                }
                continue;
            }
            owner.publishRender (std::move (audio), stats, job->id);
        }
    }

private:
    StutterLabAudioProcessor& owner;
    juce::CriticalSection lock;
    std::unique_ptr<Job> pending;
    juce::WaitableEvent wake;
};

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout StutterLabAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using API = juce::AudioParameterInt;
    using APB = juce::AudioParameterBool;
    using APC = juce::AudioParameterChoice;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<API> ("repeats", "Repeats", 2, 32, 8));
    layout.add (std::make_unique<APC> ("division", "Rate", juce::StringArray { "1/2", "1/4", "1/8", "1/8 D", "1/8 T", "1/16", "1/16 D", "1/16 T", "1/32", "1/32 T", "1/64" }, 5));
    layout.add (std::make_unique<APC> ("direction", "Direction", juce::StringArray { "Forward", "Reverse", "Ping-Pong", "Random" }, 0));
    layout.add (std::make_unique<APB> ("scan", "Scan Source", false));
    layout.add (std::make_unique<APF> ("swing", "Swing", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.05f));
    layout.add (std::make_unique<APF> ("gate", "Gate", juce::NormalisableRange<float> (0.05f, 1.0f, 0.01f), 0.82f));
    layout.add (std::make_unique<APF> ("humanize", "Humanize", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.04f));
    layout.add (std::make_unique<APF> ("drive", "Drive", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.12f));
    layout.add (std::make_unique<APB> ("phone", "Phone", false));
    layout.add (std::make_unique<APF> ("pitch", "Pitch", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<APF> ("formant", "Formant", juce::NormalisableRange<float> (-6.0f, 6.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<APC> ("delayDivision", "Delay Rate", juce::StringArray { "1/2", "1/4", "1/8", "1/8 D", "1/8 T", "1/16", "1/16 D", "1/16 T", "1/32", "1/32 T", "1/64" }, 2));
    layout.add (std::make_unique<APF> ("delayMix", "Delay Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.08f));
    layout.add (std::make_unique<APF> ("delayFeedback", "Delay Feedback", juce::NormalisableRange<float> (0.0f, 0.85f, 0.01f), 0.28f));
    layout.add (std::make_unique<APF> ("reverbMix", "Reverb Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.07f));
    layout.add (std::make_unique<APF> ("reverbSize", "Reverb Size", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
    layout.add (std::make_unique<APF> ("bpm", "Manual BPM", juce::NormalisableRange<float> (40.0f, 300.0f, 0.1f), 120.0f));
    layout.add (std::make_unique<APB> ("loop", "Loop Preview", true));
    // pattern (0 = rest, otherwise the accent level of that step)
    layout.add (std::make_unique<API> ("steps", "Pattern Length", 1, 16, 8));
    for (int i = 0; i < 16; ++i)
        layout.add (std::make_unique<APF> ("step" + juce::String (i + 1), "Step " + juce::String (i + 1), juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));
    layout.add (std::make_unique<APF> ("intensity", "Intensity", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));
    // movement
    layout.add (std::make_unique<APF> ("rampStart", "Level Start", juce::NormalisableRange<float> (0.0f, 1.5f, 0.01f), 1.0f));
    layout.add (std::make_unique<APF> ("rampEnd", "Level End", juce::NormalisableRange<float> (0.0f, 1.5f, 0.01f), 1.0f));
    layout.add (std::make_unique<APF> ("rampCurve", "Level Curve", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    layout.add (std::make_unique<APF> ("fadeIn", "Fade In", juce::NormalisableRange<float> (0.0f, 2000.0f, 1.0f, 0.5f), 0.0f));
    layout.add (std::make_unique<APF> ("fadeOut", "Fade Out", juce::NormalisableRange<float> (0.0f, 2000.0f, 1.0f, 0.5f), 0.0f));
    // filter / delay / reverb
    layout.add (std::make_unique<APB> ("filterOn", "Filter On", false));
    layout.add (std::make_unique<APC> ("filterType", "Filter Type", juce::StringArray { "Low-pass", "High-pass" }, 0));
    layout.add (std::make_unique<APF> ("filterCutoff", "Cutoff", juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.3f), 8000.0f));
    layout.add (std::make_unique<APF> ("filterRes", "Resonance", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    layout.add (std::make_unique<APB> ("delayOn", "Delay On", true));
    layout.add (std::make_unique<APB> ("reverbOn", "Reverb On", true));
    layout.add (std::make_unique<APF> ("reverbDecay", "Reverb Decay", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
    return layout;
}

StutterLabAudioProcessor::StutterLabAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, &undoManager, "STUTTER_LAB_PARAMETERS", createParameterLayout())
{
    // Start the render thread lazily on the first render request. Hosts create and
    // destroy processor instances during scans; they should not spawn idle threads.
}

StutterLabAudioProcessor::~StutterLabAudioProcessor()
{
    worker.reset();                                  // joins the thread before members die
}

void StutterLabAudioProcessor::prepareToPlay (double sampleRate, int)
{
    capture.prepare (sampleRate, 60.0);              // resamples an existing take if the host rate changed
    preview.prepare (sampleRate);
    auto* empty = new cray::PreviewBuffer();         // a render made at another rate must not play back
    preview.submit (empty);
    preview.stop();
    preview.collectGarbage();
    {
        const juce::ScopedLock sl (infoLock);
        info = {}; // a render from the previous sample rate is no longer auditionable
    }
    renderId.store (0, std::memory_order_release);
    renderProgress.store (0.0f, std::memory_order_release);
    renderState.store (RenderState::Idle);
    const double duration = getCaptureSeconds();
    if (duration > 0.0)
    {
        const double a = juce::jlimit (0.0, duration, selectionStart.load());
        double b = juce::jlimit (a, duration, selectionEnd.load());
        if (b <= a) b = duration;
        selectionStart.store (a);
        selectionEnd.store (b);
    }
    else
    {
        selectionStart.store (0.0);
        selectionEnd.store (0.5);
    }
}

bool StutterLabAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo()) && in == layouts.getMainOutputChannelSet();
}

void StutterLabAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    bool foundHostTempo = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
            {
                hostBpm.store ((float) *bpm, std::memory_order_relaxed);
                foundHostTempo = true;
            }
    hostBpmValid.store (foundHostTempo, std::memory_order_relaxed);

    syncParametersToAtomics();
    const int n = buffer.getNumSamples();
    float* l = buffer.getWritePointer (0);
    float* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    // input meter (pre-capture, pre-preview)
    float pk = 0.0f;
    for (int i = 0; i < n; ++i) pk = juce::jmax (pk, std::abs (l[i]), r ? std::abs (r[i]) : 0.0f);
    if (pk > inputPeak.load (std::memory_order_relaxed)) inputPeak.store (pk, std::memory_order_relaxed);
    if (pk >= 0.999f) clipFlag.store (true, std::memory_order_relaxed);

    capture.process (l, r, n);                       // dry signal -> capture store (if recording)
    preview.setLoop (loopPreview.load (std::memory_order_relaxed));
    preview.process (l, r, n);                       // crossfades to the rendered buffer only while previewing
    float op = 0.0f;                                 // output meter (post-preview)
    for (int i = 0; i < n; ++i) op = juce::jmax (op, std::abs (l[i]), r ? std::abs (r[i]) : 0.0f);
    if (op > outputPeak.load (std::memory_order_relaxed)) outputPeak.store (op, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* StutterLabAudioProcessor::createEditor() { return new StutterLabAudioProcessorEditor (*this); }

//==============================================================================
void StutterLabAudioProcessor::startCapture()
{
    invalidateRender();
    capture.requestStart();
}
void StutterLabAudioProcessor::stopCapture() { capture.requestStop(); }

juce::File StutterLabAudioProcessor::getAudioLibraryDirectory (const juce::String& subfolder) const
{
    auto root = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                    .getChildFile ("CRAY LDR").getChildFile ("STUTTER LAB").getChildFile ("Audio");
    if (subfolder.isNotEmpty()) root = root.getChildFile (subfolder);
    root.createDirectory();
    return root;
}

bool StutterLabAudioProcessor::archiveCapture (juce::File& savedFile, juce::String& error)
{
    if (capture.isRecording()) { error = "Stop the capture before archiving it."; return false; }
    cray::StereoBuffer take;
    if (! capture.snapshotAll (take) || take.empty()) { error = "There is no captured audio to save."; return false; }

    const auto folder = getAudioLibraryDirectory ("Captures");
    if (! folder.isDirectory()) { error = "Could not create the STUTTER LAB Audio/Captures folder."; return false; }

    static std::atomic<int> captureCounter { 0 };
    const auto stamp = juce::Time::getCurrentTime().formatted ("%Y%m%d_%H%M%S");
    savedFile = folder.getNonexistentChildFile ("STUTTER_CAPTURE_" + stamp + "_" + juce::String (++captureCounter), ".wav", false);
    const auto bytes = cray::encodeWav (take, cray::WavFormat::Pcm24);
    juce::FileOutputStream out (savedFile);
    if (! out.openedOk() || ! out.write (bytes.data(), bytes.size()))
    {
        savedFile.deleteFile();
        error = "Could not save the captured WAV in " + folder.getFullPathName();
        return false;
    }
    out.flush();
    if (! out.getStatus().wasOk())
    {
        savedFile.deleteFile();
        error = "The captured WAV could not be written completely.";
        return false;
    }
    return true;
}

void StutterLabAudioProcessor::clearCapture()
{
    invalidateRender();
    capture.clear();
    selectionStart.store (0.0); selectionEnd.store (0.5);
}

void StutterLabAudioProcessor::invalidateRender()
{
    nextJobId.fetch_add (1, std::memory_order_acq_rel); // invalidate any worker result already in flight
    cancelRender();
    preview.stop();
    {
        const juce::ScopedLock sl (infoLock);
        info = {};
        originalAudio = {};
        processedAudio = {};
    }
    renderProgress.store (0.0f, std::memory_order_release);
    renderId.fetch_add (1, std::memory_order_acq_rel);
    renderState.store (RenderState::Idle, std::memory_order_release);
}

void StutterLabAudioProcessor::setSelection (double a, double b)
{
    const double dur = getCaptureSeconds();
    const double lo = juce::jlimit (0.0, dur, juce::jmin (a, b));
    const double hi = juce::jlimit (0.0, dur, juce::jmax (a, b));
    selectionStart.store (lo);
    selectionEnd.store (juce::jmax (lo + 0.005, hi));
}

void StutterLabAudioProcessor::syncParametersToAtomics()
{
    repeats.store ((int) parameters.getRawParameterValue ("repeats")->load());
    division.store ((int) parameters.getRawParameterValue ("division")->load());
    direction.store ((int) parameters.getRawParameterValue ("direction")->load());
    scanSelection.store (parameters.getRawParameterValue ("scan")->load() >= 0.5f);
    swing.store (parameters.getRawParameterValue ("swing")->load());
    gate.store (parameters.getRawParameterValue ("gate")->load());
    humanize.store (parameters.getRawParameterValue ("humanize")->load());
    saturation.store (parameters.getRawParameterValue ("drive")->load());
    phoneMode.store (parameters.getRawParameterValue ("phone")->load() >= 0.5f);
    pitchSemitones.store (parameters.getRawParameterValue ("pitch")->load());
    formantSemitones.store (parameters.getRawParameterValue ("formant")->load());
    delayDivision.store ((int) parameters.getRawParameterValue ("delayDivision")->load());
    delayMix.store (parameters.getRawParameterValue ("delayMix")->load());
    delayFeedback.store (parameters.getRawParameterValue ("delayFeedback")->load());
    reverbMix.store (parameters.getRawParameterValue ("reverbMix")->load());
    reverbSize.store (parameters.getRawParameterValue ("reverbSize")->load());
    manualBpm.store (parameters.getRawParameterValue ("bpm")->load());
    loopPreview.store (parameters.getRawParameterValue ("loop")->load() >= 0.5f);
}

void StutterLabAudioProcessor::syncAtomicsToParameters()
{
    auto set = [this] (const juce::String& id, float value)
    {
        if (auto* p = parameters.getParameter (id)) p->setValue (p->convertTo0to1 (value));
    };
    set ("repeats", (float) repeats.load()); set ("division", (float) division.load()); set ("direction", (float) direction.load());
    set ("scan", scanSelection.load() ? 1.0f : 0.0f); set ("swing", swing.load()); set ("gate", gate.load());
    set ("delayDivision", (float) delayDivision.load());
    set ("humanize", humanize.load()); set ("drive", saturation.load()); set ("phone", phoneMode.load() ? 1.0f : 0.0f);
    set ("pitch", pitchSemitones.load()); set ("formant", formantSemitones.load()); set ("delayMix", delayMix.load());
    set ("delayFeedback", delayFeedback.load()); set ("reverbMix", reverbMix.load()); set ("reverbSize", reverbSize.load());
    set ("bpm", manualBpm.load()); set ("loop", loopPreview.load() ? 1.0f : 0.0f);
}

void StutterLabAudioProcessor::loadFactoryPreset (int index)
{
    auto set = [this] (const juce::String& id, float value)
    {
        if (auto* p = parameters.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (value));
    };
    // Pattern string: one char per step. '.' = rest, '1'..'9' = accent level (9 = full).
    auto pattern = [&set] (const char* str)
    {
        const int n = (int) std::strlen (str);
        set ("steps", (float) juce::jlimit (1, 16, n));
        for (int i = 0; i < 16; ++i)
        {
            const char c = i < n ? str[i] : '9';
            set ("step" + juce::String (i + 1), c == '.' ? 0.0f : juce::jlimit (0.1f, 1.0f, (float) (c - '0') / 9.0f));
        }
    };
    // 1) neutral baseline: every preset starts from the same state, so none inherits leftovers
    set ("scan", 0); set ("repeats", 8); set ("division", 5); set ("direction", 0); set ("swing", 0); set ("gate", 1);
    set ("humanize", 0); set ("drive", 0); set ("phone", 0); set ("pitch", 0); set ("formant", 0);
    set ("delayOn", 0); set ("delayDivision", 5); set ("delayMix", 0); set ("delayFeedback", 0.28f);
    set ("reverbOn", 0); set ("reverbMix", 0); set ("reverbSize", 0.5f); set ("reverbDecay", 0.5f);
    set ("filterOn", 0); set ("filterType", 0); set ("filterCutoff", 8000); set ("filterRes", 0);
    set ("rampStart", 1); set ("rampEnd", 1); set ("rampCurve", 0); set ("fadeIn", 0); set ("fadeOut", 0); set ("intensity", 1);
    pattern ("99999999");

    switch (juce::jlimit (0, 8, index))
    {
        case 0: break;   // Init
        case 1: // Tight Stutter: short, defined 1/16 repeats with a firm gate
            set ("repeats", 8); set ("division", 5); set ("gate", 0.62f); set ("swing", 0.05f); set ("drive", 0.1f);
            set ("fadeOut", 40); pattern ("99999999"); break;
        case 2: // Trap Rolls: 1/32 rolls that accelerate in level, accent on the downbeat
            set ("repeats", 16); set ("division", 8); set ("gate", 0.8f); set ("humanize", 0.12f);
            set ("rampStart", 0.45f); set ("rampEnd", 1.0f); set ("rampCurve", 0.35f);
            pattern ("9657965795679657"); set ("intensity", 0.8f);
            set ("filterOn", 1); set ("filterType", 1); set ("filterCutoff", 180); break;
        case 3: // Vocal Glitch: triplets, random direction, a little delay
            set ("repeats", 12); set ("division", 7); set ("direction", 3); set ("gate", 0.5f); set ("humanize", 0.25f);
            set ("swing", 0.15f); pattern ("9.99.9.9"); set ("intensity", 0.9f);
            set ("delayOn", 1); set ("delayDivision", 4); set ("delayMix", 0.2f); set ("delayFeedback", 0.35f);
            set ("drive", 0.18f); break;
        case 4: // Reverse Pulse: reversed 1/8 slices breathing with a soft reverb
            set ("repeats", 6); set ("division", 2); set ("direction", 1); set ("gate", 0.9f);
            pattern ("9.7.9.7."); set ("fadeOut", 120);
            set ("reverbOn", 1); set ("reverbMix", 0.22f); set ("reverbDecay", 0.55f); set ("reverbSize", 0.55f); break;
        case 5: // Progressive Riser: 1/16 -> a long rise in level with a filter opening on top
            set ("repeats", 16); set ("division", 5); set ("gate", 0.85f);
            set ("rampStart", 0.1f); set ("rampEnd", 1.2f); set ("rampCurve", 0.5f); set ("fadeIn", 200);
            set ("filterOn", 1); set ("filterType", 1); set ("filterCutoff", 700); set ("filterRes", 0.25f);
            set ("reverbOn", 1); set ("reverbMix", 0.18f); set ("reverbDecay", 0.7f); set ("reverbSize", 0.7f); break;
        case 6: // Rhythmic Gate: 1/16 on/off pattern, clean cut-offs
            set ("repeats", 16); set ("division", 5); set ("gate", 0.5f);
            pattern ("9.99.9.99.99.9.9"); set ("fadeOut", 30); break;
        case 7: // Fast Repeats: 1/32 repeats, descending level
            set ("repeats", 24); set ("division", 8); set ("gate", 0.9f);
            set ("rampStart", 1.0f); set ("rampEnd", 0.3f); set ("fadeOut", 80); set ("drive", 0.08f); break;
        case 8: // Broken Rhythm: mixed accents, swing, humanize and a dotted delay
            set ("repeats", 16); set ("division", 5); set ("gate", 0.7f); set ("swing", 0.35f); set ("humanize", 0.3f);
            pattern ("9.5.97.39.5.7.9."); set ("intensity", 1.0f);
            set ("delayOn", 1); set ("delayDivision", 6); set ("delayMix", 0.22f); set ("delayFeedback", 0.4f); break;
    }
    syncParametersToAtomics();
}

void StutterLabAudioProcessor::storeAB (bool storeB)
{
    if (storeB) { stateB = parameters.copyState(); hasStateB = true; }
    else { stateA = parameters.copyState(); hasStateA = true; }
}

void StutterLabAudioProcessor::toggleAB()
{
    if (! hasStateA || ! hasStateB) return;
    parameters.replaceState (showingStateB ? stateA.createCopy() : stateB.createCopy());
    showingStateB = ! showingStateB;
    syncParametersToAtomics();
}

bool StutterLabAudioProcessor::undoParameterChange()
{
    if (! undoManager.canUndo()) return false;
    undoManager.undo();
    syncParametersToAtomics();
    return true;
}

bool StutterLabAudioProcessor::saveUserPreset (const juce::File& file, juce::String& error)
{
    auto xml = parameters.copyState().createXml();
    if (xml == nullptr || ! xml->writeTo (file, {})) { error = "Could not save preset file."; return false; }
    return true;
}

bool StutterLabAudioProcessor::loadUserPreset (const juce::File& file, juce::String& error)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr) { error = "Invalid preset XML."; return false; }
    const auto tree = juce::ValueTree::fromXml (*xml);
    if (! tree.isValid() || tree.getType() != parameters.state.getType()) { error = "Preset is not a STUTTER LAB preset."; return false; }
    parameters.replaceState (tree);
    syncParametersToAtomics();
    return true;
}

double StutterLabAudioProcessor::getEffectiveBpm() const
{
    const double v = hostBpmValid.load() ? (double) hostBpm.load() : (double) manualBpm.load();
    return juce::jlimit (20.0, 400.0, v);
}

//==============================================================================
bool StutterLabAudioProcessor::generate (juce::String& error)
{
    syncParametersToAtomics();
    if (capture.isRecording()) { error = "Stop the capture before generating."; return false; }
    const double sr = capture.sampleRate();
    const size_t total = (size_t) capture.numSamples();
    if (total < 64) { error = "Capture a phrase first."; return false; }

    const size_t a = (size_t) juce::jlimit<double> (0.0, (double) total - 1.0, std::floor (selectionStart.load() * sr));
    const size_t b = (size_t) juce::jlimit<double> ((double) a + 1.0, (double) total, std::ceil (selectionEnd.load() * sr));

    Job job;
    if (! capture.snapshot (job.selection, a, b)) { error = "Could not read the capture."; return false; }
    { const juce::ScopedLock sl (infoLock); originalAudio = job.selection; }
    auto& s = job.settings;
    s.bpm = getEffectiveBpm();
    s.saturation = saturation.load();
    s.phone = phoneMode.load();
    s.pitchSemitones = pitchSemitones.load(); s.formantSemitones = formantSemitones.load();
    s.delayDivision = delayDivision.load(); s.delayMix = delayMix.load(); s.delayFeedback = delayFeedback.load();
    s.reverbMix = reverbMix.load(); s.reverbSize = reverbSize.load();
    s.stutter.repeats = repeats.load();
    s.stutter.division = juce::jlimit (0, cray::kNumDivisions - 1, division.load());
    s.stutter.direction = (cray::Direction) juce::jlimit (0, 3, direction.load());
    s.stutter.scan = scanSelection.load();
    s.stutter.swing = swing.load(); s.stutter.gate = gate.load(); s.stutter.humanize = humanize.load();
    s.stutter.seed = (uint32_t) seed.load();
    {
        auto v = [this] (const juce::String& id) { return parameters.getRawParameterValue (id)->load(); };
        s.stutter.patternLength = juce::jlimit (1, 16, (int) v ("steps"));
        s.stutter.intensity = v ("intensity");
        s.stutter.stepMask = 0;
        for (int i = 0; i < 16; ++i)
        {
            const float lv = v ("step" + juce::String (i + 1));
            s.stutter.stepLevel[i] = lv;
            if (lv > 0.001f) s.stutter.stepMask = (uint16_t) (s.stutter.stepMask | (1u << i));
        }
        s.rampStart = v ("rampStart"); s.rampEnd = v ("rampEnd"); s.rampCurve = v ("rampCurve");
        s.fadeInMs = v ("fadeIn"); s.fadeOutMs = v ("fadeOut");
        s.filterOn = v ("filterOn") >= 0.5f; s.filterType = (int) v ("filterType");
        s.filterCutoff = v ("filterCutoff"); s.filterResonance = v ("filterRes");
        s.delayOn = v ("delayOn") >= 0.5f; s.reverbOn = v ("reverbOn") >= 0.5f; s.reverbDecay = v ("reverbDecay");
    }
    job.id = nextJobId.fetch_add (1);

    const double stepSamples = cray::divisionSamples (s.stutter.division, s.bpm, sr);
    double estimate = ((double) s.stutter.repeats + 0.5) * stepSamples / sr; // swing + humanize headroom
    double delayTail = 0.0, reverbTail = 0.0;
    if (s.delayOn && s.delayMix > 0.001f)
    {
        const double delaySamples = cray::divisionSamples (s.delayDivision, s.bpm, sr);
        const double fb = juce::jlimit (0.0, 0.85, (double) s.delayFeedback);
        const double repeatsToSilence = fb > 0.001 ? std::ceil (std::log (0.001) / std::log (fb)) : 1.0;
        delayTail = std::min (6.0, delaySamples * std::max (1.0, repeatsToSilence) / sr);
    }
    if (s.reverbOn && s.reverbMix > 0.001f)
        reverbTail = 2.0 + 3.0 * juce::jlimit (0.0f, 1.0f, s.reverbDecay);
    estimate += std::min (6.0, delayTail + reverbTail);
    if (estimate > 120.0) { error = "Render including effect tails would exceed 120 s. Reduce repeats, feedback or reverb size."; return false; }

    // Keep an active audition running: the worker will hot-swap its audio buffer
    // at the next audio callback when the new render is ready.
    if (worker == nullptr)
    {
        auto candidate = std::make_unique<RenderWorker> (*this);
        if (! candidate->startThread())
        {
            error = "Could not start the background render thread.";
            renderState.store (RenderState::Failed, std::memory_order_release);
            return false;
        }
        worker = std::move (candidate);
    }

    renderProgress.store (0.0f, std::memory_order_release);
    renderState.store (RenderState::Rendering);
    worker->post (std::move (job));
    return true;
}

void StutterLabAudioProcessor::cancelRender()
{
    if (worker != nullptr) worker->cancelAll();
    if (renderState.load() == RenderState::Rendering)
    {
        renderProgress.store (0.0f, std::memory_order_release);
        renderState.store (RenderState::Idle);
    }
}

void StutterLabAudioProcessor::publishRender (cray::StereoBuffer&& audio, const cray::RenderStats& stats, uint64_t id)
{
    if (id + 1 != nextJobId.load (std::memory_order_acquire)) return;
    // 1) waveform overview
    constexpr int bins = 1024;
    std::vector<float> peaks ((size_t) bins * 2, 0.0f);
    for (int x = 0; x < bins; ++x)
    {
        const size_t a = audio.size() * (size_t) x / bins, e = juce::jmax (a + 1, audio.size() * (size_t) (x + 1) / bins);
        float lo = 0, hi = 0;
        for (size_t i = a; i < juce::jmin (e, audio.size()); ++i) { const float m = 0.5f * (audio.l[i] + audio.r[i]); lo = juce::jmin (lo, m); hi = juce::jmax (hi, m); }
        peaks[(size_t) x * 2] = lo; peaks[(size_t) x * 2 + 1] = hi;
    }
    // 2) unique WAV on disk (never overwrites a file a DAW may be importing)
    const auto file = makeUniqueRenderFile();
    const auto bytes = cray::encodeWav (audio, cray::WavFormat::Pcm24);
    bool wrote = false;
    {
        juce::FileOutputStream out (file);
        if (out.openedOk()) { wrote = out.write (bytes.data(), bytes.size()); out.flush(); }
    }
    if (! wrote)
    {
        const juce::ScopedLock sl (infoLock);
        info = {}; info.message = "Could not write the render WAV to " + file.getParentDirectory().getFullPathName();
        renderState.store (RenderState::Failed);
        return;
    }
    // 3) Publish atomically with respect to capture invalidation. A render that
    // finished after a new capture began must never replace the new take's result.
    {
        const juce::ScopedLock sl (infoLock);
        if (id + 1 != nextJobId.load (std::memory_order_acquire))
        {
            file.deleteFile();
            return;
        }
        processedAudio = audio;
        info.file = file; info.seconds = stats.seconds; info.peak = stats.peak; info.sampleRate = audio.sampleRate;
        info.slices = (int) stats.slices; info.bitDepth = 24; info.id = id; info.peaks = std::move (peaks);
        info.message = {};
        auto* pb = new cray::PreviewBuffer();
        pb->audio = audio;
        pb->keepPosition = preview.isPlaying();
        preview.submit (pb);
        renderProgress.store (1.0f, std::memory_order_release);
        renderId.store (id, std::memory_order_release);
        renderState.store (RenderState::Ready, std::memory_order_release);
    }
}

StutterLabAudioProcessor::RenderInfo StutterLabAudioProcessor::getRenderInfo() const
{
    const juce::ScopedLock sl (infoLock);
    return info;
}

void StutterLabAudioProcessor::startPreview (bool original)
{
    if (! hasRender()) return;
    auto* pb = new cray::PreviewBuffer();
    {
        const juce::ScopedLock sl (infoLock);
        pb->audio = original ? originalAudio : processedAudio;   // processed == exactly what Export writes
    }
    if (pb->audio.empty()) { delete pb; return; }
    preview.submit (pb);
    preview.collectGarbage();
    preview.play (loopPreview.load());
}

bool StutterLabAudioProcessor::exportRenderTo (const juce::File& destination, juce::String& error) const
{
    const auto src = getRenderInfo().file;
    if (! src.existsAsFile()) { error = "Generate a render first."; return false; }
    destination.getParentDirectory().createDirectory();
    if (! src.copyFileTo (destination)) { error = "Could not write " + destination.getFullPathName(); return false; }
    return true;
}

juce::File StutterLabAudioProcessor::makeUniqueRenderFile() const
{
    static std::atomic<int> counter { 0 };
    auto dir = getAudioLibraryDirectory ("Renders");
    const auto stamp = juce::Time::getCurrentTime().formatted ("%Y%m%d_%H%M%S");
    return dir.getNonexistentChildFile ("STUTTER_LAB_" + stamp + "_" + juce::String (++counter), ".wav", false);
}


//==============================================================================
// State: v1-v4 are migrated; v5 adds compressed capture persistence.
void StutterLabAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    syncParametersToAtomics();
    juce::MemoryOutputStream out (dest, true);
    out.writeInt (7);
    out.writeInt (repeats.load()); out.writeInt (division.load()); out.writeInt (direction.load());
    out.writeBool (scanSelection.load());
    out.writeFloat (swing.load()); out.writeFloat (gate.load()); out.writeFloat (humanize.load());
    out.writeFloat (saturation.load()); out.writeBool (phoneMode.load());
    out.writeFloat (pitchSemitones.load()); out.writeFloat (formantSemitones.load());
    out.writeInt (delayDivision.load());
    out.writeFloat (delayMix.load()); out.writeFloat (delayFeedback.load());
    out.writeFloat (reverbMix.load()); out.writeFloat (reverbSize.load());
    out.writeInt (seed.load()); out.writeFloat (manualBpm.load()); out.writeBool (loopPreview.load());
    out.writeDouble (selectionStart.load()); out.writeDouble (selectionEnd.load());

    // Persist the captured take with the project/session state. Compress a compact
    // float payload so a typical vocal take remains manageable in a DAW state chunk.
    cray::StereoBuffer take;
    if (capture.snapshotAll (take) && ! take.empty())
    {
        juce::MemoryOutputStream raw;
        raw.writeDouble (take.sampleRate);
        raw.writeInt ((int) take.size());
        for (size_t i = 0; i < take.size(); ++i) { raw.writeFloat (take.l[i]); raw.writeFloat (take.r[i]); }
        juce::MemoryOutputStream compressed;
        {
            juce::GZIPCompressorOutputStream gzip (compressed, 6, juce::GZIPCompressorOutputStream::windowBitsGZIP);
            gzip.write (raw.getData(), raw.getDataSize());
            gzip.flush();
        }
        out.writeInt ((int) compressed.getDataSize());
        out.write (compressed.getData(), compressed.getDataSize());
    }
    else out.writeInt (0);
    // v7: full parameter tree, so pattern, movement and filter settings survive project recall
    if (auto xml = parameters.copyState().createXml()) out.writeString (xml->toString (juce::XmlElement::TextFormat().singleLine()));
    else out.writeString ({});
}

void StutterLabAudioProcessor::setStateInformation (const void* data, int size)
{
    // Hosts may pass an empty, truncated or stale state block during project recovery.
    // Reject impossible chunks before constructing a stream or mutating processor state.
    constexpr int maxStateBytes = 128 * 1024 * 1024;
    if (data == nullptr || size < (int) sizeof (int) || size > maxStateBytes)
        return;

    juce::MemoryInputStream in (data, (size_t) size, false);
    const int version = in.readInt();
    if (version < 1 || version > 7) return;

    // Minimum remaining payload for each legacy format, including the selection range
    // and any version-specific fields that must be present before they are read.
    const int minimumPayloadBytes = version <= 2 ? (version == 1 ? 25 : 30)
                                  : version == 3 ? 55
                                  : version == 4 ? 79
                                  : version == 5 ? 83
                                  : version == 6 ? 87
                                  : 91;
    if (in.getNumBytesRemaining() < minimumPayloadBytes)
        return;
    if (version <= 2)
    {
        repeats.store (juce::jlimit (2, 32, in.readInt()));
        division.store (cray::divisionFromLegacy (in.readInt()));
        direction.store (in.readBool() ? (int) cray::Direction::Reverse : (int) cray::Direction::Forward);
        if (version == 2) { saturation.store (juce::jlimit (0.0f, 1.0f, in.readFloat())); phoneMode.store (in.readBool()); }
    }
    else
    {
        repeats.store (juce::jlimit (2, 32, in.readInt()));
        division.store (juce::jlimit (0, cray::kNumDivisions - 1, in.readInt()));
        direction.store (juce::jlimit (0, 3, in.readInt()));
        scanSelection.store (in.readBool());
        swing.store (juce::jlimit (0.0f, 1.0f, in.readFloat()));
        gate.store (juce::jlimit (0.05f, 1.0f, in.readFloat()));
        humanize.store (juce::jlimit (0.0f, 1.0f, in.readFloat()));
        saturation.store (juce::jlimit (0.0f, 1.0f, in.readFloat()));
        phoneMode.store (in.readBool());
        if (version >= 4)
        {
            pitchSemitones.store (juce::jlimit (-12.0f, 12.0f, in.readFloat()));
            formantSemitones.store (juce::jlimit (-6.0f, 6.0f, in.readFloat()));
            if (version >= 6) delayDivision.store (juce::jlimit (0, cray::kNumDivisions - 1, in.readInt()));
            delayMix.store (juce::jlimit (0.0f, 1.0f, in.readFloat()));
            delayFeedback.store (juce::jlimit (0.0f, 0.85f, in.readFloat()));
            reverbMix.store (juce::jlimit (0.0f, 1.0f, in.readFloat()));
            reverbSize.store (juce::jlimit (0.0f, 1.0f, in.readFloat()));
        }
        seed.store (in.readInt());
        manualBpm.store (juce::jlimit (20.0f, 400.0f, in.readFloat()));
        loopPreview.store (in.readBool());
    }
    const double a = juce::jmax (0.0, in.readDouble());
    selectionStart.store (a);
    selectionEnd.store (juce::jmax (a + 0.005, in.readDouble()));
    if (version >= 5 && in.getNumBytesRemaining() >= 4)
    {
        const int compressedSize = juce::jlimit (0, 128 * 1024 * 1024, in.readInt());
        if (compressedSize > 0 && compressedSize <= 64 * 1024 * 1024
            && compressedSize <= in.getNumBytesRemaining())
        {
            juce::MemoryBlock compressed;
            compressed.setSize ((size_t) compressedSize);
            in.read (compressed.getData(), (size_t) compressedSize);
            juce::MemoryInputStream compressedInput (compressed, false);
            juce::GZIPDecompressorInputStream gzip (&compressedInput, false, juce::GZIPDecompressorInputStream::gzipFormat);
            juce::MemoryBlock raw;
            if (gzip.readIntoMemoryBlock (raw, (ssize_t) (12 + (size_t) 60 * 192000 * 8)) && raw.getSize() >= 12)
            {
                juce::MemoryInputStream payload (raw, false);
                cray::StereoBuffer take;
                take.sampleRate = payload.readDouble();
                const int samples = juce::jlimit (0, 60 * 192000, payload.readInt());
                if (take.sampleRate >= 8000.0 && take.sampleRate <= 384000.0 && samples > 0
                    && (size_t) samples <= (raw.getSize() - 12) / 8)
                {
                    take.resize ((size_t) samples);
                    for (int i = 0; i < samples; ++i) { take.l[(size_t) i] = payload.readFloat(); take.r[(size_t) i] = payload.readFloat(); }
                    capture.load (take);
                    selectionStart.store (juce::jlimit (0.0, take.seconds(), selectionStart.load()));
                    selectionEnd.store (std::min (take.seconds(), juce::jmax (selectionStart.load() + 0.005, selectionEnd.load())));
                }
            }
        }
    }
    if (version >= 7 && in.getNumBytesRemaining() >= 4)
    {
        const auto xmlText = in.readString();
        if (xmlText.getNumBytesAsUTF8() <= 1024 * 1024)
        if (auto xml = juce::XmlDocument::parse (xmlText))
        {
            const auto tree = juce::ValueTree::fromXml (*xml);
            if (tree.isValid() && tree.getType() == parameters.state.getType()) parameters.replaceState (tree);
        }
    }
    syncAtomicsToParameters();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new StutterLabAudioProcessor(); }

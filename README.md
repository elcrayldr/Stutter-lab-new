# CRAY LDR — STUTTER LAB

Capture-based vocal stutter, pitch/formant and ambience processor. Windows x64 VST3, JUCE 8.0.6, C++17, CMake.

## Product workflow

`CAPTURE → SELECT WORD / SYLLABLE → AUTO-RENDER → PREVIEW → DRAG RESULT TO DAW`

- Captures mono or stereo input for up to 60 seconds.
- Stores the captured take in the DAW project state using a compressed payload.
- Displays the captured waveform; drag across it to isolate a word, syllable or transient-sized fragment.
- Builds the stutter automatically after capture stops and re-renders after parameter or selection changes (debounced, background worker).
- The only output action is external drag-and-drop of the finished 24-bit WAV directly into FL Studio, Cubase or another DAW. No export/save-as workflow is exposed in the editor.
- Preview auditions the exact completed render, with a short transition ramp to avoid clicks.
- Rhythmic engine: repeat count, tempo division, gate, swing, humanize, step accents/rests, forward/reverse/ping-pong/random playback and scan across the selection.
- Creative tone/movement: drive, pitch/formant, resonant filter, tempo-synchronised ping-pong delay, reverb, level ramp and fades.
- A new capture invalidates the previous result, preventing a stale render from being dragged into the session.
- Host-automatable parameters through `AudioProcessorValueTreeState`.
- Dark, hardware-inspired panel with amber indicators, analog metering, source/result waveforms and a single drag-to-DAW output area.

## Build requirements

- Windows 10/11 x64
- Visual Studio 2022 with **Desktop development with C++**
- CMake 3.22+
- Git and internet access for the first JUCE FetchContent configure

From PowerShell in the repository root:

```powershell
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release -DSTUTTERLAB_BUILD_TESTS=ON
cmake --build build --config Release --target STUTTER_LAB_VST3 StutterLabCoreTests --parallel
ctest --test-dir build -C Release --output-on-failure
```

Expected VST3 bundle:

`build\STUTTER_LAB_artefacts\Release\VST3\STUTTER LAB.vst3`

Copy the complete `.vst3` bundle to:

`C:\Program Files\Common Files\VST3\`

Then rescan in FL Studio 26 or Cubase 13.

## Automated tests

The JUCE-free DSP/capture/preview test target can also be built independently on Linux or Windows:

```bash
cmake -S STUTTER_LAB_VST3/Tests -B build-core -DCMAKE_BUILD_TYPE=Release
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
```

The test suite covers rhythmic divisions, full-selection scanning, gate/swing, fades, deterministic randomisation, cancellation, pitch/formant processing, delay/reverb tails, WAV encoding, capture persistence primitives, exact preview/render sample equality after the safety ramp, 192 kHz processing and concurrent capture/preview stress.

## GitHub Actions

`.github/workflows/windows-vst3.yml` builds the Windows x64 VST3, runs the JUCE-free core tests, verifies the bundle and uploads both the VST3 and a complete source handoff artifact.

## Validation status

Do not treat a successful core test as proof that the JUCE wrapper or VST3 host integration works. The JUCE plugin must still pass the Windows MSVC build and manual validation in FL Studio 26 and Cubase 13. In particular, verify project-state recall with a captured take, auto-render after capture and parameter edits, selection changes, drag-out from both hosts, mono/stereo buses, sample-rate changes and editor close/reopen.

See `Docs/FINAL_VALIDATION_STATUS.md` and `LICENSING.md` before distribution.

# CRAY LDR — StutterLabLdr

Capture-based vocal stutter generator and render-to-DAW tool. Windows x64 VST3, JUCE 8.0.6, C++17, CMake.

## Product workflow

`CAPTURE → SELECT WORD / SYLLABLE → AUTO-RENDER → PREVIEW → DRAG RESULT TO DAW`

- Captures mono or stereo input for up to 60 seconds.
- Stores the captured take in the DAW project state using a compressed payload.
- Displays the captured waveform; drag across it to isolate a word, syllable or transient-sized fragment.
- Builds the stutter automatically after capture stops and re-renders after parameter or selection edits (debounced background worker).
- The output action is external drag-and-drop of the finished 24-bit WAV into FL Studio, Cubase or another DAW.
- Every completed capture is archived as a 24-bit WAV in `Documents/CRAY LDR/STUTTER LAB/Audio/Captures`. Generated renders are retained in the sibling `Renders` folder.
- Preview loops by default. While it is playing, parameter edits rebuild in the background and hot-swap the new audio while preserving relative playhead position with a short de-click ramp.
- The editor is resizable from 960×660 up to 1900×1250.
- Rhythmic engine: repeat count, tempo division, gate, swing, humanize, step accents/rests, forward/reverse/ping-pong/random playback and scan across the selection.
- Creative processing: drive, pitch/formant, resonant filter, tempo-synchronised ping-pong delay, reverb, level ramp and fades.
- A new capture invalidates the previous result, preventing a stale render from being dragged into the session.
- Host-automatable parameters through `AudioProcessorValueTreeState`.
- Dark, hardware-inspired panel with amber indicators, segmented LED metering, source/result waveforms and a single drag-to-DAW output area.

## Build requirements

- Windows 10/11 x64
- Visual Studio 2022 with **Desktop development with C++**
- CMake 3.22+
- Git and internet access for the first JUCE FetchContent configure

From PowerShell in the repository root:

```powershell
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release -DSTUTTERLAB_BUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Expected VST3 bundle:

`build\\STUTTER_LAB_artefacts\\Release\\VST3\\StutterLabLdr.vst3`

Copy the complete `.vst3` bundle to:

`C:\\Program Files\\Common Files\\VST3\\`

Then rescan in FL Studio 26 or Cubase 13.

## Automated validation

GitHub Actions on Windows builds the complete VST3 with MSVC, runs the JUCE-free core tests, validates the VST3 lifecycle/editor/state with pluginval strictness level 5, and uploads the VST3 bundle.

The JUCE-free test target can be built separately:

```powershell
cmake -S . -B build -A x64 -DSTUTTERLAB_BUILD_TESTS=ON
cmake --build build --config Release --target stutter_lab_core_tests --parallel
ctest --test-dir build -C Release --output-on-failure
```

## Validation status

The current source build and pluginval run passed in [workflow run #44](https://github.com/elcrayldr/Stutter-lab-new/actions/runs/44192406911) for commit `0960c292c6be483377d018113bd96893e1d375df`. The subsequent commits on this branch only update documentation; they do not change the validated plugin source. This is not a substitute for manual host testing. Before release, verify FL Studio 26 and Cubase 13 loading, project-state recall with a captured take, auto-render after capture and edits, drag-out import, parameter automation, mono/stereo operation, sample-rate changes, editor close/reopen and CPU/memory stress.

See `FINAL_VALIDATION_STATUS.md` and `TEST_PLAN.md` for the remaining release gates.

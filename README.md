# CRAY LDR — STUTTER LAB

Capture-based vocal stutter, pitch/formant and ambience processor. Windows x64 VST3, JUCE 8.0.6, C++17, CMake.

## Product workflow

`CAPTURE → SELECT → GENERATE → PREVIEW → DRAG TO DAW / EXPORT WAV`

- Captures mono or stereo input for up to 60 seconds.
- Stores the captured take in the DAW project state using a compressed payload.
- Displays source and generated waveforms; drag across the source waveform to select a region.
- Generates on a background worker, reports progress and supports cancellation.
- Processes the full selected region using repeat scanning, rhythmic divisions, reverse, ping-pong, seeded random direction, swing, gate and humanize.
- Offline pitch shift and independent spectral-envelope/formant shift.
- Tempo-synchronised stereo ping-pong delay, comb reverb, phone filtering and drive saturation.
- Preview and WAV export use the same completed render buffer. Preview uses a 4 ms transition ramp to avoid clicks, then reproduces the render samples without additional DSP.
- Host-automatable parameters through `AudioProcessorValueTreeState`.
- Five factory presets, user preset save/load, A/B snapshots and Undo.

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

Copy the complete `.vst3` folder to:

`C:\Program Files\Common Files\VST3\`

Do not copy only the inner binary from `Contents\x86_64-win`. The `.vst3` bundle must remain intact.

## FL Studio installation and routing

STUTTER LAB is a **VST3 effect**, not an instrument. Copy the complete `STUTTER LAB.vst3` folder directly into:

`C:\Program Files\Common Files\VST3\`

Then run **Options → Manage plugins → Find plugins** in FL Studio and insert STUTTER LAB into a Mixer effect slot. Do not load it from the Channel Rack instrument list.

## Automated validation

- JUCE-free DSP/capture/preview core tests.
- Windows x64 MSVC build.
- `pluginval` strictness level 5 lifecycle/state validation.

The test suite covers rhythmic divisions, full-selection scanning, gate/swing, fades, deterministic randomisation, cancellation, pitch/formant processing, delay/reverb tails, WAV encoding, capture persistence primitives, exact preview/render sample equality after the safety ramp, 192 kHz processing and concurrent capture/preview stress.

Passing `pluginval` does not guarantee compatibility with every DAW. Final host acceptance still requires loading the VST3 in FL Studio 26 and Cubase 13, testing mono/stereo routing, state recall, parameter automation, drag-out, sample-rate changes and editor close/reopen.

## GitHub Actions

`.github/workflows/windows-vst3.yml` builds the Windows x64 VST3, runs core tests and `pluginval`, then uploads the complete `.vst3` bundle directly (not a ZIP nested inside another ZIP).


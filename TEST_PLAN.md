# STUTTER LAB 0.5.0 — QA test plan

## 1. Automated build gate — Windows
- [x] Windows x64 configure with Visual Studio 2022 succeeds.
- [x] JUCE 8.0.6 VST3 target compiles in Release.
- [x] JUCE-free core tests build and CTest passes.
- [x] VST3 bundle is generated as `StutterLabLdr.vst3`.
- [x] pluginval strictness level 5 passes.
- [ ] Repeat CI after any subsequent source-code change.

## 2. Host integration
- [ ] Plugin scans and opens in FL Studio 26.
- [ ] Plugin scans and opens in Cubase 13.
- [ ] Editor opens, closes and reopens without a crash.
- [ ] Mono and stereo input/output layouts work.
- [ ] Verify 44.1, 48, 88.2, 96 and 192 kHz.
- [ ] Verify 64, 128, 256, 512 and 1024 sample buffers.

## 3. Capture and project state
- [ ] Mono input records dual-mono; stereo input preserves L/R.
- [ ] Capture starts/stops on audio blocks without interrupting host playback.
- [ ] 60-second limit stops safely; repeated takes contain no stale samples.
- [ ] Completed captures are archived as 24-bit WAV in Documents/CRAY LDR/STUTTER LAB/Audio/Captures.
- [ ] Captured take is present after DAW save, close and reopen.
- [ ] Project restore works whether setStateInformation occurs before or after prepareToPlay.
- [ ] Malformed, truncated and oversized state chunks fail safely.

## 4. Selection and render
- [ ] Selection start/end clamp to the captured take.
- [ ] Full selection is available to the scan renderer; no silent truncation.
- [ ] Straight, dotted and triplet divisions match host BPM.
- [ ] Forward, reverse, ping-pong and deterministic random directions are correct.
- [ ] Swing, gate, humanize and seed produce expected, repeatable patterns.
- [ ] Parameter and selection edits trigger a debounced background re-render.
- [ ] A cancelled or stale render never replaces the current take's result.
- [ ] Render WAV is 24-bit PCM at the expected sample rate and opens in both hosts.
- [ ] Render archive creates unique filenames and never overwrites an existing file.

## 5. Effects
- [ ] Pitch shift changes fundamental frequency while preserving render duration.
- [ ] Formant movement changes the spectral envelope independently of pitch.
- [ ] Phone filter and drive operate only on the render.
- [ ] Delay division follows host tempo; ping-pong feedback remains bounded.
- [ ] Reverb remains finite and stable at high size/mix values.
- [ ] Combined effect peaks do not hard-clip the 24-bit render.

## 6. Preview and DAW integration
- [ ] Preview crossfades in/out without clicks and does not alter capture.
- [ ] Preview loop on/off behaves correctly.
- [ ] Live re-render hot-swaps while previewing without stopping or jumping unexpectedly.
- [ ] Drag-out imports the exact rendered WAV in FL Studio 26.
- [ ] Drag-out imports the exact rendered WAV in Cubase 13.

## 7. Parameters and UI
- [ ] Host automation read/write works for representative APVTS parameters.
- [ ] Pattern step accents/rests respond correctly to mouse editing.
- [ ] UI resize and DPI at 100%, 125%, 150% and 200% remain usable.
- [ ] LED meters respond correctly and clip indication resets as expected.

## 8. Release gate
- [ ] All host integration checks above completed and recorded.
- [ ] CPU and memory stress tests completed.
- [ ] JUCE and third-party licensing obligations reviewed.
- [ ] Release package includes VST3, version number, installation notes and changelog.

Automated CI and pluginval are necessary build checks, not substitutes for testing in the target DAWs.

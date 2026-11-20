# STUTTER LAB — validation status

This document separates automated checks already completed from integration checks that still require the target DAWs.

## Automated checks completed

- [x] JUCE 8.0.6 / C++17 / CMake project definition.
- [x] Windows x64 Release VST3 compiled with MSVC 2022 in GitHub Actions.
- [x] JUCE-free core test target compiled and CTest passed in the same CI run.
- [x] VST3 bundle discovered by pluginval.
- [x] pluginval strictness level 5 completed successfully, including cold/warm plugin opening and editor validation.
- [x] VST3 artifact uploaded: `StutterLabLdr-Windows-x64-VST3`.
- [x] Capture engine, bounded 60-second buffer, mono/stereo handling and sample-rate conversion are implemented.
- [x] Background render worker, cooperative cancellation and progress reporting are implemented.
- [x] Unique 24-bit WAV render and preview hand-off are implemented.
- [x] Capture and render WAV archive folders are implemented.
- [x] Compressed captured-audio project-state persistence is implemented.
- [x] APVTS parameters and host automation support are implemented.
- [x] Editor includes source/result waveforms, segmented LED meters and drag-to-DAW interaction.

## Evidence

- Workflow run: [Windows VST3 and core tests #44](https://github.com/elcrayldr/Stutter-lab-new/actions/runs/44192406911)
- Branch: `fix/restore-build-structure`
- Tested commit: `0960c292c6be483377d018113bd96893e1d375df`
- Artifact: [StutterLabLdr-Windows-x64-VST3](https://github.com/elcrayldr/Stutter-lab-new/actions/runs/44192406911)

## Still required before public release

- [ ] Confirm plugin scan and editor opening in FL Studio 26.
- [ ] Confirm plugin scan and editor opening in Cubase 13.
- [ ] Record, stop and archive a vocal take in both hosts.
- [ ] Confirm automatic render after capture, selection edits and parameter changes.
- [ ] Audition preview for clicks, dropouts, timing drift and correct loop behaviour.
- [ ] Confirm drag-out imports the exact rendered WAV in FL Studio 26 and Cubase 13.
- [ ] Save, close and reopen a project containing a captured take; confirm state recall.
- [ ] Verify automation playback/write/read for representative APVTS parameters.
- [ ] Verify editor close/reopen, mono/stereo buses, 44.1/48/88.2/96/192 kHz and 64–1024 sample buffers.
- [ ] Stress-test large captures and render cancellation during pitch/formant, delay and reverb processing.
- [ ] Listen critically to pitch/formant processing and confirm whether its artifacts are acceptable for release.
- [ ] Confirm JUCE licence tier and all third-party licence obligations before distribution.

A successful CI build and pluginval run establish a valid VST3 build and a useful level of plugin lifecycle validation. They do not certify compatibility or audio behaviour in FL Studio or Cubase. Do not mark the product release-certified until the DAW checks above are completed.

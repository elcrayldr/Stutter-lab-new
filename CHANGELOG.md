# Changelog

## 0.5.0 (work in progress, not yet built with MSVC)
- Engine: 16-step pattern with rests and per-step accent (+ intensity), progressive level ramp with curve,
  equal-power fade in/out, resonant LP/HP filter, per-effect on/off, reverb decay separated from size.
- Preview: PLAY ORIGINAL / PLAY PROCESSED; processed preview is the exact buffer that Export writes.
- State v7 stores the full parameter tree (pattern, movement, filter survive project recall).
- 8 new factory presets (Tight Stutter, Trap Rolls, Vocal Glitch, Reverse Pulse, Progressive Riser, Rhythmic Gate, Fast Repeats, Broken Rhythm).
- UI: new hardware-style panel (VU meters, illuminated buttons, LED read-out, editable pattern display).
- Tests: +29 core checks (pattern, movement, filter, switches, stereo, empty/tiny/silent input, 22.05-192 kHz).
- Removed from the UI (still in the processor): store A/B, A/B toggle. Per-step pitch was NOT implemented.


## 0.4.0 — expanded production workflow

- Added actual render progress and UI cancellation.
- Added phase-vocoder pitch shifting with independent spectral-envelope/formant movement.
- Added tempo-synchronised ping-pong delay and stereo comb reverb.
- Registered host-automatable parameters with APVTS.
- Added factory presets, user preset save/load, A/B snapshots and UndoManager integration.
- Added compressed capture persistence to the DAW state chunk.
- Refined the UI with a vintage hardware-inspired colour system, meter and control rack.
- Expanded core tests for pitch/formant, ambience and progress.

## 0.3.0 — Milestone A

- Moved render work to a background thread.
- Added full-selection processing, capture/preview engines, deterministic randomisation, unique WAV output and core stress tests.

# Implementation Plan

## Phase 1: CLAP and Bitwig event inspection

- [x] Pin official CLAP headers and the selected C++ glue revision.
- [x] Build the smallest loadable event-only CLAP plug-in.
- [x] Advertise a note input/output and the `note-effect` feature.
- [x] Implement the stable CLAP latency extension and report `0` samples.
- [x] Add bounded debug capture for incoming core event headers and relevant
  payload fields.
- [x] Verify the exact Bitwig event type and units for per-note pitch expression.
- [x] Record findings in `docs/bitwig_clap_findings.md`.

## Phase 2: Pure processing components

- [x] Define CLAP note-identity matching according to the official event rules.
- [x] Implement the fixed 15-channel member allocator.
- [x] Implement and test semitone-to-14-bit pitch conversion.
- [x] Define deterministic behavior for channel exhaustion and malformed events.
- [x] Test overlapping notes, note IDs, endpoints, held expression, and reuse.

## Phase 3: Realtime event translation

- [x] Translate Note On and Note Off to assigned MIDI member channels.
- [x] Translate the confirmed per-note expression event into MIDI Pitch Bend.
- [x] Guarantee initial bend-before-Note-On ordering at the same timestamp.
- [x] Preserve held-note expression timestamps.
- [x] Preserve idle-channel bend through Note Off to protect synth release
  tails, then prepare the channel before its next Note On.
- [x] Handle output queue insertion failure deterministically.

## Phase 4: Host verification

- Validate the binary with the CLAP validator where applicable.
- Confirm recognition and routing in Bitwig.
- Compare tuning and latency against the loopMIDI reference chain.
- Verify polyphony, channel reuse, Note Off behavior, and release-tail pitch with
  OsTIrus configured for +/-24 semitones.

## Phase 5: Isolated information GUI

- [x] Keep translator source and realtime state independent of GUI code.
- [x] Implement a fixed-size embedded Win32 information window with no
  controls or processing behavior.
- [x] Expose the stable CLAP GUI extension only when a platform backend is
  available.
- [x] Provide a non-Windows headless backend for portable processing builds.
- [x] Test Win32 creation, scale, size, embedding, painting, show/hide, and
  destruction without a DAW.
- [ ] Verify the information window inside Bitwig.

## Deferred work

- Configurable pitch-bend range.
- Native X11 or Wayland information GUI backend.
- Presets and state beyond what the first useful parameter requires.
- Additional expression mappings or MIDI transformations.

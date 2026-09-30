# Agent Notes

This is the `PitchExprToPitchBend01` project root.

The project is a small, event-only CLAP Note Effect for Bitwig Studio. It
converts CLAP-native per-note pitch expression into MIDI 1.0 channel pitch
bend using MPE member-channel allocation. It does not process or generate
audio.

## Priorities

1. Preserve sample-accurate CLAP event timestamps.
2. Emit the initial pitch bend before the corresponding Note On when both
   events have the same timestamp.
3. Associate expression with active notes by CLAP note identity.
4. Keep realtime processing deterministic and allocation-free.
5. Keep the implementation small and auditable.

Do not add a GUI, presets, unrelated MIDI transformations, or framework
abstractions until the first milestone works in Bitwig with OsTIrus.

The first milestone is now verified. Revision 0.3.0 has an optional read-only
information GUI. Keep all GUI code behind `InformationGui`, keep platform
window types out of the translator, and never connect GUI lifetime or drawing
to realtime processing state. Unsupported platforms must remain usable as
headless Note Effects.

## Language and build

- Use C++17 for hand-written implementation code.
- Use the stable official CLAP headers and stable extensions unless a draft
  extension is proven necessary.
- Build with CMake on Windows.
- Pin third-party dependency revisions; do not track an unpinned branch.
- Fetch third-party code through CMake into the generated build tree. Do not
  vendor downloaded dependencies into the project repository.
- Keep warning levels high and make project code warning-free.

## Project shape

- `include/pitch_expr_to_pitch_bend/`: internal/public project headers.
- `src/`: plug-in entry, CLAP adapter, and processing implementation.
- `tests/`: deterministic unit tests independent of Bitwig where practical.
- `docs/`: architecture, conventions, investigation notes, and plans.
- `build/`, `build-*/`, `bin/`, `obj/`, and `dist/`: generated output and
  downloaded dependency content only.

## Design conventions

- Prefer explicit data layout and procedural transformations.
- Use a fixed-capacity active-note/channel table in realtime processing.
- Make ownership, lifetime, capacity, and invalid states visible.
- Separate CLAP ABI wiring, event translation, channel allocation, and debug
  logging when they have different reasons to change.
- Do not allocate, lock, perform file I/O, or call unsafe OS services from the
  audio/realtime processing path.
- Use fixed-width integer types where event representation requires them.
- Check all host-provided indexes, sizes, event types, and output insertion
  results.

## C++ formatting

Preserve the visual character of the neighboring projects while adapting it
to modern C++:

- Use generous vertical spacing and explicit control flow.
- Put braces on the same line as functions and control statements.
- Always use braces for `if`, `else`, `for`, `while`, and `do` bodies.
- Do not use ternary expressions or comma expressions as control-flow
  shortcuts.
- Keep spaces inside calls and expressions, for example
  `allocateChannel ( noteIdentity )`.
- Prefer named intermediate values over dense expressions.
- Keep declarations close enough to their use to make lifetime clear.
- Use RAII for non-realtime ownership, but avoid object hierarchies and hidden
  work in the realtime path.
- Do not run an automatic formatter over hand-written source.
- Do not rewrite unrelated formatting during focused changes.
- Comments should explain contracts, invariants, timing, or non-obvious CLAP
  behavior rather than repeat code.

## CLAP rules

- Export only the required `clap_entry` symbol from the plug-in binary.
- Advertise the CLAP `note-effect` feature and appropriate note dialects.
- Do not advertise audio ports for this event-only plug-in unless host testing
  proves that Bitwig requires a different arrangement.
- Preserve incoming event times on generated events.
- Check output event capacity/failure and define deterministic behavior.
- Treat the exact Bitwig pitch-expression event type as an investigation item;
  do not assume it is MIDI pitch bend.
- Keep debug logging bounded and compiled out or realtime-safe in release
  processing.

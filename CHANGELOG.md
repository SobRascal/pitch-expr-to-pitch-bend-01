# Changelog

## Revision 0.3.0 - 2026-08-08

- Added a small fixed-size CLAP information GUI for Windows using native Win32
  and GDI only; no GUI framework or unrelated dependency was introduced.
- Isolated all GUI implementation behind `InformationGui`; the translator
  source and realtime state remain untouched.
- Added a non-Windows headless backend so Linux builds retain complete event
  processing without advertising an unsupported GUI.
- Added author, version, zero-latency, bend-range, channel-layout, and purpose
  information to the window.
- Added a native GUI lifecycle test covering API selection, physical scaling,
  host-window embedding, painting, show/hide, and deterministic destruction.
- Centralized plug-in identity and version metadata and advanced the revision
  to 0.3.0.

## Revision 0.2.2 - 2026-08-08

- Removed the immediate center Pitch Bend after Note Off so OsTIrus release
  tails retain their tuned pitch.
- Kept channel preparation sample-accurate: the required bend is still emitted
  before the next Note On when a released channel is reused.
- Suppressed redundant held-note Pitch Bend output when the quantized 14-bit
  bend value has not changed.
- Limited initial-tuning lookahead to expression events following the relevant
  Note On at the same sample timestamp.
- Made Note On output conditional on successful initial bend output so a note
  cannot start with an unknown bend state.
- Kept a channel reserved when Note Off output fails and added an allocation-
  free retry at sample zero of the next processing block.
- Applied wildcard CLAP Note Choke events to every matching active note.
- Changed generated-output diagnostics to log only events accepted by the
  host's output queue, and bounded repeated translation warnings.
- Corrected the tested OsTIrus receiver range to Pitch Bender Down `-24` and
  Pitch Bender Up `+24`.

## Revision 0.2.1 - 2026-08-08

- Restored CLAP as the preferred output note dialect for Bitwig-to-OsTIrus
  compatibility.
- Changed translated Note On/Off output to CLAP-native note events carrying
  the allocated MPE member channel.
- Kept pitch bend as raw MIDI on the same allocated member channel.
- Added bounded generated-output logging for event-order verification.

## Revision 0.2.0 - 2026-08-08

- Recorded the confirmed Bitwig CLAP tuning-expression contract and ordering.
- Added a fixed-capacity allocator for MPE member channels 2-16.
- Added fixed +/-24-semitone to 14-bit MIDI Pitch Bend conversion.
- Added bend-before-Note-On ordering for equal sample timestamps.
- Added held-note bend updates, channel-consistent Note Off, safe bend reset,
  and member-channel reuse.
- Added minimal plug-in state support for Bitwig undo history.
- Added detailed Note, Note Expression, and raw MIDI debug decoding.
- Added deterministic translation tests; all tests pass under native MSVC.

## Revision 0.1.0 - 2026-08-08

- Created the initial C++17 CLAP Note Effect project scaffold.
- Documented realtime, event-ordering, MPE allocation, and formatting rules.
- Added the first-milestone implementation plan.
- Reserved source, include, test, documentation, and dependency locations.
- Documented the user-visible zero-sample latency guarantee and CLAP latency
  extension contract.
- Pinned the official CLAP and clap-helpers dependencies.
- Added the first loadable event-inspection Note Effect implementation.
- Added bounded debug logging and unchanged note-event passthrough.
- Produced a native Windows MSVC debug build with a static runtime.
- Verified that the native artifact exports only `clap_entry` and imports only
  `KERNEL32.dll`.
- Added native build and Bitwig probe-test instructions.

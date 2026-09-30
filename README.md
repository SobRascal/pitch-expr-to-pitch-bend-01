# PitchExprToPitchBend01

Small CLAP Note Effect for translating CLAP-native per-note pitch expression
into legacy MPE-style MIDI 1.0 channel pitch bend inside Bitwig Studio.

Target signal path:

```text
VTBPM2Pitch -> PitchExprToPitchBend01 -> OsTIrus
```

The plug-in is event-only. It does not generate or process audio.

**Reported plug-in latency: 0 samples.** Events are translated at their
original sample timestamps without lookahead, block delay, or external MIDI/OS
scheduling.

Current revision: `0.3.0` (`2026-08-08`).

## First milestone

- Load in Bitwig as a CLAP Note Effect when supported by the host.
- Accept CLAP note events and Bitwig per-note pitch expression.
- Allocate active notes to MIDI channels 2-16; reserve channel 1 as master.
- Emit 14-bit MIDI Pitch Bend with a fixed range of +/-24 semitones.
- Emit initial bend before Note On at the same sample timestamp.
- Keep later expression and Note Off events on the assigned member channel.
- Preserve a released channel's bend so an audible release tail is not
  retuned; prepare the channel before its next Note On.
- Preserve sample-accurate timing and handle overlapping notes.
- Report zero samples through the CLAP latency extension.
- Provide useful bounded diagnostics in debug builds.
- Provide an optional, read-only information window without connecting it to
  realtime translation state.

## Planned structure

```text
include/pitch_expr_to_pitch_bend/
  Project headers and processing contracts.

src/
  CLAP entry/factory wiring, event translation, and isolated platform GUI
  backends.

tests/
  Translation tests plus a Windows GUI embedding/lifecycle test.

docs/
  Architecture, conventions, CLAP/Bitwig findings, and implementation plan.

```

Generated files belong in `build/`, `bin/`, `obj/`, or `dist/` and are not
tracked.

CMake fetches the official CLAP and clap-helpers repositories into the
generated build tree at exact commit identities. Downloaded dependency source
is never stored in this project repository.

## Implementation stack

- C++17
- CMake
- Official CLAP headers
- Official `clap-helpers` for C++ ABI glue

The dependency choice remains deliberately small. Bitwig host logging has
confirmed the CLAP event type and units used by the translation logic.

## Status

The first working translation milestone is implemented. It readdresses CLAP
Note On/Off events to allocated member channels and converts per-note tuning to
raw MIDI 1.0 Pitch Bend on those channels. It reports zero latency and retains
bounded debug logging for Bitwig verification.

Revision 0.3.0 adds a fixed-size information window showing the author,
version, zero-sample latency, MPE channel layout, bend range, and purpose. The
window has no access to active notes or translator state and is never involved
in audio-thread processing.

The sender uses a fixed +/-24-semitone bend range. For correct tuning, set the
OsTIrus Virus Pitch Bender Down control to `-24` and its Pitch Bender Up
control to `+24`. A larger receiver range exaggerates every tuning offset.

The native Windows debug artifact is written to:

```text
build-msvc/bin/PitchExprToPitchBend01.clap
```

It uses the static MSVC runtime, depends only on Windows system libraries, and
exports only the required `clap_entry` symbol.

## Platform behavior

CLAP and the processing core are cross-platform. The current Windows build
provides an embedded native information GUI through the stable CLAP GUI
extension. A non-Windows build uses the isolated headless backend: it does not
advertise a GUI, but note translation, MPE allocation, timing, and zero latency
remain available and unchanged. An X11 or Wayland backend can be added later
without modifying the translator.

See `docs/build_and_bitwig_test.md` for the current build and host-test steps.

## License

License to be confirmed before the first public release. See `NOTICE`.

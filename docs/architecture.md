# Architecture

## Processing boundary

The plug-in has one CLAP note-event input and one note-event output. It has no
audio ports. All generated events retain the relevant incoming sample time.

The descriptor is expected to advertise `note-effect`. Exact note dialects
will be finalized after Bitwig integration testing; CLAP input is required and
MIDI 1.0 output is the intended translation boundary.

## Latency contract

The plug-in reports `0` samples through the stable CLAP latency extension.
Every translated event retains the appropriate incoming sample timestamp, and
the processor introduces no lookahead or block buffering.

This reported latency is distinct from ordinary CPU execution time. Processing
must complete within Bitwig's realtime deadline, but that execution time does
not shift events and is not reported as plug-in latency.

If a future design introduces intentional buffering, the reported latency and
host notification behavior must be updated. No such buffering is permitted in
the first milestone.

## Realtime data

The core state will have fixed capacity:

- 15 member-channel records representing MIDI channels 2-16.
- Active-note identity associated with each occupied channel.
- Last emitted 14-bit pitch-bend value per channel.
- Pending/observed pitch expression needed for correct note-start ordering.

The representation will be selected after inspecting CLAP's matching rules
for `note_id`, port, channel, and key. No heap allocation is permitted during
processing.

## Responsibilities

```text
CLAP entry and factory
  Creates the plug-in and exposes only the CLAP ABI.

CLAP adapter
  Declares note ports and maps host events to processing operations.

Note/channel allocator
  Owns active-note identity and MIDI member-channel assignment.

Pitch conversion
  Converts semitone offsets to clamped 14-bit MIDI Pitch Bend.

Event emitter
  Preserves timestamps and enforces bend-before-note-on ordering.

Debug diagnostics
  Records bounded observations without blocking the realtime thread.

Information GUI facade
  Delegates optional host-window lifecycle calls to one platform backend and
  has no translator reference.

Platform GUI backend
  Owns only main-thread window and drawing resources. Windows uses a native
  embedded child; unsupported platforms use a headless stub.
```

## GUI isolation contract

The information GUI is optional and read-only. It is created only when the
host opens it, runs exclusively through CLAP main-thread GUI callbacks, and
does not read or mutate active-note, pitch-bend, allocator, or processing
state. It owns no audio resources and cannot change reported latency.

Windows builds support the embedded `win32` CLAP window API with a fixed
physical-pixel size and no floating or resizable mode. Non-Windows builds do
not advertise the GUI extension, so Linux hosts can still load and use the
complete Note Effect without a platform window backend.

## Pitch conversion

For bend range `R` and pitch offset `p` in semitones:

```text
normalized = clamp(p / R, -1, +1)
bend       = round(8192 + normalized * 8192)
bend       = clamp(bend, 0, 16383)
```

The positive endpoint is 16383, never 16384.

## Event-order invariant

For a newly allocated note, the member channel must have the correct bend
state before its Note On is emitted. If the bend and Note On share a sample
timestamp, the bend event is inserted first in output-event order.

Note Off uses the same assigned channel. The channel becomes available after
Note Off, but its bend is deliberately left unchanged so an audible synth
release tail is not pulled back to center. On channel reuse, the next required
bend is emitted before the new Note On. This is the first point at which a
state change is necessary and deterministic without audio-tail knowledge.

If the host rejects a generated Note Off, its member channel remains reserved.
The translator retries that Note Off at sample zero of the next processing
block and releases the channel only after the event is accepted. A wildcard
CLAP Note Choke releases every matching active note rather than only the oldest
match.

## Confirmed host facts

Bitwig loads the plug-in as an event-only CLAP Note Effect. It supplies
per-note tuning as `CLAP_EVENT_NOTE_EXPRESSION`, orders the initial expression
after Note On at the same sample time, and accepts the working CLAP-note plus
raw-MIDI-pitch-bend output used by OsTIrus. Detailed observations are recorded
in `bitwig_clap_findings.md`.

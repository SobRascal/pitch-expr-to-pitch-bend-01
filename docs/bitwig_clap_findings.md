# Bitwig CLAP Findings

This document will contain observations from debug builds and host tests.

## Questions to answer

- Which CLAP event type carries Bitwig per-note pitch/tuning expression?
- What are its value units and range?
- Which identity fields are populated?
- What ordering is used relative to Note On at the same sample time?
- Which input/output note dialects does Bitwig negotiate?
- Does Bitwig classify the plug-in as a Note Effect with no audio ports?

## Confirmed in Bitwig - 2026-08-08

- Bitwig loads the plug-in as a CLAP Note Effect with no audio ports.
- Note On arrives as `CLAP_EVENT_NOTE_ON`.
- Note Off arrives as `CLAP_EVENT_NOTE_OFF`.
- Per-note microtuning arrives as `CLAP_EVENT_NOTE_EXPRESSION` with
  `expression_id = CLAP_NOTE_EXPRESSION_TUNING`.
- The tuning value is a relative semitone offset. One observed value was
  `+0.0433387756348`, approximately `+4.334` cents.
- The observed identity was `note_id=-1, port=0, channel=0, key=38`.
- Bitwig emitted Note On before tuning expression at the same sample time.
  Therefore the translator must look ahead and emit Pitch Bend before its
  generated Note On.
- Bitwig may repeat the current tuning expression at time zero of the next
  processing block.
- Raw MIDI (`CLAP_EVENT_MIDI`) was also observed and requires byte-level debug
  decoding during the translation test.
- The plug-in host shut down normally with exit code zero.
- Clean Note On/Off tests confirmed that Bitwig sends a matching Note Off and
  that no later tuning expression is translated for the released note.
- Resetting Pitch Bend to center immediately after Note Off can retune an
  audible OsTIrus release tail. Revision 0.2.2 therefore preserves the idle
  channel bend and prepares that channel only when it is reused.
- The receiver bend range must match the sender. With this plug-in's fixed
  range, the tested OsTIrus setting is Pitch Bender Down `-24` and Pitch
  Bender Up `+24`; larger settings exaggerate the pitch offset.

## Official CLAP contract

The stable CLAP headers define per-note relative tuning as
`CLAP_EVENT_NOTE_EXPRESSION` with
`expression_id = CLAP_NOTE_EXPRESSION_TUNING`. Its value is a relative offset
in equal-tempered semitones, nominally from -120 to +120. Expression targeting
uses the `(port, channel, key, note_id)` tuple with `-1` wildcard fields.

This is the expected event contract. Debug inspection must still confirm what
Bitwig sends in the working reference chain.

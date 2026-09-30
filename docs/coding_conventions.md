# C++ Coding Conventions

PitchExprToPitchBend01 uses modern C++ where it improves safety and ownership,
while retaining the explicit and spacious style of the neighboring projects.

## Language baseline

- Hand-written implementation code targets C++17.
- Compiler-specific behavior must be isolated and documented.
- Project code must compile cleanly with a high warning level.
- The CLAP boundary follows the official C ABI exactly.

## Explicit control flow

Always use braces, including for a single statement:

```cpp
if ( channelAvailable ) {


          assignChannel ( noteIdentity, channel );


          }
else {


          reportChannelExhaustion ( noteIdentity );


          }
```

Do not replace visible branches with ternary expressions, comma expressions,
or dense one-line statements.

## Naming

- Types: `PascalCase`.
- Functions and local variables: `camelCase`.
- Compile-time constants: `kPascalCase`.
- Files and directories: lowercase `snake_case` except repository metadata.
- CLAP ABI names retain their official spelling.

Names should describe domain meaning: `noteIdentity`, `memberChannel`,
`pitchOffsetSemitones`, and `sampleTime` are preferable to abbreviations.

## Data and ownership

- Prefer fixed-size arrays, indexes, and explicit counts in realtime state.
- Use strong, focused records for note identity and channel state.
- State capacities and sentinel/invalid values near their definitions.
- Avoid owning raw pointers. Use RAII outside the realtime path.
- Do not use exceptions for ordinary realtime control flow.
- Do not allocate, lock, log through blocking I/O, or destroy dynamic objects
  while processing events.

## Functions and modules

- Give each function one clear responsibility.
- Separate pure pitch conversion from note allocation and CLAP event emission.
- Prefer functions whose inputs, outputs, and mutation are evident.
- Check host data and output-event insertion explicitly.
- Use named intermediate values for endpoint clamping and event ordering.

## GUI boundary

- Keep platform window APIs inside one platform-specific GUI source file.
- Expose GUI behavior through the small `InformationGui` facade only.
- Do not include platform GUI headers in translator code or data structures.
- GUI callbacks run on the CLAP main thread and must never be called from
  `process()`.
- An unsupported platform must remain a complete headless Note Effect rather
  than failing plug-in creation.
- Informational drawing must not observe or mutate active-note state.

## Formatting

- Braces stay on the same line as declarations and control statements.
- Spaces are used inside calls and expressions:
  `emitPitchBend ( sampleTime, memberChannel, bendValue )`.
- Wide vertical spacing is intentional.
- Align related fields and parameters when it improves scanning.
- Do not apply an automatic formatter to hand-written files.
- Preserve nearby style and avoid unrelated formatting changes.

## Comments

Comments document contracts, invariants, threading, sample-time ordering, and
observed host behavior. They should not narrate obvious statements.

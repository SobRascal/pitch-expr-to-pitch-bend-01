# Build and Bitwig Test

## Native Windows build

Use the Visual Studio 2022 x64 native tools environment. Configure and build a
Debug artifact with CMake and Ninja:

```text
cmake -S . -B build-msvc -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-msvc --parallel
```

Configuration downloads the official CLAP and clap-helpers sources at the
exact commits pinned in `CMakeLists.txt`. They remain inside the ignored build
tree and are not vendored into this repository.

Output:

```text
build-msvc/bin/PitchExprToPitchBend01.clap
```

The target uses the static MSVC debug runtime. Do not use the Cygwin-generated
artifact from `build/bin/` in Bitwig.

Run both deterministic test targets with:

```text
ctest --test-dir build-msvc --output-on-failure
```

The `translator` test covers realtime behavior. The `information_gui` test
uses a hidden native parent window to verify the isolated Win32 GUI lifecycle.

## Linux processing build

Configure with a native Linux C++17 toolchain and build normally with CMake.
The non-Windows backend does not advertise the optional GUI extension and has
no X11 or Wayland dependency. The event processor remains a complete CLAP Note
Effect with the same zero-sample latency and translation behavior.

## Install for the current probe test

Copy the native `.clap` file to one of the official Windows CLAP search paths:

```text
%COMMONPROGRAMFILES%\CLAP
%LOCALAPPDATA%\Programs\Common\CLAP
```

Restart Bitwig or request a plug-in rescan after replacing the binary. Keep a
single installed copy to avoid ambiguity during development.

## Translation test

1. Close Bitwig before replacing the installed `.clap` binary.
2. Insert `PitchExprToPitchBend01` after `VTBPM2Pitch` and before OsTIrus.
3. Set OsTIrus Pitch Bender Down to `-24` and Pitch Bender Up to `+24`.
4. Verify ordinary notes, simultaneous notes, and repeated notes.
5. Exercise microtuning and held-note pitch expression.
6. Compare against the working HW Instrument configuration with PB range +/-24.
7. Use a patch with an audible release and verify that Note Off does not pull
   the release tail back toward the equal-tempered pitch.
8. Prefer known offsets such as +0.5, +1.0, and -0.5 semitone when possible.
9. Capture Bitwig's CLAP host log entries beginning with
   `PitchExprToPitchBend01 input`.
10. Record representative Note On, tuning-expression, raw MIDI, and Note Off lines in
    `docs/bitwig_clap_findings.md`, preserving their order.
11. Open the plug-in GUI and verify the fixed information window, version,
    zero-sample latency, and +/-24 setup text.

The debug build logs at most 256 incoming events per plug-in instance. This
bound prevents an unending log stream. The log extension is specified as
thread-safe by CLAP, but logging remains debug-only and will not be part of the
release realtime path.

## Expected tuning event

The official stable CLAP contract defines:

```text
type          = CLAP_EVENT_NOTE_EXPRESSION
expression_id = CLAP_NOTE_EXPRESSION_TUNING
value         = relative equal-tempered semitones
```

The first probe confirmed this contract. The second test verifies the generated
MPE behavior in OsTIrus and decodes the raw MIDI events also observed upstream.

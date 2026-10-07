# DragonFlyReverb-SuperCollider-Port

SuperCollider server-plugin port of [Dragonfly Reverb](https://github.com/michaelwillis/dragonfly-reverb).

This package provides four native audio-rate SuperCollider UGens:

- `EarlyReflectionsDF`
- `HallDF`
- `PlateDF`
- `RoomDF`

This is not a Chow DSP port and intentionally does not use `Ch` class names.

## Status

This is a source release candidate. All four UGens are DSP-backed and build as one scsynth plugin named `DragonflyReverbDF`.

Current verification:

- `tests/run_smoke.sh` renders all four UGens offline.
- `tests/run_ab_vst.sh` can render optional A/B files against installed Dragonfly VST3 plugins using SuperCollider VSTPlugin.
- `tests/compare_ab_audio.py` compares paired AIFF files using Python standard-library code only.

Known A/B caveat: the Dragonfly VST3 binaries installed on the development machine reported `Version 2.34`, while this port vendors Dragonfly source `3.2.11`. Local A/B checks therefore show near-null parity for `EarlyReflectionsDF` and `PlateDF`, but not for `HallDF` and `RoomDF`.

## Layout

```text
DragonFlyReverb-SuperCollider-Port/
  CMakeLists.txt
  Classes/
  HelpSource/Classes/
  src/
  tests/
  notes/
  upstream/dragonfly-reverb/
```

The SuperCollider plugin target remains `DragonflyReverbDF` because that is the installed extension/plugin bundle name. The project/repository name is `DragonFlyReverb-SuperCollider-Port`.

The upstream Dragonfly/Freeverb3 source is vendored under `upstream/dragonfly-reverb` at commit:

```text
b3c15af951b998ac4c4e974c93b583c8b3eaeb31
```

## License

Dragonfly Reverb is GPL-3.0-or-later. This port vendors and derives from that source, so it is distributed under GPL-3.0-or-later terms. See `LICENSE` and `NOTICE.md`.

## Build

Requirements:

- SuperCollider source tree, tested locally with SuperCollider `3.14.1`
- CMake
- A C++14 compiler

From this directory:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH="/path/to/supercollider-3.14.1"
cmake --build build-3.14.1
cmake --install build-3.14.1 --prefix build-3.14.1/stage
```

Local workspace example:

```bash
cmake -S . -B build-3.14.1 -DSC_PATH="../chow dsp to sc codex/supercollider-3.14.1"
```

## Smoke Test

```bash
SC_PATH="/path/to/supercollider-3.14.1" tests/run_smoke.sh
```

The smoke test renders:

```text
build-3.14.1/smoke-dragonfly-df.aiff
```

## Optional VST A/B Test

Requirements:

- SuperCollider VSTPlugin extension
- Installed Dragonfly VST3 plugins

```bash
SC_PATH="/path/to/supercollider-3.14.1" \
DRAGONFLY_VST3_DIR="/Library/Audio/Plug-Ins/VST3" \
tests/run_ab_vst.sh

tests/compare_ab_audio.py build-3.14.1/ab
```

The A/B harness writes paired native/VST renders under `build-3.14.1/ab`.

On macOS, loading all Dragonfly VST3 plugins in one scsynth process may print duplicate Objective-C class warnings from the Dragonfly plugin bundles. The harness completes locally, but stricter future testing should isolate each VST in a separate scsynth process.

## Release Notes

See `RELEASE_NOTES.md` for the current pre-release notes and known limitations.

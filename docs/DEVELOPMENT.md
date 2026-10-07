# Development Notes

This document contains testing, layout, and architectural notes that were previously part of the main README.

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

The SuperCollider plugin target is `DragonflyReverbDF` because that is the installed extension/plugin bundle name. This project is not a Chow DSP port and intentionally does not use `Ch` class names.

The upstream Dragonfly/Freeverb3 source is vendored under `upstream/dragonfly-reverb` at commit: `b3c15af951b998ac4c4e974c93b583c8b3eaeb31`.

## Status

This is a source release candidate. All four UGens are DSP-backed and build as one scsynth plugin.

Current verification:
- `tests/run_smoke.sh` renders all four UGens offline.
- `tests/run_ab_vst.sh` can render optional A/B files against installed Dragonfly VST3 plugins using SuperCollider VSTPlugin.
- `tests/compare_ab_audio.py` compares paired AIFF files using Python standard-library code only.

Known A/B caveat: the Dragonfly VST3 binaries installed on the development machine reported `Version 2.34`, while this port vendors Dragonfly source `3.2.11`. Local A/B checks therefore show near-null parity for `EarlyReflectionsDF` and `PlateDF`, but not for `HallDF` and `RoomDF`.

## Smoke Test

```bash
SC_PATH="/path/to/supercollider-3.14.1" tests/run_smoke.sh
```

The smoke test renders: `build-3.14.1/smoke-dragonfly-df.aiff`

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

The A/B harness writes paired native/VST renders under `build-3.14.1/ab`. On macOS, loading all Dragonfly VST3 plugins in one scsynth process may print duplicate Objective-C class warnings from the Dragonfly plugin bundles.

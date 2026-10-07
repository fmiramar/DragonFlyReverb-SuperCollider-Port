# Release Notes

## v0.1.0-pre

Initial GitHub-ready source release preparation for DragonFlyReverb-SuperCollider-Port.

Included UGens:

- `EarlyReflectionsDF`
- `HallDF`
- `PlateDF`
- `RoomDF`

Status:

- Builds as one SuperCollider server plugin target: `DragonflyReverbDF`.
- Includes SuperCollider class wrappers and schelp help files.
- Includes an offline smoke test covering all four UGens.
- Includes optional VSTPlugin A/B render tooling and a Python standard-library audio comparison script.
- Local A/B checks show `EarlyReflectionsDF` and `PlateDF` are numerically very close to the installed Dragonfly VST3 plugins.
- Local A/B checks show `HallDF` and `RoomDF` differ from the installed Dragonfly VST3 plugins; the installed VST3 binaries report Dragonfly `2.34`, while this port vendors Dragonfly `3.2.11` source.

Known limitations:

- Parameter changes that resize or reload Freeverb3 internals should be treated as setup/block-boundary controls until a stricter preallocation strategy is added.
- Broader sample-rate coverage beyond the current 48 kHz smoke/A-B path still needs release testing.
- The optional VST A/B harness expects SuperCollider VSTPlugin and installed Dragonfly VST3 plugins.

# DragonFlyReverb-SuperCollider-Port

SuperCollider server-plugin port of Michael Willis's open-source [Dragonfly Reverb](https://github.com/michaelwillis/dragonfly-reverb), which is based on algorithms from Freeverb3. This port runs the identical C++ DSP code found in the upstream VST3 plugins natively as SuperCollider UGens, bypassing the need for a plugin host.

## UGens

This package provides four native audio-rate SuperCollider UGens:

- **`EarlyReflectionsDF`**: Dragonfly Early Reflections. Uses the Freeverb3 `earlyref` model to simulate the first few discrete reflections of a room, providing spatial cues without a long reverberant tail.
- **`HallDF`**: Dragonfly Hall Reverb. Combines the geometric `earlyref` model with the highly modulated `zrev2` late-reverb model to emulate large, dense concert halls.
- **`PlateDF`**: Dragonfly Plate Reverb. Incorporates three modified Freeverb3 plate algorithms (`nrev`, `nrevb`, `strev`) to simulate the metallic density and bright decay of physical plate reverberators.
- **`RoomDF`**: Dragonfly Room Reverb. Combines `earlyref` with the scaled-down `progenitor2` late-reverb network, optimized for placing sounds in tight, realistic spaces.

## Build

Requirements: SuperCollider SDK, CMake, a C++14 compiler.

```bash
cmake -S . -B build -DSC_PATH="/path/to/supercollider"
cmake --build build --config Release
cmake --install build --config Release --prefix "/path/to/SC/Extensions"
```

*For developer and testing information, see `docs/DEVELOPMENT.md` and `RELEASE_NOTES.md`.*

## License

Dragonfly Reverb is GPL-3.0-or-later. This port vendors and derives from that source, so it is distributed under **GPL-3.0-or-later** terms. See `LICENSE` and `NOTICE.md`.

# Notices

DragonFlyReverb-SuperCollider-Port is a SuperCollider server-plugin port of Dragonfly Reverb.

Upstream Dragonfly Reverb:

- Project: <https://github.com/michaelwillis/dragonfly-reverb>
- Pinned source revision used for this port: `b3c15af951b998ac4c4e974c93b583c8b3eaeb31`
- Upstream version in vendored source: `3.2.11`
- License: GPL-3.0-or-later

This repository vendors the relevant upstream Dragonfly Reverb/Freeverb3 source under `upstream/dragonfly-reverb` so the SuperCollider plugin can be built from source without fetching DSP code at build time.

The installed Dragonfly VST3 binaries used during local A/B checks on the development machine reported `Version 2.34`, which does not match the vendored `3.2.11` source. This is why strict A/B parity against those local VST binaries is not treated as a release blocker for HallDF and RoomDF.

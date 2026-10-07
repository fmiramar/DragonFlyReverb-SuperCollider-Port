# DragonFlyReverb-SuperCollider-Port Work Plan

## Goal

Port Dragonfly Reverb's plate, room, hall, and early-reflections effects as native SuperCollider audio-rate UGens.

## Workspace Boundary

Dragonfly is not a Chow DSP port. Its package lives at:

```text
SuperCollider UGen Ports/DragonFlyReverb-SuperCollider-Port
```

The Chow DSP ports remain separate under:

```text
SuperCollider UGen Ports/chow dsp to sc codex
```

## Phase 1: Source Audit

1. Clone or vendor a pinned Dragonfly Reverb revision. Completed: `upstream/dragonfly-reverb`.
2. Record the upstream commit hash and release version. Completed commit: `b3c15af951b998ac4c4e974c93b583c8b3eaeb31`.
3. Identify the exact files needed from:
   - `plugins/dragonfly-plate-reverb`
   - `plugins/dragonfly-room-reverb`
   - `plugins/dragonfly-hall-reverb`
   - `plugins/dragonfly-early-reflections`
   - `common/freeverb`
   - `common/fv3_config.h`
4. Remove DPF/UI/build-system dependencies from the DSP extraction boundary.
5. Preserve copyright and GPL notices.

## Phase 2: Build Skeleton

1. Add `CMakeLists.txt` following the existing standalone `ports/` server-plugin pattern. Completed.
2. Build one shared SC plugin target named `DragonflyReverbDF`. Completed and verified; all four requested UGens are DSP-backed.
3. Add four sclang wrapper classes. Completed:
   - `PlateDF`
   - `RoomDF`
   - `HallDF`
   - `EarlyReflectionsDF`
4. Add placeholder schelp files with argument ranges matching upstream parameters. Completed.

## Phase 3: First DSP Port

Start with `EarlyReflectionsDF`. Completed as the first DSP-backed UGen.

Reason: it depends mainly on `freeverb/earlyref`, has the smallest algorithm surface, and validates the Freeverb3 extraction before tackling the late-reverb models.

Implementation requirements:

1. Stereo audio input and stereo audio output. Completed.
2. RT-safe allocation in `Ctor`, cleanup in `Dtor`. Completed for object construction/destruction.
3. No locks, logging, or file access in the audio callback. Completed. Remaining caveat: upstream Freeverb3 may reallocate when changing reflection program or room-size factor, so those controls should be treated as setup/block-boundary controls until a preallocation strategy is added.
4. Parameter smoothing or safe block-boundary updates for controls that cause large discontinuities.
5. Correct `sampleRateChanged` equivalent behavior at construction and server reset.

## Phase 4: Hall Port

Port `HallDF` after early reflections. Completed as the second DSP-backed UGen.

Reason: upstream hall combines `earlyref` with `zrev2`, so it reuses the early-reflection groundwork and adds the first full late-reverb core.

Key checks:

1. Predelay clamping, because upstream avoids zero predelay. Completed.
2. Dry, early, late, width, size, diffusion, cutoffs, crossover, decay, spin, wander, early-send, and modulation mappings. Completed.
3. Internal block buffer size handling independent of SuperCollider block size. Completed.

## Phase 5: Plate And Room Ports

Port `PlateDF` and `RoomDF` after hall. Completed.

Plate has multiple algorithm choices in upstream code and likely needs a SuperCollider-facing selector argument. Room should be audited for its exact Freeverb3 model dependencies before implementation.

Key decisions:

1. Keep upstream algorithm selectors where musically meaningful.
2. Use argument ranges that match Dragonfly's normalized UI values where possible.
3. Prefer separate UGen classes over one mode-switching mega-UGen, because SC users get clearer docs and less runtime branching.

## Phase 6: Verification

1. Compile against the local SuperCollider source path used by the other ports.
2. Add SuperCollider smoke tests that instantiate every UGen and render several blocks without NaNs. Completed.
3. Render impulse responses for each UGen. Completed through the A/B render harness.
4. Compare against upstream Dragonfly offline renders at matching sample rates and parameter values. In progress with SuperCollider VSTPlugin and the installed Dragonfly VST3 plugins.
5. Test at 44.1 kHz, 48 kHz, and 96 kHz.
6. Test extreme parameter values for stability, denormals, and runaway feedback.

Current A/B tracking:

1. Native render ordering bug was fixed; native renders are no longer silent.
2. `tests/compare_ab_audio.py` compares AIFF renders and reports peak/RMS dBFS, correlation, raw null, gain-matched null, and normalization/headroom.
3. Latest saved comparison: `build-3.14.1/ab/reports/compare_20260516_184350_after_vst_param_readback.md`.
4. VST parameter readback matched the normalized values sent by the harness for Hall and Room. Saved in `build-3.14.1/ab/reports/vst_param_readback_20260516_184226.md`.
5. Installed Dragonfly Hall/Room VST3 binaries report `Version 2.34`, while the vendored source used by this port reports `3.2.11`. Saved in `build-3.14.1/ab/reports/vst_source_version_check_20260516_184650.md`.
6. EarlyReflectionsDF and PlateDF are numerically very similar to the installed VSTs in the current A/B test. HallDF and RoomDF remain substantially different, with current evidence pointing to source-version DSP differences rather than parameter-write errors.

## Main Risks

1. Freeverb3 extraction may pull in more files than expected.
2. Some Dragonfly parameter behavior lives in plugin glue rather than the Freeverb3 classes.
3. Exact upstream parity requires matching parameter defaults, presets, clamping, and sample-rate scaling.
4. GPL obligations must remain visible in the standalone port.
5. The first working version may be CPU-heavy until unused Freeverb3 code is trimmed.

## Recommended Milestone Order

1. `EarlyReflectionsDF` compiles and passes smoke tests. Completed.
2. `HallDF` compiles and produces stable stereo output. Completed.
3. `PlateDF` compiles with upstream algorithm selector. Completed.
4. `RoomDF` compiles after source audit confirms its model dependencies. Completed.
5. Documentation and A/B render tests are added for all four UGens. A/B render harness and numerical comparison are completed; Hall/Room mismatch is under investigation and currently likely explained by installed VST/source version mismatch.

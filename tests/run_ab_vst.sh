#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "${script_dir}/.." && pwd)"

sc_path="${SC_PATH:-${root}/../chow dsp to sc codex/supercollider-3.14.1}"
sc_classlib="${sc_path}/SCClassLibrary"
vstplugin_ext="${VSTPLUGIN_EXT:-${HOME}/Library/Application Support/SuperCollider/Extensions/VSTPlugin}"

mkdir -p "${root}/tests/vstplugin-scsynth-only"
ln -sf "${vstplugin_ext}/plugins/VSTPlugin.scx" \
    "${root}/tests/vstplugin-scsynth-only/VSTPlugin.scx"

cmake --build "${root}/build-3.14.1"
cmake --install "${root}/build-3.14.1" --prefix "${root}/build-3.14.1/stage"

DRAGONFLY_DF_ROOT="${root}" \
DRAGONFLY_VST3_DIR="${DRAGONFLY_VST3_DIR:-/Library/Audio/Plug-Ins/VST3}" \
sclang -u 0 -a \
    --include-path "${sc_classlib}" \
    --include-path "${root}/build-3.14.1/stage/DragonflyReverbDF" \
    --include-path "${vstplugin_ext}" \
    "${root}/tests/render_ab_vst.scd"

for name in early hall plate room; do
    test -s "${root}/build-3.14.1/ab/${name}_native.aiff"
    test -s "${root}/build-3.14.1/ab/${name}_vst.aiff"
done

if command -v ffprobe >/dev/null 2>&1; then
    for file in "${root}"/build-3.14.1/ab/*.aiff; do
        stats="$(ffprobe -v error -f lavfi -i "amovie=${file},astats=metadata=1:reset=0" \
            -show_entries frame_tags=lavfi.astats.Overall.RMS_level \
            -of default=nw=1 2>/dev/null | tail -1)"
        case "${stats}" in
            *-inf*|"")
                echo "Silent or unreadable A/B render: ${file}" >&2
                exit 1
                ;;
        esac
    done
fi

echo "DRAGONFLY_DF_AB_VST_OK ${root}/build-3.14.1/ab"

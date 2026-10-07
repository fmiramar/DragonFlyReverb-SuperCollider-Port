#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "${script_dir}/.." && pwd)"

sc_path="${SC_PATH:-${root}/../chow dsp to sc codex/supercollider-3.14.1}"
sc_classlib="${sc_path}/SCClassLibrary"
app_plugins="/Applications/SuperCollider.app/Contents/Resources/plugins"
extension="${root}/build-3.14.1/stage/DragonflyReverbDF"
osc_path="${root}/build-3.14.1/smoke-dragonfly-df.osc"
out_path="${root}/build-3.14.1/smoke-dragonfly-df.aiff"

cmake --build "${root}/build-3.14.1"
cmake --install "${root}/build-3.14.1" --prefix "${root}/build-3.14.1/stage"

rm -f "${osc_path}"

DRAGONFLY_DF_ROOT="${root}" \
SC_PLUGIN_PATH="${extension}:${app_plugins}" \
    sclang -u 0 -a \
    --include-path "${sc_classlib}" \
    --include-path "${extension}" \
    "${root}/tests/sc_smoke_dragonfly_df.scd" &

sclang_pid=$!
for _ in {1..100}; do
    if [[ -s "${osc_path}" ]]; then
        break
    fi
    sleep 0.1
done

if [[ ! -s "${osc_path}" ]]; then
    kill "${sclang_pid}" 2>/dev/null || true
    wait "${sclang_pid}" 2>/dev/null || true
    echo "Failed to generate ${osc_path}" >&2
    exit 1
fi

kill "${sclang_pid}" 2>/dev/null || true
wait "${sclang_pid}" 2>/dev/null || true

scsynth -N "${osc_path}" _ "${out_path}" 48000 AIFF int16 \
    -i 0 -o 2 -D 0 \
    -U "${extension}:${app_plugins}" \
    -V 0

test -s "${out_path}"
echo "DRAGONFLY_DF_SMOKE_OK ${out_path}"

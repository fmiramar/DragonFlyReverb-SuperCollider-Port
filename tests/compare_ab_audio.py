#!/usr/bin/env python3
"""Compare Dragonfly native/VST A/B AIFF render pairs.

The script intentionally uses only Python's standard library so it can run in
the local SuperCollider porting environment without installing dependencies.
It reports normalization/headroom, RMS/peak levels, correlation, gain-matched
correlation, and null-test difference for each native/VST pair.
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path


PAIR_NAMES = ("early", "hall", "plate", "room")


def db(value: float) -> float:
    if value <= 0.0:
        return float("-inf")
    return 20.0 * math.log10(value)


def dbfs_from_int(value: float, full_scale: float) -> float:
    return db(value / full_scale)


def parse_extended_float_80(data: bytes) -> float:
    if len(data) != 10:
        raise ValueError("AIFF sample-rate field must be 10 bytes")
    exponent = int.from_bytes(data[0:2], "big")
    mantissa = int.from_bytes(data[2:10], "big")
    sign = -1 if exponent & 0x8000 else 1
    exponent &= 0x7FFF
    if exponent == 0 and mantissa == 0:
        return 0.0
    return sign * mantissa * (2.0 ** (exponent - 16383 - 63))


def read_aiff(path: Path) -> dict:
    data = path.read_bytes()
    if data[0:4] != b"FORM" or data[8:12] not in (b"AIFF", b"AIFC"):
        raise ValueError(f"{path}: not an AIFF/AIFC file")

    channels = None
    sample_width = None
    frames = None
    sample_rate = None
    raw = None

    offset = 12
    while offset + 8 <= len(data):
        chunk_id = data[offset : offset + 4]
        chunk_size = int.from_bytes(data[offset + 4 : offset + 8], "big")
        chunk_start = offset + 8
        chunk_end = chunk_start + chunk_size
        chunk = data[chunk_start:chunk_end]

        if chunk_id == b"COMM":
            channels = int.from_bytes(chunk[0:2], "big")
            frames = int.from_bytes(chunk[2:6], "big")
            bits = int.from_bytes(chunk[6:8], "big")
            if bits % 8 != 0:
                raise ValueError(f"{path}: unsupported sample width {bits} bits")
            sample_width = bits // 8
            sample_rate = int(round(parse_extended_float_80(chunk[8:18])))
        elif chunk_id == b"SSND":
            sound_offset = int.from_bytes(chunk[0:4], "big")
            raw = chunk[8 + sound_offset :]

        offset = chunk_end + (chunk_size & 1)

    if None in (channels, sample_width, frames, sample_rate) or raw is None:
        raise ValueError(f"{path}: missing COMM or SSND chunk")

    if sample_width not in (2, 3, 4):
        raise ValueError(f"{path}: unsupported sample width {sample_width}")

    sample_count = frames * channels
    raw = raw[: sample_count * sample_width]
    peak = 0
    sum_sq = 0.0
    for sample in sample_iter(raw, sample_width):
        abs_sample = abs(sample)
        peak = max(peak, abs_sample)
        sum_sq += float(sample) * float(sample)
    rms = math.sqrt(sum_sq / sample_count) if sample_count else 0.0
    full_scale = float((1 << (sample_width * 8 - 1)) - 1)

    return {
        "path": path,
        "channels": channels,
        "sample_width": sample_width,
        "frames": frames,
        "sample_rate": sample_rate,
        "raw": raw,
        "peak": peak,
        "rms": rms,
        "peak_dbfs": dbfs_from_int(peak, full_scale),
        "rms_dbfs": dbfs_from_int(rms, full_scale),
        "full_scale": full_scale,
    }


def sample_iter(raw: bytes, width: int):
    # AIFF is big-endian signed PCM.
    for i in range(0, len(raw), width):
        yield int.from_bytes(raw[i : i + width], "big", signed=True)


def vector_stats(a: dict, b: dict) -> dict:
    if (a["sample_rate"], a["channels"], a["sample_width"]) != (
        b["sample_rate"],
        b["channels"],
        b["sample_width"],
    ):
        raise ValueError("Files have different sample format, channel count, or sample rate")

    width = a["sample_width"]
    full_scale = a["full_scale"]
    compare_bytes = min(len(a["raw"]), len(b["raw"]))
    compare_bytes -= compare_bytes % width
    raw_a = a["raw"][:compare_bytes]
    raw_b = b["raw"][:compare_bytes]
    count = 0
    sum_x2 = 0.0
    sum_y2 = 0.0
    sum_xy = 0.0
    sum_diff2 = 0.0
    max_diff = 0.0

    for x, y in zip(sample_iter(raw_a, width), sample_iter(raw_b, width)):
        xf = float(x)
        yf = float(y)
        diff = xf - yf
        count += 1
        sum_x2 += xf * xf
        sum_y2 += yf * yf
        sum_xy += xf * yf
        sum_diff2 += diff * diff
        max_diff = max(max_diff, abs(diff))

    if count == 0:
        raise ValueError("Empty files")

    rms_x = math.sqrt(sum_x2 / count)
    rms_y = math.sqrt(sum_y2 / count)
    rms_diff = math.sqrt(sum_diff2 / count)
    corr = sum_xy / math.sqrt(sum_x2 * sum_y2) if sum_x2 > 0 and sum_y2 > 0 else 0.0
    gain = sum_xy / sum_y2 if sum_y2 > 0 else 0.0  # y * gain best fits x

    matched_diff2 = 0.0
    for x, y in zip(sample_iter(raw_a, width), sample_iter(raw_b, width)):
        diff = float(x) - gain * float(y)
        matched_diff2 += diff * diff
    matched_rms_diff = math.sqrt(matched_diff2 / count)

    return {
        "samples": count,
        "compared_frames": count // a["channels"],
        "frame_delta": a["frames"] - b["frames"],
        "corr": corr,
        "gain_vst_to_native": gain,
        "gain_vst_to_native_db": db(abs(gain)),
        "diff_rms_dbfs": dbfs_from_int(rms_diff, full_scale),
        "diff_peak_dbfs": dbfs_from_int(max_diff, full_scale),
        "diff_vs_native_rms_db": db(rms_diff / rms_x) if rms_x > 0 else float("inf"),
        "matched_diff_rms_dbfs": dbfs_from_int(matched_rms_diff, full_scale),
        "matched_diff_vs_native_rms_db": db(matched_rms_diff / rms_x) if rms_x > 0 else float("inf"),
    }


def classify(corr: float, diff_vs_native_rms_db: float, rms_delta_db: float) -> str:
    abs_delta = abs(rms_delta_db)
    if corr > 0.999 and diff_vs_native_rms_db < -45 and abs_delta < 0.25:
        return "very similar"
    if corr > 0.98 and diff_vs_native_rms_db < -25 and abs_delta < 1.5:
        return "similar"
    if corr > 0.85 and diff_vs_native_rms_db < -12:
        return "related, audible differences likely"
    return "substantially different"


def compare_pair(directory: Path, name: str) -> dict:
    native = read_aiff(directory / f"{name}_native.aiff")
    vst = read_aiff(directory / f"{name}_vst.aiff")
    stats = vector_stats(native, vst)
    rms_delta = native["rms_dbfs"] - vst["rms_dbfs"]
    peak_delta = native["peak_dbfs"] - vst["peak_dbfs"]
    return {
        "name": name,
        "native": native,
        "vst": vst,
        "stats": stats,
        "rms_delta_db": rms_delta,
        "peak_delta_db": peak_delta,
        "classification": classify(stats["corr"], stats["diff_vs_native_rms_db"], rms_delta),
    }


def format_db(value: float) -> str:
    if math.isinf(value):
        return "-inf dB" if value < 0 else "inf dB"
    return f"{value:.2f} dB"


def print_report(results: list[dict]) -> None:
    print("# Dragonfly Native vs VST A/B Audio Comparison")
    print()
    print("Interpretation:")
    print("- Peak/RMS dBFS indicate normalization and loudness. 0 dBFS peak would mean full-scale normalization or clipping risk.")
    print("- RMS delta is native minus VST. Positive means native is louder.")
    print("- Correlation near 1.0 means similar waveform/phase. Reverbs can sound similar while not nulling perfectly.")
    print("- Null RMS is the RMS of native - VST. More negative is more similar.")
    print("- Gain-matched null removes simple level mismatch before subtracting.")
    print()

    for result in results:
        native = result["native"]
        vst = result["vst"]
        stats = result["stats"]
        print(f"## {result['name']}")
        print(f"Classification: {result['classification']}")
        print(
            "Native: "
            f"peak {format_db(native['peak_dbfs'])}FS, "
            f"RMS {format_db(native['rms_dbfs'])}FS"
        )
        print(
            "VST:    "
            f"peak {format_db(vst['peak_dbfs'])}FS, "
            f"RMS {format_db(vst['rms_dbfs'])}FS"
        )
        print(
            "Level deltas: "
            f"peak {format_db(result['peak_delta_db'])}, "
            f"RMS {format_db(result['rms_delta_db'])}"
        )
        print(
            "Similarity: "
            f"corr {stats['corr']:.6f}, compared frames {stats['compared_frames']}, "
            f"frame delta native-vst {stats['frame_delta']}, "
            f"null RMS {format_db(stats['diff_rms_dbfs'])}FS "
            f"({format_db(stats['diff_vs_native_rms_db'])} vs native RMS)"
        )
        print(
            "Gain matched: "
            f"VST gain to native {stats['gain_vst_to_native']:.6f} "
            f"({format_db(stats['gain_vst_to_native_db'])}), "
            f"null RMS {format_db(stats['matched_diff_rms_dbfs'])}FS "
            f"({format_db(stats['matched_diff_vs_native_rms_db'])} vs native RMS)"
        )
        print()

    native_peaks = [r["native"]["peak_dbfs"] for r in results]
    vst_peaks = [r["vst"]["peak_dbfs"] for r in results]
    print("## Normalization Summary")
    print(f"Native peak range: {format_db(min(native_peaks))}FS to {format_db(max(native_peaks))}FS")
    print(f"VST peak range:    {format_db(min(vst_peaks))}FS to {format_db(max(vst_peaks))}FS")
    print("None of the current renders are normalized to 0 dBFS; all have headroom.")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "directory",
        nargs="?",
        default="build-3.14.1/ab",
        help="Directory containing *_native.aiff and *_vst.aiff files",
    )
    args = parser.parse_args()

    directory = Path(args.directory)
    results = [compare_pair(directory, name) for name in PAIR_NAMES]
    print_report(results)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
import sys
import subprocess
import re
import math
import statistics

ELF_DEFAULT = "firmware_cpp/build/firmware_cpp.elf"
ROWS = 11
COLS = 19


def main():
    sample_count = int(sys.argv[1]) if len(sys.argv) > 1 else 100
    elf_path = sys.argv[2] if len(sys.argv) > 2 else ELF_DEFAULT

    print(f"Starting probe-rs run for {elf_path}...")
    print(f"Collecting {sample_count} frames...")
    proc = subprocess.Popen(
        ["probe-rs", "run", "--chip", "STM32F429IGTx", elf_path],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )

    history = [[[] for _ in range(COLS)] for _ in range(ROWS)]
    row_pattern = re.compile(r"R(\d+):((?:\s+\d+)+)\s*$")
    frames_collected = 0
    in_frame = False

    while frames_collected < sample_count:
        line = proc.stdout.readline()
        if not line:
            break
        stripped = line.strip()
        if stripped.endswith("===GRID==="):
            in_frame = True
            continue
        if stripped.endswith("===END==="):
            in_frame = False
            frames_collected += 1
            if frames_collected % 10 == 0:
                print(f"  {frames_collected}/{sample_count} frames...")
            continue
        if not in_frame:
            continue
        match = row_pattern.search(stripped)
        if not match:
            continue
        row = int(match.group(1))
        vals = [int(v) for v in match.group(2).strip().split()]
        if 0 <= row < ROWS and len(vals) == COLS:
            for c in range(COLS):
                history[row][c].append(vals[c])

    proc.terminate()
    proc.wait()

    if frames_collected == 0:
        print("No frames collected.")
        return

    print(f"\nCollected {frames_collected} frames. Computing statistics...\n")

    stats = []
    for r in range(ROWS):
        for c in range(COLS):
            vals = history[r][c]
            mean = statistics.mean(vals)
            stdev = statistics.stdev(vals) if len(vals) > 1 else 0.0
            rng = max(vals) - min(vals)
            stats.append({
                "row": r,
                "col": c,
                "mean": mean,
                "min": min(vals),
                "max": max(vals),
                "range": rng,
                "stdev": stdev,
            })

    avg_stdev = statistics.mean(s["stdev"] for s in stats)
    avg_range = statistics.mean(s["range"] for s in stats)
    max_stdev = max(stats, key=lambda s: s["stdev"])
    min_stdev = min(stats, key=lambda s: s["stdev"])
    max_range = max(stats, key=lambda s: s["range"])
    min_range = min(stats, key=lambda s: s["range"])

    print(f"{'='*60}")
    print(f"OVERALL SUMMARY ({frames_collected} frames)")
    print(f"{'='*60}")
    print(f"  Average stddev (noise): {avg_stdev:.2f} LSB")
    print(f"  Average range (min-max): {avg_range:.2f} LSB")
    print(f"")
    print(f"MOST STABLE sensor:")
    print(f"  R{min_stdev['row']:02d}:C{min_stdev['col']:02d}  stddev={min_stdev['stdev']:.2f}  range={min_stdev['range']}")
    print(f"")
    print(f"LEAST STABLE sensor:")
    print(f"  R{max_stdev['row']:02d}:C{max_stdev['col']:02d}  stddev={max_stdev['stdev']:.2f}  range={max_stdev['range']}")
    print(f"")
    print(f"WORST RANGE:")
    print(f"  R{max_range['row']:02d}:C{max_range['col']:02d}  range={max_range['range']}  stddev={max_range['stdev']:.2f}")
    print(f"")

    sorted_by_stdev = sorted(stats, key=lambda s: s["stdev"], reverse=True)
    print(f"{'='*60}")
    print(f"TOP 10 NOISIEST SENSORS (by stddev)")
    print(f"{'='*60}")
    print(f"{'Pos':>8} {'Mean':>8} {'Min':>6} {'Max':>6} {'Range':>7} {'Stddev':>8}")
    print(f"{'-'*50}")
    for s in sorted_by_stdev[:10]:
        pos = f"R{s['row']:02d}:C{s['col']:02d}"
        print(f"{pos:>8} {s['mean']:>8.1f} {s['min']:>6} {s['max']:>6} {s['range']:>7} {s['stdev']:>8.2f}")

    sorted_by_range = sorted(stats, key=lambda s: s["range"], reverse=True)
    print(f"\n{'='*60}")
    print(f"TOP 10 WORST RANGE SENSORS")
    print(f"{'='*60}")
    print(f"{'Pos':>8} {'Mean':>8} {'Min':>6} {'Max':>6} {'Range':>7} {'Stddev':>8}")
    print(f"{'-'*50}")
    for s in sorted_by_range[:10]:
        pos = f"R{s['row']:02d}:C{s['col']:02d}"
        print(f"{pos:>8} {s['mean']:>8.1f} {s['min']:>6} {s['max']:>6} {s['range']:>7} {s['stdev']:>8.2f}")

    above_2stdev = sum(1 for s in stats if s["stdev"] > 2 * avg_stdev)
    print(f"\n{'='*60}")
    print(f"DISTRIBUTION")
    print(f"{'='*60}")
    print(f"  Sensors with stddev > 2x average: {above_2stdev}/{len(stats)}")
    print(f"  Stddev percentiles:")
    sorted_stdevs = sorted(s["stdev"] for s in stats)
    for p in [50, 90, 95, 99]:
        idx = int(len(sorted_stdevs) * p / 100)
        idx = min(idx, len(sorted_stdevs) - 1)
        print(f"    P{p:02d}: {sorted_stdevs[idx]:.2f}")


if __name__ == "__main__":
    main()

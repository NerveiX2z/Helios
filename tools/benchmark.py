#!/usr/bin/env python3
"""Summarize saved Helios --test output. Python 3, standard library only."""
import argparse
import re
from collections import defaultdict
from pathlib import Path

parser = argparse.ArgumentParser(description="Summarize Helios benchmark logs")
parser.add_argument("log", type=Path, help="File saved by ./helios --test 2>&1 | tee test.log")
args = parser.parse_args()
text = args.log.read_text(encoding="utf-8", errors="replace")
pattern = re.compile(r"\[STATS\] FPS: ([\d.]+) \| Frame ms: ([\d.]+) \| Extra cubes: (\d+)")
results = defaultdict(list)
for fps, ms, cubes in pattern.findall(text):
    results[int(cubes)].append((float(fps), float(ms)))
if not results:
    raise SystemExit("No [STATS] lines found in log")
print(f"{'Cubes':>8} {'Samples':>8} {'Avg FPS':>10} {'Min FPS':>10} {'Avg ms':>10}")
for cubes, samples in sorted(results.items()):
    print(f"{cubes:8d} {len(samples):8d} {sum(v[0] for v in samples)/len(samples):10.2f} "
          f"{min(v[0] for v in samples):10.2f} {sum(v[1] for v in samples)/len(samples):10.2f}")

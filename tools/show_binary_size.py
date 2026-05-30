#!/usr/bin/env python3
import argparse
import os
import subprocess
import sys

MEMORY_REGIONS = {
    "FLASH": {"length_kb": 1024},
    "RAM":   {"length_kb": 192},
    "CCMRAM": {"length_kb": 64},
}

SECTION_REGION = {
    ".isr_vector": "FLASH",
    ".text":       "FLASH",
    ".rodata":     "FLASH",
    ".data":       "RAM",
    ".bss":        "RAM",
    ".ccmram":     "CCMRAM",
}

BAR_WIDTH = 30


def parse_size_output(output):
    sections = {}
    in_table = False
    for line in output.splitlines():
        line = line.strip()
        if line.startswith("section"):
            in_table = True
            continue
        if in_table and line:
            parts = line.split()
            if len(parts) >= 3:
                try:
                    sections[parts[0]] = int(parts[1])
                except ValueError:
                    pass
    return sections


def parse_berkeley_output(output):
    sections = {}
    for line in output.splitlines():
        line = line.strip()
        if line.startswith(".") or line.startswith("_"):
            parts = line.split()
            if len(parts) >= 2:
                try:
                    sections[parts[0].lstrip(".")] = int(parts[1])
                except ValueError:
                    pass
    return sections


def get_size_output(elf_path):
    for fmt in [["--format=sysv"], []]:
        try:
            result = subprocess.run(
                ["arm-none-eabi-size"] + fmt + [elf_path],
                capture_output=True, text=True, timeout=30,
            )
            if result.returncode == 0:
                return result.stdout
        except FileNotFoundError:
            print("ERROR: arm-none-eabi-size not found on PATH.", file=sys.stderr)
            sys.exit(1)
    print(f"ERROR: arm-none-eabi-size failed on {elf_path}", file=sys.stderr)
    sys.exit(1)


def get_binary_size(elf_path):
    bin_path = elf_path.replace(".elf", ".bin")
    if os.path.exists(bin_path):
        return os.path.getsize(bin_path)
    try:
        result = subprocess.run(
            ["arm-none-eabi-objcopy", "-O", "binary", elf_path, "-"],
            capture_output=True, timeout=30,
        )
        if result.returncode == 0:
            return len(result.stdout)
    except Exception:
        pass
    return 0


def format_bytes(size):
    if size >= 1024:
        return f"{size / 1024:.1f} KB"
    return f"{size} B"


def make_bar(used, total, width=BAR_WIDTH):
    if total <= 0:
        return "[" + "?" * width + "]"
    filled = int(min(used / total, 1.0) * width)
    return "[" + "█" * filled + "░" * (width - filled) + "]"


def print_report(sections, binary_size, elf_path):
    region_usage = {r: 0 for r in MEMORY_REGIONS}
    region_sections = {r: [] for r in MEMORY_REGIONS}

    for sec_name, sec_size in sections.items():
        region = SECTION_REGION.get(sec_name)
        if region:
            region_usage[region] += sec_size
            region_sections[region].append((sec_name, sec_size))

    print()
    print("─── Memory Usage ─────────────────────────────────────")

    for region, info in MEMORY_REGIONS.items():
        used = region_usage[region]
        total_bytes = info["length_kb"] * 1024
        percent = (used / total_bytes * 100) if total_bytes else 0
        print(f"  {region:7s} ({info['length_kb']:>4} KB): "
              f"{make_bar(used, total_bytes)} {percent:5.1f}%  ({format_bytes(used)})")
        for name, sz in sorted(region_sections[region], key=lambda x: -x[1]):
            print(f"    {name:<16s} {sz:>8,} B")

    print("──────────────────────────────────────────────────────")
    if binary_size:
        print(f"  Binary (.bin):     {format_bytes(binary_size):>12s}  ({os.path.basename(elf_path)})")

    flash_total = MEMORY_REGIONS["FLASH"]["length_kb"] * 1024
    ram_total   = MEMORY_REGIONS["RAM"]["length_kb"] * 1024
    print(f"  Total FLASH used:  {format_bytes(region_usage['FLASH']):>12s} / {format_bytes(flash_total)}")
    print(f"  Total RAM used:    {format_bytes(region_usage['RAM']):>12s} / {format_bytes(ram_total)}")
    print("──────────────────────────────────────────────────────")
    print()


def main():
    parser = argparse.ArgumentParser(description="Wonkle firmware memory usage reporter")
    parser.add_argument("elf_file", help="Path to the .elf firmware binary")
    args = parser.parse_args()

    elf_path = os.path.abspath(args.elf_file)
    if not os.path.exists(elf_path):
        print(f"ERROR: File not found: {elf_path}", file=sys.stderr)
        sys.exit(1)

    output = get_size_output(elf_path)
    sections = parse_size_output(output) or parse_berkeley_output(output)
    if not sections:
        print("ERROR: Could not parse size output.", file=sys.stderr)
        sys.exit(1)

    print_report(sections, get_binary_size(elf_path), elf_path)


if __name__ == "__main__":
    main()

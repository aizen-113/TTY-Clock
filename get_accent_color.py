#!/usr/bin/env python3
"""
Detect the system accent color and return an ANSI color code.
Output is just the number, e.g. 34 (blue), 32 (green), 31 (red).
Default is 37 (white) if nothing is found.
"""

import os
import subprocess
import sys

# Map GNOME accent names to ANSI color codes
GNOME_MAP = {
    "blue":   "34",
    "teal":   "36",
    "green":  "32",
    "yellow": "33",
    "orange": "33",
    "red":    "31",
    "pink":   "35",
    "purple": "35",
    "slate":  "37",
}

ANSI_COLORS = [
    ("30", (0, 0, 0)),       # black
    ("31", (255, 0, 0)),     # red
    ("32", (0, 255, 0)),     # green
    ("33", (255, 255, 0)),   # yellow
    ("34", (0, 0, 255)),     # blue
    ("35", (255, 0, 255)),   # magenta/pink
    ("36", (0, 255, 255)),   # cyan/teal
    ("37", (255, 255, 255)), # white
]


def rgb_to_ansi(r, g, b):
    """Map an RGB color to the closest standard ANSI color."""
    best = "37"
    best_dist = float("inf")
    for code, (cr, cg, cb) in ANSI_COLORS:
        dist = (r - cr) ** 2 + (g - cg) ** 2 + (b - cb) ** 2
        if dist < best_dist:
            best_dist = dist
            best = code
    return best


def get_gnome_accent():
    try:
        out = subprocess.run(
            ["gsettings", "get", "org.gnome.desktop.interface", "accent-color"],
            capture_output=True, text=True, check=True, timeout=2
        )
        name = out.stdout.strip().strip("'\"")
        return GNOME_MAP.get(name, None)
    except Exception:
        return None


def get_kde_accent():
    for cmd in ("kreadconfig6", "kreadconfig5"):
        try:
            out = subprocess.run(
                [cmd, "--file", "kdeglobals", "--group", "General", "--key", "AccentColor"],
                capture_output=True, text=True, check=False, timeout=2
            )
            val = out.stdout.strip()
            if not val:
                continue
            parts = val.replace(",", " ").split()
            if len(parts) >= 3:
                r, g, b = int(parts[0]), int(parts[1]), int(parts[2])
                return rgb_to_ansi(r, g, b)
        except Exception:
            continue
    return None


def get_pywal_accent():
    """Read pywal colors (common for Hyprland/rice setups)."""
    try:
        wal_colors = os.path.expanduser("~/.cache/wal/colors")
        with open(wal_colors) as f:
            colors = [line.strip() for line in f if line.strip()]
        # color[1] is usually the accent / secondary color in pywal
        if len(colors) > 1:
            hex_color = colors[1].lstrip("#")
            r = int(hex_color[0:2], 16)
            g = int(hex_color[2:4], 16)
            b = int(hex_color[4:6], 16)
            return rgb_to_ansi(r, g, b)
    except Exception:
        return None


def get_portal_accent():
    """Try reading the Freedesktop accent color portal."""
    try:
        out = subprocess.run(
            [
                "dbus-send", "--session", "--dest=org.freedesktop.portal.Desktop",
                "--type=method_call", "--print-reply", "/org/freedesktop/portal/desktop",
                "org.freedesktop.portal.Settings.Read",
                "string:org.freedesktop.appearance", "string:accent-color",
            ],
            capture_output=True, text=True, check=True, timeout=2
        )
        # Output looks like: variant array [ double 0.12, double 0.56, double 0.89 ]
        nums = []
        for line in out.stdout.splitlines():
            for word in line.replace(",", " ").split():
                try:
                    nums.append(float(word))
                except ValueError:
                    pass
        if len(nums) >= 3:
            r, g, b = int(nums[0] * 255), int(nums[1] * 255), int(nums[2] * 255)
            return rgb_to_ansi(r, g, b)
    except Exception:
        return None


def main():
    color = (
        get_pywal_accent()
        or get_portal_accent()
        or get_kde_accent()
        or get_gnome_accent()
        or "37"
    )
    print(color, end="")


if __name__ == "__main__":
    main()

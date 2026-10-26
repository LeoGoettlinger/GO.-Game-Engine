#!/usr/bin/env python3
"""Reproducibly generate GO. Game Engine's splash screens and app icon.

GO.-owned generator code is MIT licensed; see docs/go/THIRD_PARTY_NOTICES.md.
Requires Pillow and the bundled OFL-1.1 Monocraft font. No original Mojang or
Microsoft font/artwork is used.

Usage:
    python3 go_assets/make_splash.py
    python3 go_assets/make_splash.py --check
"""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import math
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
FONT = ROOT / "go_assets" / "fonts" / "Monocraft.ttf"
SPLASH_TEXT = "A Godot Game Engine Fork by SampleCraft"
GAME_BG = (36, 36, 36)  # main_builders.py: Color(0.14, 0.14, 0.14)
EDITOR_BG = (32, 37, 49)  # main_builders.py: Color(0.125, 0.145, 0.192)
INK = (235, 241, 249)
MUTED = (154, 169, 190)
ACCENT = (86, 204, 242)
ACCENT_DARK = (31, 86, 111)
PANEL = (28, 33, 45)


def font(size: int) -> ImageFont.FreeTypeFont:
    if not FONT.is_file():
        raise SystemExit(f"Missing Monocraft font: {FONT}")
    return ImageFont.truetype(str(FONT), size)


def centered_text(
    draw: ImageDraw.ImageDraw,
    center: tuple[float, float],
    text: str,
    face: ImageFont.FreeTypeFont,
    fill: tuple[int, int, int],
) -> None:
    bbox = draw.textbbox((0, 0), text, font=face)
    x = center[0] - (bbox[0] + bbox[2]) / 2
    y = center[1] - (bbox[1] + bbox[3]) / 2
    draw.text((x, y), text, font=face, fill=fill)


def draw_go_mark(
    draw: ImageDraw.ImageDraw,
    center: tuple[float, float],
    radius: float,
    accent: tuple[int, int, int],
) -> None:
    """A simple original circular arrow / 'go' mark, not derived from Godot's robot."""
    cx, cy = center
    r = radius
    box = (cx - r, cy - r, cx + r, cy + r)
    width = max(3, int(r * 0.13))
    draw.arc(box, start=48, end=318, fill=accent, width=width)
    # Arrow head at the end of the arc (318 degrees).
    angle = math.radians(318)
    tip_x = cx + math.cos(angle) * r
    tip_y = cy + math.sin(angle) * r
    tangent = angle + math.pi / 2
    nx, ny = math.cos(tangent), math.sin(tangent)
    bx, by = math.cos(angle), math.sin(angle)
    size = r * 0.34
    triangle = [
        (tip_x + bx * size * 0.45, tip_y + by * size * 0.45),
        (tip_x - bx * size * 0.7 + nx * size * 0.6, tip_y - by * size * 0.7 + ny * size * 0.6),
        (tip_x - bx * size * 0.7 - nx * size * 0.6, tip_y - by * size * 0.7 - ny * size * 0.6),
    ]
    draw.polygon(triangle, fill=accent)
    # A small forward chevron in the middle.
    s = r * 0.38
    draw.line(
        [(cx - s * 0.45, cy - s * 0.5), (cx + s * 0.25, cy), (cx - s * 0.45, cy + s * 0.5)],
        fill=accent,
        width=max(3, int(s * 0.28)),
        joint="curve",
    )


def render_splash(size: tuple[int, int], background: tuple[int, int, int]) -> Image.Image:
    width, height = size
    image = Image.new("RGB", size, background)
    draw = ImageDraw.Draw(image)

    # Very restrained geometric background: two soft diagonal accents.
    draw.line((0, int(height * 0.74), width, int(height * 0.31)), fill=ACCENT_DARK, width=2)
    draw.line((0, int(height * 0.82), width, int(height * 0.39)), fill=(23, 28, 39), width=2)

    panel_w = int(width * 0.54)
    panel_h = int(height * 0.31)
    panel_left = (width - panel_w) // 2
    panel_top = int(height * 0.20)
    draw.rounded_rectangle(
        (panel_left, panel_top, panel_left + panel_w, panel_top + panel_h),
        radius=18,
        fill=PANEL,
        outline=ACCENT_DARK,
        width=2,
    )

    mark_r = min(panel_h * 0.29, panel_w * 0.12)
    mark_center = (panel_left + panel_w * 0.24, panel_top + panel_h * 0.50)
    draw_go_mark(draw, mark_center, mark_r, ACCENT)

    logo_face = font(int(min(panel_h * 0.52, panel_w * 0.22)))
    centered_text(
        draw,
        (panel_left + panel_w * 0.56, panel_top + panel_h * 0.47),
        "GO.",
        logo_face,
        INK,
    )

    subtitle_face = font(max(14, int(height * 0.035)))
    centered_text(
        draw,
        (width / 2, panel_top + panel_h + max(24, height * 0.075)),
        "Game Engine",
        subtitle_face,
        MUTED,
    )

    footer_face = font(max(13, int(height * 0.027)))
    margin = max(24, int(width * 0.035))
    bbox = draw.textbbox((0, 0), SPLASH_TEXT, font=footer_face)
    footer_x = width - margin - (bbox[2] - bbox[0])
    footer_y = height - margin - (bbox[3] - bbox[1]) - bbox[1]
    draw.text((footer_x, footer_y), SPLASH_TEXT, font=footer_face, fill=INK)
    return image


def render_app_icon(size: int = 256) -> Image.Image:
    image = Image.new("RGBA", (size, size), (32, 37, 49, 255))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle((8, 8, size - 8, size - 8), radius=42, fill=PANEL, outline=ACCENT_DARK, width=5)
    draw_go_mark(draw, (size * 0.34, size * 0.45), size * 0.19, ACCENT)
    logo_face = font(int(size * 0.30))
    centered_text(draw, (size * 0.70, size * 0.45), "GO", logo_face, INK)
    draw.ellipse((size * 0.76, size * 0.64, size * 0.85, size * 0.73), fill=ACCENT)
    return image


def generate() -> list[Path]:
    outputs: list[Path] = []
    game = render_splash((800, 600), GAME_BG)
    editor = render_splash((1152, 648), EDITOR_BG)
    icon = render_app_icon()

    targets = [
        (ROOT / "main" / "splash.png", game),
        (ROOT / "main" / "splash_editor.png", editor),
        (ROOT / "main" / "app_icon.png", icon),
        (ROOT / "go_project_template" / "splash.png", game),
    ]
    for path, image in targets:
        path.parent.mkdir(parents=True, exist_ok=True)
        image.save(path, optimize=True)
        outputs.append(path)
    return outputs


def check() -> int:
    expected = {
        ROOT / "main" / "splash.png": (800, 600),
        ROOT / "main" / "splash_editor.png": (1152, 648),
        ROOT / "main" / "app_icon.png": (256, 256),
        ROOT / "go_project_template" / "splash.png": (800, 600),
    }
    for path, size in expected.items():
        if not path.is_file():
            print(f"missing: {path}", file=sys.stderr)
            return 1
        with Image.open(path) as image:
            if image.size != size:
                print(f"wrong size: {path}: {image.size} != {size}", file=sys.stderr)
                return 1
    print(f"OK: {len(expected)} generated images; exact splash text: {SPLASH_TEXT!r}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="verify generated image sizes without writing")
    args = parser.parse_args()
    if args.check:
        return check()
    for path in generate():
        print(path.relative_to(ROOT))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

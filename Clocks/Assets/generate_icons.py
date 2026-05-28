"""
Generate the Clocks app icon set in a modern Windows 11 / Fluent style.

Design goals
------------
* **Readable at 16 px**. Earlier iterations packed in 12 ticks, three hands,
  a meridian arc and a sheen — all of that collapses to mud in the taskbar.
  This generator switches detail levels based on output size so a 16 px tile
  carries only the essentials (rounded tile + outer ring + two hands + cap).
* **Win11 squircle aesthetic**. Solid color with a very gentle vertical
  gradient, rounded-square corners tuned per size (smaller icons need
  proportionally larger radii to read as Fluent tiles).
* **Pure white glyph** on the plated tile so contrast holds at every size.
* **Asset coverage**. We emit every scale (100/125/150/200/400) and target
  size (16/24/32/48/256) variant Windows looks up via MRT, plus
  ``altform-unplated`` (white glyph on transparent for dark surfaces) and
  ``altform-lightunplated`` (dark glyph on transparent for light surfaces).

Run from this folder:

    pip install pillow
    python generate_icons.py
"""
from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

ASSETS = Path(__file__).resolve().parent

# ---------------------------------------------------------------------------
# Palette
# ---------------------------------------------------------------------------
# Fluent-style blue tile with a hint of indigo at the bottom. Solid enough to
# look like a system app, distinctive enough to not collide with the built-in
# Windows Clock (which is amber).
TILE_TOP = (46, 116, 230)
TILE_BOTTOM = (74, 66, 198)

GLYPH_LIGHT = (255, 255, 255)   # for the plated tile and dark unplated surfaces
GLYPH_DARK = (32, 38, 56)       # for the lightunplated surface
ACCENT = (255, 196, 60)         # warm amber, only used on larger sizes


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
def _lerp(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(3))


def _vertical_gradient(size, top, bottom):
    w, h = size
    img = Image.new("RGB", size, top)
    px = img.load()
    for y in range(h):
        t = y / max(1, h - 1)
        c = _lerp(top, bottom, t)
        for x in range(w):
            px[x, y] = c
    return img


def _rounded_mask(size, radius):
    m = Image.new("L", size, 0)
    ImageDraw.Draw(m).rounded_rectangle(
        (0, 0, size[0], size[1]), radius=radius, fill=255
    )
    return m


def _corner_radius_frac(side: int) -> float:
    """Win11 tiles look right when the corner radius is proportionally
    bigger on small icons and smaller (but still soft) on large ones."""
    if side <= 24:
        return 0.26
    if side <= 48:
        return 0.22
    if side <= 96:
        return 0.18
    return 0.16


def _glyph_margin_frac(side: int) -> float:
    """How much padding to leave around the clock face inside the tile."""
    if side <= 24:
        return 0.10
    if side <= 48:
        return 0.12
    return 0.14


# ---------------------------------------------------------------------------
# Clock glyph
# ---------------------------------------------------------------------------
def _draw_clock_glyph(canvas: Image.Image, cx: float, cy: float, r: float,
                      color, *, side: int):
    """Draw a minimalist clock face (outer ring, hands, cap).

    Detail is scaled to the output size:
      * very small (<=32): no ticks at all — would just become noise
      * medium (33..96): 4 ticks at the cardinal positions
      * large (>=97): subtle minor tick marks plus an accent on the
        second hand
    """
    draw = ImageDraw.Draw(canvas, "RGBA")

    # Choose stroke weights as a fraction of radius so they scale evenly.
    # At very small sizes we drop the outer ring entirely — it competes with
    # the rounded-square tile silhouette and crowds the hands. The hands +
    # cap alone read as a clock at 16 px.
    draw_ring = side > 24
    ring_w = max(1, int(round(r * (0.085 if side <= 64 else 0.075))))
    if side <= 48:
        hour_w = max(2, int(round(r * 0.22)))
        min_w = max(2, int(round(r * 0.16)))
    else:
        hour_w = max(2, int(round(r * 0.14)))
        min_w = max(2, int(round(r * 0.10)))

    if draw_ring:
        draw.ellipse(
            (cx - r, cy - r, cx + r, cy + r),
            outline=color, width=ring_w,
        )

    # Ticks (skipped on very small sizes so the silhouette stays clean)
    if side > 32:
        major_count = 12 if side >= 96 else 4
        for i in range(major_count):
            step_deg = 360 / major_count
            ang = math.radians(i * step_deg - 90)
            outer = r * 0.86
            inner = r * (0.72 if side >= 96 else 0.74)
            tick_w = max(1, int(round(r * (0.07 if side >= 96 else 0.10))))
            x1 = cx + math.cos(ang) * inner
            y1 = cy + math.sin(ang) * inner
            x2 = cx + math.cos(ang) * outer
            y2 = cy + math.sin(ang) * outer
            draw.line((x1, y1, x2, y2), fill=color, width=tick_w)

    def _hand(angle_deg: float, length_frac: float, width: int, hand_color):
        ang = math.radians(angle_deg - 90)
        x = cx + math.cos(ang) * r * length_frac
        y = cy + math.sin(ang) * r * length_frac
        draw.line((cx, cy, x, y), fill=hand_color, width=width)

    # Classic 10:10 — friendly silhouette, makes the cap visible.
    _hand(300, 0.52, hour_w, color)   # hour hand pointing to "10"
    _hand(60, 0.72, min_w, color)     # minute hand pointing to "2"

    # Accent second hand (only on big tiles where it stays legible)
    if side >= 128:
        sec_w = max(1, int(round(r * 0.035)))
        _hand(120, 0.78, sec_w, ACCENT)

    # Center cap — kept smaller at tiny sizes so it doesn't swallow the
    # hands; larger sizes get a subtle accent dot inside.
    cap = max(2, int(round(r * (0.18 if side <= 24 else 0.10))))
    draw.ellipse((cx - cap, cy - cap, cx + cap, cy + cap), fill=color)
    if side >= 96:
        inner = max(1, int(round(r * 0.035)))
        draw.ellipse(
            (cx - inner, cy - inner, cx + inner, cy + inner), fill=ACCENT
        )


# ---------------------------------------------------------------------------
# Compositions
# ---------------------------------------------------------------------------
def make_plated_tile(size_px: int) -> Image.Image:
    """Squircle tile with the gradient background and the white clock glyph."""
    size = (size_px, size_px)
    bg = _vertical_gradient(size, TILE_TOP, TILE_BOTTOM).convert("RGBA")

    radius = int(round(size_px * _corner_radius_frac(size_px)))
    mask = _rounded_mask(size, radius)

    canvas = Image.new("RGBA", size, (0, 0, 0, 0))
    canvas.paste(bg, (0, 0), mask)

    margin = int(round(size_px * _glyph_margin_frac(size_px)))
    cx = cy = size_px / 2
    r = (size_px - 2 * margin) / 2
    _draw_clock_glyph(canvas, cx, cy, r, GLYPH_LIGHT, side=size_px)

    # A tiny inner highlight on larger tiles to give the squircle a hint of
    # depth without resorting to a heavy sheen.
    if size_px >= 96:
        highlight = Image.new("RGBA", size, (0, 0, 0, 0))
        hd = ImageDraw.Draw(highlight)
        hd.rounded_rectangle(
            (1, 1, size_px - 1, size_px - 1),
            radius=radius - 1,
            outline=(255, 255, 255, 38),
            width=max(1, size_px // 256),
        )
        canvas.alpha_composite(highlight)

    return canvas


def make_unplated_glyph(size_px: int, color) -> Image.Image:
    """Just the clock glyph on transparent — used by Windows surfaces that
    apply their own background plate (taskbar/Start in some themes)."""
    size = (size_px, size_px)
    canvas = Image.new("RGBA", size, (0, 0, 0, 0))
    # Unplated glyphs get a bit more margin so they don't crowd the system plate
    margin = max(1, int(round(size_px * 0.08)))
    cx = cy = size_px / 2
    r = (size_px - 2 * margin) / 2
    _draw_clock_glyph(canvas, cx, cy, r, color, side=size_px)
    return canvas


def make_wide_tile(size: tuple[int, int]) -> Image.Image:
    """Wide tile: gradient background, clock on the left, wordmark on the right."""
    w, h = size
    bg = _vertical_gradient(size, TILE_TOP, TILE_BOTTOM).convert("RGBA")

    radius = int(round(min(w, h) * _corner_radius_frac(min(w, h))))
    mask = _rounded_mask(size, radius)

    canvas = Image.new("RGBA", size, (0, 0, 0, 0))
    canvas.paste(bg, (0, 0), mask)

    margin = int(round(h * 0.16))
    cx = h / 2 + h * 0.04
    cy = h / 2
    r = (h - 2 * margin) / 2
    _draw_clock_glyph(canvas, cx, cy, r, GLYPH_LIGHT, side=h)

    try:
        from PIL import ImageFont
        font = None
        for fname in ("segoeuisl.ttf", "segoeui.ttf", "arial.ttf"):
            try:
                font = ImageFont.truetype(fname, int(h * 0.32))
                break
            except OSError:
                continue
        if font is None:
            font = ImageFont.load_default()
        draw = ImageDraw.Draw(canvas)
        text = "Clocks"
        bbox = draw.textbbox((0, 0), text, font=font)
        th = bbox[3] - bbox[1]
        tx = cx + r + h * 0.20
        ty = (h - th) / 2 - bbox[1]
        draw.text((tx, ty), text, font=font, fill=(245, 247, 252, 245))
    except Exception:
        pass

    return canvas


def make_splash(size: tuple[int, int]) -> Image.Image:
    """Splash screen — gradient bg, large centered clock, wordmark below."""
    w, h = size
    bg = _vertical_gradient(size, TILE_TOP, TILE_BOTTOM).convert("RGBA")
    canvas = bg

    r = min(w, h) * 0.22
    cx = w / 2
    cy = h / 2 - h * 0.04
    _draw_clock_glyph(canvas, cx, cy, r, GLYPH_LIGHT, side=int(min(w, h)))

    try:
        from PIL import ImageFont
        font = None
        for fname in ("segoeuil.ttf", "segoeuisl.ttf", "segoeui.ttf", "arial.ttf"):
            try:
                font = ImageFont.truetype(fname, int(h * 0.10))
                break
            except OSError:
                continue
        if font is None:
            font = ImageFont.load_default()
        draw = ImageDraw.Draw(canvas)
        text = "Clocks"
        bbox = draw.textbbox((0, 0), text, font=font)
        tw = bbox[2] - bbox[0]
        draw.text(((w - tw) / 2, cy + r + h * 0.04), text,
                  font=font, fill=(245, 247, 252, 235))
    except Exception:
        pass

    return canvas


# ---------------------------------------------------------------------------
# Manifest output set
# ---------------------------------------------------------------------------
# Square44x44Logo: the taskbar/Start icon. We emit every variant Windows
# might pull via MRT so no matter what surface or scale is asking, there's
# a hand-tuned asset waiting.
SQUARE44_SCALES = [
    ("scale-100", 44),
    ("scale-125", 55),
    ("scale-150", 66),
    ("scale-200", 88),
    ("scale-400", 176),
]
SQUARE44_TARGETSIZES = [16, 20, 24, 30, 32, 36, 40, 48, 60, 64, 72, 80, 96, 256]

SQUARE150_SCALES = [
    ("scale-100", 150),
    ("scale-125", 188),
    ("scale-150", 225),
    ("scale-200", 300),
    ("scale-400", 600),
]

WIDE310_SCALES = [
    ("scale-100", (310, 150)),
    ("scale-125", (388, 188)),
    ("scale-150", (465, 225)),
    ("scale-200", (620, 300)),
    ("scale-400", (1240, 600)),
]

SPLASH_SCALES = [
    ("scale-100", (620, 300)),
    ("scale-125", (775, 375)),
    ("scale-150", (930, 450)),
    ("scale-200", (1240, 600)),
    ("scale-400", (2480, 1200)),
]


def _save(img: Image.Image, name: str):
    out = ASSETS / name
    img.save(out, "PNG", optimize=True)
    print(f"  wrote {name}  ({img.size[0]}x{img.size[1]})")


def main():
    print(f"Generating icons in {ASSETS}")

    # ---- Square 44x44 (taskbar / Start) -----------------------------------
    for suffix, px in SQUARE44_SCALES:
        _save(make_plated_tile(px), f"Square44x44Logo.{suffix}.png")
    for tsz in SQUARE44_TARGETSIZES:
        _save(make_plated_tile(tsz),
              f"Square44x44Logo.targetsize-{tsz}.png")
        _save(make_unplated_glyph(tsz, GLYPH_LIGHT),
              f"Square44x44Logo.targetsize-{tsz}_altform-unplated.png")
        _save(make_unplated_glyph(tsz, GLYPH_DARK),
              f"Square44x44Logo.targetsize-{tsz}_altform-lightunplated.png")

    # ---- Square 150x150 (medium tile) -------------------------------------
    for suffix, px in SQUARE150_SCALES:
        _save(make_plated_tile(px), f"Square150x150Logo.{suffix}.png")

    # ---- Wide 310x150 (wide tile) -----------------------------------------
    for suffix, size in WIDE310_SCALES:
        _save(make_wide_tile(size), f"Wide310x150Logo.{suffix}.png")

    # ---- Splash screen ----------------------------------------------------
    for suffix, size in SPLASH_SCALES:
        _save(make_splash(size), f"SplashScreen.{suffix}.png")

    # ---- Lock screen ------------------------------------------------------
    _save(make_unplated_glyph(48, GLYPH_LIGHT),
          "LockScreenLogo.scale-200.png")

    # ---- Store logo (used in Partner Center & some shell surfaces) --------
    _save(make_plated_tile(50), "StoreLogo.png")
    _save(make_plated_tile(50), "StoreLogo.scale-100.png")
    _save(make_plated_tile(100), "StoreLogo.scale-200.png")

    print("Done.")


if __name__ == "__main__":
    main()

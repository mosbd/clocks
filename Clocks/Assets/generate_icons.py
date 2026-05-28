"""
Generate the Clocks app icon set.

Concept: a modern world clock — a clean clock face on a deep
indigo→teal gradient rounded-square tile, with a subtle meridian arc
hinting at the app's world-clock / time-zone focus. Hands are set to
the classic 10:10 position for a friendly, balanced silhouette.

Run from the Assets folder:
    python generate_icons.py
"""
from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

ASSETS = Path(__file__).resolve().parent

# Brand palette
BG_TOP = (28, 42, 89)        # deep indigo
BG_BOTTOM = (14, 116, 144)   # teal
FACE = (248, 250, 252)       # near-white
FACE_SHADOW = (180, 195, 215)
TICK = (40, 55, 95)
ACCENT = (255, 178, 36)      # warm amber for the hour hand & meridian
HAND = (24, 33, 70)
RING = (255, 255, 255, 60)


def lerp(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(3))


def vertical_gradient(size, top, bottom):
    w, h = size
    img = Image.new("RGB", size, top)
    px = img.load()
    for y in range(h):
        t = y / max(1, h - 1)
        c = lerp(top, bottom, t)
        for x in range(w):
            px[x, y] = c
    return img


def rounded_mask(size, radius):
    m = Image.new("L", size, 0)
    ImageDraw.Draw(m).rounded_rectangle((0, 0, size[0], size[1]), radius=radius, fill=255)
    return m


def draw_clock_face(canvas: Image.Image, cx, cy, r, *, plated: bool):
    """Draw the clock face (and hands) centered at (cx, cy) with radius r."""
    draw = ImageDraw.Draw(canvas, "RGBA")

    # Soft outer halo / depth ring (only on plated tiles)
    if plated:
        halo = Image.new("RGBA", canvas.size, (0, 0, 0, 0))
        hdraw = ImageDraw.Draw(halo)
        hdraw.ellipse((cx - r - r * 0.08, cy - r - r * 0.08,
                       cx + r + r * 0.08, cy + r + r * 0.08),
                      fill=(0, 0, 0, 90))
        halo = halo.filter(ImageFilter.GaussianBlur(radius=max(2, r * 0.05)))
        canvas.alpha_composite(halo)

    # Face
    face_color = FACE if plated else (255, 255, 255, 255)
    draw.ellipse((cx - r, cy - r, cx + r, cy + r), fill=face_color)

    # Inner bezel
    bezel = max(1, int(r * 0.04))
    draw.ellipse((cx - r, cy - r, cx + r, cy + r),
                 outline=(TICK[0], TICK[1], TICK[2], 90), width=bezel)

    # Subtle meridian arc (a curved line suggesting a globe longitude)
    if r >= 30:
        meridian = Image.new("RGBA", canvas.size, (0, 0, 0, 0))
        mdraw = ImageDraw.Draw(meridian)
        ell = (cx - r * 0.55, cy - r * 0.92, cx + r * 0.55, cy + r * 0.92)
        mdraw.arc(ell, start=255, end=285, fill=(*ACCENT, 110),
                  width=max(1, int(r * 0.05)))
        ell2 = (cx - r * 0.85, cy - r * 0.92, cx + r * 0.85, cy + r * 0.92)
        mdraw.arc(ell2, start=260, end=280, fill=(*ACCENT, 70),
                  width=max(1, int(r * 0.04)))
        canvas.alpha_composite(meridian)

    # Hour ticks
    for i in range(12):
        ang = math.radians(i * 30 - 90)
        is_major = (i % 3 == 0)
        outer = r * 0.92
        inner = r * (0.78 if is_major else 0.84)
        x1 = cx + math.cos(ang) * inner
        y1 = cy + math.sin(ang) * inner
        x2 = cx + math.cos(ang) * outer
        y2 = cy + math.sin(ang) * outer
        w = max(1, int(r * (0.075 if is_major else 0.035)))
        draw.line((x1, y1, x2, y2), fill=TICK, width=w)

    # Hands — 10:10
    # Minute hand → "2" (60° from 12)
    def hand(angle_deg, length_frac, width_frac, color):
        ang = math.radians(angle_deg - 90)
        x = cx + math.cos(ang) * r * length_frac
        y = cy + math.sin(ang) * r * length_frac
        # Tail (short stub opposite)
        tx = cx - math.cos(ang) * r * 0.12
        ty = cy - math.sin(ang) * r * 0.12
        w = max(2, int(r * width_frac))
        draw.line((tx, ty, x, y), fill=color, width=w)

    hand(300, 0.55, 0.085, HAND)   # hour at 10
    hand(60, 0.78, 0.06, HAND)     # minute at 2

    # Accent second hand
    hand(125, 0.82, 0.022, ACCENT)

    # Center cap
    cap = max(2, int(r * 0.07))
    draw.ellipse((cx - cap, cy - cap, cx + cap, cy + cap), fill=HAND)
    inner_cap = max(1, int(r * 0.025))
    draw.ellipse((cx - inner_cap, cy - inner_cap, cx + inner_cap, cy + inner_cap), fill=ACCENT)


def make_plated_tile(size, *, margin_frac=0.08, corner_frac=0.18):
    """Rounded-square gradient tile with the clock face."""
    w, h = size
    bg = vertical_gradient(size, BG_TOP, BG_BOTTOM).convert("RGBA")

    # Subtle diagonal sheen
    sheen = Image.new("RGBA", size, (0, 0, 0, 0))
    sdraw = ImageDraw.Draw(sheen)
    sdraw.polygon([(0, 0), (w, 0), (w, int(h * 0.45)), (0, int(h * 0.85))],
                  fill=(255, 255, 255, 22))
    bg.alpha_composite(sheen)

    radius = int(min(w, h) * corner_frac)
    mask = rounded_mask(size, radius)

    canvas = Image.new("RGBA", size, (0, 0, 0, 0))
    canvas.paste(bg, (0, 0), mask)

    margin = int(min(w, h) * margin_frac)
    cx, cy = w / 2, h / 2
    r = (min(w, h) - 2 * margin) / 2
    draw_clock_face(canvas, cx, cy, r, plated=True)
    return canvas


def make_unplated(size, *, color=(255, 255, 255, 255)):
    """Transparent background, monochrome white clock glyph (for unplated/lockscreen)."""
    w, h = size
    canvas = Image.new("RGBA", size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(canvas, "RGBA")
    cx, cy = w / 2, h / 2
    r = min(w, h) / 2 - max(1, int(min(w, h) * 0.05))

    line_w = max(1, int(r * 0.12))
    draw.ellipse((cx - r, cy - r, cx + r, cy + r), outline=color, width=line_w)

    # Ticks at 12/3/6/9
    for i in range(4):
        ang = math.radians(i * 90 - 90)
        outer = r * 0.95
        inner = r * 0.75
        x1 = cx + math.cos(ang) * inner
        y1 = cy + math.sin(ang) * inner
        x2 = cx + math.cos(ang) * outer
        y2 = cy + math.sin(ang) * outer
        draw.line((x1, y1, x2, y2), fill=color, width=max(1, int(r * 0.10)))

    # Hands 10:10
    def hand(angle_deg, length_frac, width_frac):
        ang = math.radians(angle_deg - 90)
        x = cx + math.cos(ang) * r * length_frac
        y = cy + math.sin(ang) * r * length_frac
        draw.line((cx, cy, x, y), fill=color, width=max(1, int(r * width_frac)))

    hand(300, 0.50, 0.13)
    hand(60, 0.72, 0.10)

    cap = max(1, int(r * 0.10))
    draw.ellipse((cx - cap, cy - cap, cx + cap, cy + cap), fill=color)
    return canvas


def make_wide_tile(size):
    """Wide tile: gradient bg with clock centered-left and 'Clocks' wordmark."""
    w, h = size
    bg = vertical_gradient(size, BG_TOP, BG_BOTTOM).convert("RGBA")
    sheen = Image.new("RGBA", size, (0, 0, 0, 0))
    sdraw = ImageDraw.Draw(sheen)
    sdraw.polygon([(0, 0), (w, 0), (w, int(h * 0.4)), (0, int(h * 0.8))],
                  fill=(255, 255, 255, 22))
    bg.alpha_composite(sheen)

    canvas = bg
    r = h / 2 - h * 0.12
    cx = h / 2 + h * 0.05
    cy = h / 2
    draw_clock_face(canvas, cx, cy, r, plated=True)

    # Wordmark
    try:
        from PIL import ImageFont
        # Try a few common Windows fonts; fall back to default
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
        tw = bbox[2] - bbox[0]
        th = bbox[3] - bbox[1]
        tx = cx + r + h * 0.18
        ty = (h - th) / 2 - bbox[1]
        draw.text((tx, ty), text, font=font, fill=(245, 247, 252, 245))
    except Exception:
        pass

    return canvas


def make_splash(size):
    """Splash screen — gradient bg, large centered clock, wordmark below."""
    w, h = size
    bg = vertical_gradient(size, BG_TOP, BG_BOTTOM).convert("RGBA")
    canvas = bg
    r = min(w, h) * 0.22
    cx = w / 2
    cy = h / 2 - h * 0.04
    draw_clock_face(canvas, cx, cy, r, plated=True)

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


def save(img: Image.Image, name: str):
    out = ASSETS / name
    img.save(out, "PNG", optimize=True)
    print(f"  wrote {name}  ({img.size[0]}x{img.size[1]})")


def main():
    print(f"Generating icons in {ASSETS}")

    # Square 44x44 @ scale-200 → 88x88, plated tile
    save(make_plated_tile((88, 88), margin_frac=0.06, corner_frac=0.14),
         "Square44x44Logo.scale-200.png")

    # Square 44x44 targetsize-24 unplated → 24x24 small colored tile
    # (rendered as-is by Windows with no system plate, so we keep a
    # colored background to stay legible on light surfaces).
    save(make_plated_tile((24, 24), margin_frac=0.06, corner_frac=0.22),
         "Square44x44Logo.targetsize-24_altform-unplated.png")

    # Square 150x150 @ scale-200 → 300x300
    save(make_plated_tile((300, 300)),
         "Square150x150Logo.scale-200.png")

    # Wide 310x150 @ scale-200 → 620x300
    save(make_wide_tile((620, 300)),
         "Wide310x150Logo.scale-200.png")

    # Splash 620x300 @ scale-200 → 1240x600
    save(make_splash((1240, 600)),
         "SplashScreen.scale-200.png")

    # Lockscreen 24x24 @ scale-200 → 48x48 white glyph on transparent
    save(make_unplated((48, 48)),
         "LockScreenLogo.scale-200.png")

    # StoreLogo 50x50
    save(make_plated_tile((50, 50), margin_frac=0.06, corner_frac=0.16),
         "StoreLogo.png")

    print("Done.")


if __name__ == "__main__":
    main()

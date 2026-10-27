#!/usr/bin/env python3
"""Composes the README's screenshots and banner from raw window grabs, as Glate does.

    scripts/screenshots.py <raw-dir>   # writes screenshots/*.png

Run through scripts/screenshots.sh, which makes the raw grabs (tst_screenshots) in <raw-dir>:
    proofread.png  popup.png  tone.png  reply.png  translate.png  summarize.png  models.png
A grab is the window's inside only (the desktop draws the title bar), so each gets a dark
title bar like KDE's Breeze. Style: the icon's blue in a gradient with soft circles, a
headline, a one-line subtitle, the window rounded on a shadow.
"""

import functools
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont  # type: ignore[import-not-found]  # uv run --with pillow

ROOT = Path(__file__).resolve().parent.parent
FONT_BOLD = "/usr/share/fonts/noto/NotoSans-Bold.ttf"
FONT_REG = "/usr/share/fonts/noto/NotoSans-Regular.ttf"
FONT_MED = "/usr/share/fonts/noto/NotoSans-Medium.ttf"
ICON = ROOT / "app/data/com.ktechpit.paraphrase.svg"

TOP = (0x35, 0x55, 0xD6)  # the icon's blue (#2747C9), a little lighter at the top
BOTTOM = (0x0E, 0x14, 0x30)  # towards a dark indigo ground
SUBTITLE = (0xD9, 0xE0, 0xFA)
TITLEBAR = (0x1C, 0x1D, 0x20)
TITLE_TEXT = (0xDD, 0xDE, 0xE1)


def font(path: str, size: int) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(path, size)


@functools.cache
def icon(size: int) -> Image.Image:
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "icon.png"
        subprocess.run(
            [
                "magick",
                "-background",
                "none",
                "-density",
                "600",
                str(ICON),
                "-resize",
                f"{size}x{size}",
                str(out),
            ],
            check=True,
        )
        return Image.open(out).convert("RGBA").copy()


def background(w: int, h: int) -> Image.Image:
    """Vertical blue gradient with two soft, translucent circles."""
    img = Image.new("RGB", (w, h), TOP)
    draw = ImageDraw.Draw(img)
    for y in range(h):
        t = y / max(1, h - 1)
        draw.line(
            [(0, y), (w, y)], fill=tuple(int(TOP[i] * (1 - t) + BOTTOM[i] * t) for i in range(3))
        )
    overlay = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    od = ImageDraw.Draw(overlay)
    r = int(h * 0.55)
    od.ellipse(
        (w - r * 1.1, -r * 0.5, w - r * 1.1 + r * 1.6, -r * 0.5 + r * 1.6), fill=(255, 255, 255, 14)
    )
    od.ellipse(
        (-r * 0.6, h - r * 0.9, -r * 0.6 + r * 1.4, h - r * 0.9 + r * 1.4), fill=(255, 255, 255, 12)
    )
    return Image.alpha_composite(img.convert("RGBA"), overlay)


def with_titlebar(shot: Image.Image, title: str) -> Image.Image:
    """The grab under a dark title bar: the icon, the title in the middle, the three buttons."""
    bar = 36
    out = Image.new("RGB", (shot.width, shot.height + bar), TITLEBAR)
    out.paste(shot.convert("RGB"), (0, bar))
    d = ImageDraw.Draw(out)
    logo = icon(20)
    out.paste(logo, (10, (bar - 20) // 2), logo)
    f = font(FONT_REG, 14)
    tw = d.textlength(title, font=f)
    d.text(((shot.width - tw) / 2, 9), title, font=f, fill=TITLE_TEXT)
    cy, x = bar // 2, shot.width - 20
    d.line((x - 5, cy - 5, x + 5, cy + 5), fill=TITLE_TEXT, width=2)  # close
    d.line((x - 5, cy + 5, x + 5, cy - 5), fill=TITLE_TEXT, width=2)
    x -= 34
    d.line((x - 5, cy + 2, x, cy - 3, x + 5, cy + 2), fill=TITLE_TEXT, width=2)  # maximise
    x -= 34
    d.line((x - 5, cy - 2, x, cy + 3, x + 5, cy - 2), fill=TITLE_TEXT, width=2)  # minimise
    d.line((0, bar - 1, shot.width, bar - 1), fill=(0x2E, 0x30, 0x34))
    return out


def framed(shot: Image.Image, width: int, radius: int = 14) -> Image.Image:
    """The window scaled to `width`, rounded, on a soft shadow (RGBA)."""
    scale = width / shot.width
    shot = shot.convert("RGB").resize((width, int(shot.height * scale)), Image.LANCZOS)
    mask = Image.new("L", shot.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle(
        (0, 0, shot.width - 1, shot.height - 1), radius=radius, fill=255
    )
    pad = 60
    canvas = Image.new("RGBA", (shot.width + pad * 2, shot.height + pad * 2), (0, 0, 0, 0))
    shadow = Image.new("RGBA", canvas.size, (0, 0, 0, 0))
    ImageDraw.Draw(shadow).rounded_rectangle(
        (pad, pad + 18, pad + shot.width, pad + shot.height + 18),
        radius=radius,
        fill=(0, 0, 0, 150),
    )
    canvas = Image.alpha_composite(canvas, shadow.filter(ImageFilter.GaussianBlur(28)))
    rounded = Image.new("RGBA", shot.size, (0, 0, 0, 0))
    rounded.paste(shot, (0, 0), mask)
    # A hairline so the dark window separates from the dark ground.
    ImageDraw.Draw(rounded).rounded_rectangle(
        (0, 0, shot.width - 1, shot.height - 1), radius=radius, outline=(255, 255, 255, 40)
    )
    canvas.alpha_composite(rounded, (pad, pad))
    return canvas


def text_centered(
    draw: ImageDraw.ImageDraw,
    y: int,
    text: str,
    fnt: ImageFont.FreeTypeFont,
    fill: tuple[int, int, int],
    w: int,
) -> None:
    draw.text(((w - draw.textlength(text, font=fnt)) / 2, y), text, font=fnt, fill=fill)


def compose(
    shot: Image.Image,
    headline: str,
    subtitle: str,
    size: tuple[int, int] = (1280, 800),
    top: int = 56,
) -> Image.Image:
    w, h = size
    img = background(w, h)
    d = ImageDraw.Draw(img)
    text_centered(d, top, headline, font(FONT_BOLD, int(h * 0.058)), (255, 255, 255), w)
    text_centered(d, top + int(h * 0.085), subtitle, font(FONT_REG, int(h * 0.026)), SUBTITLE, w)
    # Fit the window into the box under the text, keeping its aspect ratio.
    box_top = top + int(h * 0.15)
    box_w, box_h = int(w * 0.80), h - box_top - int(h * 0.05)
    scale = min(box_w / shot.width, box_h / shot.height, 1.25)
    fr = framed(shot, int(shot.width * scale))
    inner_h = int(shot.height * scale)
    y = box_top + (box_h - inner_h) // 2 - 60  # 60 = frame padding
    img.alpha_composite(fr, ((w - fr.width) // 2, y))
    return img.convert("RGB")


def banner(window: Image.Image, size: tuple[int, int] = (2160, 720)) -> Image.Image:
    w, h = size
    img = background(w, h)
    d = ImageDraw.Draw(img)
    x0 = 150
    logo = icon(150)
    img.alpha_composite(logo, (x0, 112))
    d.text((x0 + 180, 100), "Paraphrase", font=font(FONT_BOLD, 104), fill=(255, 255, 255))
    d.text(
        (x0, 304),
        "Write better. Nothing leaves your computer.",
        font=font(FONT_REG, 46),
        fill=(255, 255, 255),
    )
    d.text(
        (x0, 380),
        "An AI writing assistant for Linux that runs on your own computer.",
        font=font(FONT_REG, 30),
        fill=SUBTITLE,
    )
    # Translucent pills need their own layer: ImageDraw writes alpha, it does not blend.
    pf = font(FONT_MED, 27)
    layer = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ld = ImageDraw.Draw(layer)
    x = x0
    for pill in ("Proofread", "Tone", "Reply", "Summarize", "Translate"):
        tw = ld.textlength(pill, font=pf)
        ld.rounded_rectangle((x, 470, x + tw + 56, 528), radius=29, fill=(255, 255, 255, 40))
        ld.text((x + 28, 481), pill, font=pf, fill=(255, 255, 255, 255))
        x += tw + 56 + 22
    img = Image.alpha_composite(img, layer)
    fr = framed(window, 820, radius=16)
    img.alpha_composite(fr, (w - fr.width - 30, (h - fr.height) // 2))
    return img.convert("RGB")


def social(window: Image.Image, size: tuple[int, int] = (1280, 640)) -> Image.Image:
    """GitHub's social preview (2:1): the name and tagline over the window, cut at the bottom."""
    w, h = size
    img = background(w, h)
    d = ImageDraw.Draw(img)
    name_font, tag_font = font(FONT_BOLD, 64), font(FONT_REG, 30)
    logo = icon(72)
    name_w = d.textlength("Paraphrase", font=name_font)
    x = int((w - (72 + 20 + name_w)) / 2)
    img.alpha_composite(logo, (x, 44))
    d.text((x + 92, 34), "Paraphrase", font=name_font, fill=(255, 255, 255))
    text_centered(d, 136, "Offline AI writing assistant for Linux", tag_font, SUBTITLE, w)
    fr = framed(window, 1000, radius=14)
    img.alpha_composite(fr, ((w - fr.width) // 2, 140))
    return img.convert("RGB")


PLAN = [
    (
        "00-proofread",
        "proofread",
        "Paraphrase",
        "Fix your writing on your own computer",
        "Proofread shows every change, each with its own Undo. No account, no internet.",
    ),
    (
        "01-popup",
        "popup",
        None,
        "Over any app",
        "Select text anywhere, press Ctrl+Alt+Space, and put the fixed text back where it was.",
    ),
    (
        "02-tone",
        "tone",
        "Paraphrase",
        "Say it the right way",
        "Professional, polite, friendly, concise and more, keeping every name, date and number.",
    ),
    (
        "03-reply",
        "reply",
        "Paraphrase",
        "Replies, written for you",
        "Paste the message, note what you want to say, and get a complete email or chat reply.",
    ),
    (
        "04-translate",
        "translate",
        "Paraphrase",
        "Translate, Hindi and Hinglish included",
        "Into English, Hindi, Hinglish and 20 more languages, with names and numbers kept.",
    ),
    (
        "05-summarize",
        "summarize",
        "Paraphrase",
        "The point of a long message",
        "Summaries of long emails and documents, with the dates and amounts that matter.",
    ),
    (
        "06-models",
        "models",
        "Paraphrase settings",
        "Runs on your computer",
        "Open models, downloaded once and checked. Your text never leaves the machine.",
    ),
]


def main(raw_dir: str) -> None:
    raw = Path(raw_dir)
    out = ROOT / "screenshots"
    out.mkdir(parents=True, exist_ok=True)
    framed_windows = {}
    for name, key, title, head, sub in PLAN:
        window = Image.open(raw / f"{key}.png")
        if title:  # the popup has no title bar: it is a frameless window
            window = with_titlebar(window, title)
        framed_windows[key] = window
        compose(window, head, sub).save(out / f"{name}.png", optimize=True)
    banner(framed_windows["proofread"]).save(out / "banner.png", optimize=True)
    social(framed_windows["proofread"]).save(out / "social.png", optimize=True)
    for f in sorted(out.glob("*.png")):
        print(f.relative_to(ROOT), Image.open(f).size, f.stat().st_size // 1024, "KB")


if __name__ == "__main__":
    main(sys.argv[1])

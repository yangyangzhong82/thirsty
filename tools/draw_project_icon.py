"""Compose the project icon from the mod's existing pixel-art flask."""
from pathlib import Path

from PIL import Image, ImageDraw

from draw_item_textures import flask

ROOT = Path(__file__).resolve().parents[1]


def main():
    icon = Image.new("RGBA", (64, 64), "#202a2c")
    draw = ImageDraw.Draw(icon)
    draw.polygon(
        [(8, 2), (55, 2), (61, 8), (61, 55), (55, 61), (8, 61), (2, 55), (2, 8)],
        fill="#344846",
    )
    draw.line([(9, 4), (54, 4), (59, 9)], fill="#59746a", width=2)
    draw.line([(59, 54), (54, 59), (9, 59), (4, 54)], fill="#172526", width=2)
    draw.ellipse((14, 51, 51, 55), fill="#223331")
    sprite = flask(4, "clean").resize((48, 48), Image.Resampling.NEAREST)
    icon.alpha_composite(sprite, (8, 6))

    # A cyan droplet keeps the thirst theme legible at avatar sizes.
    drop = [(15, 8), (18, 12), (18, 14), (21, 18), (21, 23), (19, 26),
            (16, 28), (12, 28), (9, 26), (7, 23), (7, 19), (10, 14)]
    draw.polygon(drop, fill="#192d33")
    draw.polygon([(15, 11), (16, 15), (19, 19), (19, 23), (17, 25),
                  (13, 26), (10, 24), (9, 21), (11, 17)], fill="#37cbe0")
    draw.line([(13, 17), (11, 20), (11, 22)], fill="#cefff5", width=2)
    draw.line([(14, 25), (17, 24), (18, 22)], fill="#2187ad", width=2)
    destination = ROOT / "icon.png"
    icon.resize((512, 512), Image.Resampling.NEAREST).save(destination)
    print(f"Saved {destination}")


if __name__ == "__main__":
    main()

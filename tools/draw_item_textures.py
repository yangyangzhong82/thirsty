"""Draw Thirsty's editable 32 px inventory sprites and a preview sheet."""
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "packs/resource_packs/thirsty/textures/items"


def flask(sips=0, dirty=False):
    image = Image.new("RGBA", (32, 32))
    d = ImageDraw.Draw(image)

    # Carrying loop, bottle neck, and the stepped silhouette of a hip flask.
    d.line([(20, 8), (23, 6), (26, 6), (28, 8), (28, 12), (26, 14)], fill="#272b31", width=3)
    d.line([(21, 8), (24, 7), (26, 7), (27, 9), (27, 12), (26, 13)], fill="#a77a50")
    d.rectangle((13, 5, 19, 10), fill="#24313a")
    d.rectangle((14, 6, 18, 10), fill="#9ab6bb")
    d.line((14, 6, 14, 9), fill="#e3f3e9")
    outline = [(11, 8), (20, 8), (20, 9), (23, 9), (23, 10), (25, 10),
               (25, 12), (27, 12), (27, 27), (25, 27), (25, 29), (23, 29),
               (23, 30), (9, 30), (9, 29), (7, 29), (7, 27), (5, 27),
               (5, 14), (6, 14), (6, 11), (8, 11), (8, 10), (11, 10)]
    d.polygon(outline, fill="#252e36")
    d.polygon([(11, 10), (20, 10), (20, 11), (24, 11), (24, 13), (26, 13),
               (26, 26), (24, 26), (24, 28), (22, 29), (10, 29), (8, 27),
               (6, 26), (6, 15), (7, 15), (7, 12), (11, 12)], fill="#8d6044")
    d.polygon([(11, 11), (19, 11), (20, 12), (23, 12), (24, 14), (24, 26),
               (22, 28), (11, 28), (9, 26), (9, 15)], fill="#64828a")
    d.polygon([(11, 12), (19, 12), (21, 13), (13, 13), (11, 15), (11, 26),
               (10, 25), (10, 15)], fill="#aec6be")
    d.line((12, 12, 18, 12), fill="#e2ecce")
    d.polygon([(22, 13), (23, 14), (23, 25), (21, 27), (12, 27), (12, 28),
               (22, 28), (24, 26), (24, 14)], fill="#3e5863")
    d.line((13, 28, 21, 28), fill="#4c6972")

    # Warm leather side panels with stitches and rivets.
    d.line([(10, 11), (8, 13), (7, 16), (7, 25), (9, 28), (11, 29)], fill="#c59260")
    d.line([(25, 14), (25, 26), (23, 28), (23, 29)], fill="#593d33")
    for y in (15, 19, 23):
        d.point((8, y), fill="#f0c488")
        d.point((25, y + 1), fill="#b17c50")
    for x, y in ((9, 12), (24, 13), (10, 27), (24, 26)):
        d.rectangle((x, y, x + 1, y + 1), fill="#503a31")
        d.point((x, y), fill="#e7bc76")

    # Recessed glass window; liquid height is shared by every water quality.
    d.rounded_rectangle((12, 14, 22, 26), radius=2, fill="#202f3c")
    d.line([(13, 14), (21, 14), (22, 15), (22, 24)], fill="#bdd0c8")
    d.line([(13, 26), (21, 26), (22, 25)], fill="#38515c")
    d.rectangle((13, 16, 21, 24), fill="#2e4652")
    d.rectangle((14, 15, 20, 25), fill="#344f5c")
    if sips:
        top = 25 - sips * 2
        colors = ("#bac76d", "#7e9c49", "#536e3c") if dirty else ("#9bfff1", "#32bad0", "#227897")
        d.rectangle((14, top, 20, 25), fill=colors[1])
        d.rectangle((13, max(16, top + 1), 21, 24), fill=colors[1])
        d.line((14, top, 20, top), fill=colors[0])
        d.line((14, 25, 20, 25), fill=colors[2])
        d.line((20, top + 1, 20, 24), fill=colors[2])
        if dirty:
            for x, y in ((15, 24), (18, 23), (16, 21), (19, 19)):
                if y > top:
                    d.point((x, y), fill="#bbc080")
        else:
            d.point((17, min(24, top + 2)), fill="#c0fff0")
            if sips > 2:
                d.point((18, 22), fill="#72e6e1")
    d.line((14, 16, 14, 20), fill=(198, 243, 233, 210))
    d.point((15, 15), fill="#f0fff4")
    for y in (18, 20, 22, 24):
        d.point((22, y), fill="#e2dbaf")

    # Knurled metal lid with copper band and a bright top-left edge.
    d.rectangle((11, 2, 20, 5), fill="#252f38")
    d.rectangle((12, 1, 19, 2), fill="#38434b")
    d.rectangle((12, 2, 19, 4), fill="#7a9296")
    d.line((12, 2, 18, 2), fill="#e0efdb")
    d.line((12, 5, 19, 5), fill="#b58b57")
    for x in (13, 16, 19):
        d.point((x, 4), fill="#c3d6cf")
    return image


def purified_bottle():
    image = Image.new("RGBA", (32, 32))
    d = ImageDraw.Draw(image)
    # Subtle blue glass body with shaped shoulders and a broad weighted base.
    d.polygon([(12, 6), (20, 6), (20, 11), (22, 13), (24, 14), (26, 17),
               (26, 26), (24, 29), (22, 30), (10, 30), (8, 29), (6, 26),
               (6, 17), (8, 14), (10, 13), (12, 11)], fill="#294451")
    d.polygon([(13, 7), (19, 7), (19, 12), (22, 14), (23, 15), (25, 18),
               (25, 26), (23, 28), (21, 29), (11, 29), (9, 28), (7, 25),
               (7, 18), (9, 15), (13, 12)], fill=(136, 204, 214, 145))
    d.polygon([(13, 8), (15, 8), (15, 12), (11, 15), (9, 18), (9, 25),
               (11, 27), (10, 28), (8, 26), (8, 18), (10, 15), (13, 12)], fill="#c6f3ec")
    d.polygon([(9, 19), (12, 18), (17, 19), (22, 18), (24, 19), (24, 26),
               (22, 28), (11, 28), (8, 25), (8, 20)], fill="#38afcf")
    d.polygon([(20, 20), (24, 19), (24, 26), (22, 28), (11, 28), (10, 27),
               (20, 27), (22, 25), (22, 21)], fill="#287baf")
    d.line([(9, 19), (12, 18), (17, 19), (22, 18), (24, 19)], fill="#b3fff0")
    d.line((9, 21, 9, 25), fill="#e0fff7")
    d.line((10, 26, 12, 28), fill="#96ebec")
    d.line((13, 29, 20, 29), fill="#b3dedc")
    d.line((24, 21, 24, 25), fill="#70c4db")
    d.line((23, 15, 24, 17), fill="#c6f9ee")
    d.line((18, 8, 18, 11), fill="#8bb5c4")

    # Purity seal: a luminous droplet on a small silver medallion.
    d.polygon([(16, 20), (19, 22), (19, 25), (16, 27), (13, 25), (13, 22)], fill="#266781")
    d.polygon([(16, 20), (18, 22), (18, 25), (16, 26), (14, 25), (14, 22)], fill="#d3f6dc")
    d.polygon([(16, 21), (15, 23), (15, 24), (16, 25), (17, 24), (17, 23)], fill="#38b7d2")
    d.point((15, 23), fill="#9afff1")
    d.point((20, 21), fill="#b0faf2")
    d.point((12, 23), fill="#76d9e6")

    # Wrapped cork, metal lip, and a teal neck ribbon.
    d.rectangle((11, 2, 21, 6), fill="#3a3031")
    d.rectangle((12, 1, 20, 2), fill="#514335")
    d.rectangle((12, 2, 20, 5), fill="#b3895f")
    d.line((12, 2, 19, 2), fill="#f0d199")
    d.rectangle((18, 3, 20, 5), fill="#8e6347")
    for x, y in ((13, 4), (16, 3), (17, 5)):
        d.point((x, y), fill="#74513d")
    d.line((11, 6, 21, 6), fill="#dde1b4")
    d.rectangle((12, 8, 20, 9), fill="#286673")
    d.line((12, 8, 19, 8), fill="#63c9ba")
    d.polygon([(19, 9), (22, 10), (23, 13), (21, 12)], fill="#3d9a9d")
    return image


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    sprites = {"flask": flask(), "purified_water_bottle": purified_bottle()}
    for quality in ("clean", "dirty"):
        for sips in range(1, 5):
            sprites[f"{quality}_flask_{sips}"] = flask(sips, quality == "dirty")
    for name, sprite in sprites.items():
        sprite.save(OUTPUT / f"{name}.png")

    # Nearest-neighbor preview at a useful size; original PNGs remain 32 x 32.
    sheet = Image.new("RGB", (1000, 472), "#18212a")
    d = ImageDraw.Draw(sheet)
    layout = ["flask"] + [f"clean_flask_{n}" for n in range(1, 5)]
    layout += ["purified_water_bottle"] + [f"dirty_flask_{n}" for n in range(1, 5)]
    font = ImageFont.load_default(size=14)
    for i, name in enumerate(layout):
        x, y = (i % 5) * 200, (i // 5) * 236
        d.rounded_rectangle((x + 5, y + 5, x + 194, y + 228), radius=8, fill="#25333e")
        sprite = sprites[name].resize((192, 192), Image.Resampling.NEAREST)
        sheet.paste(sprite, (x + 4, y + 5), sprite)
        label = name.replace("_", " ").title()
        if name == "purified_water_bottle":
            label = "Purified Water Bottle"
        d.text((x + 100, y + 207), label, font=font, anchor="mm", fill="#d0e6e5")
    preview = ROOT / "build/texture_preview.png"
    preview.parent.mkdir(exist_ok=True)
    sheet.save(preview)
    print(f"Wrote {len(sprites)} sprites and {preview}")


if __name__ == "__main__":
    main()

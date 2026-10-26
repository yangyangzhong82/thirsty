"""Draw Thirsty's editable 32 px inventory sprites and a preview sheet."""
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont
from generate_items import QUALITIES

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "packs/resource_packs/thirsty/textures/items"


WATER_COLORS = {
    "dirty": ("#b9a26c", "#8c713c", "#594929"),
    "slightly_dirty": ("#c4d592", "#8ca65c", "#587740"),
    "acceptable": ("#b1e9ed", "#68b1c0", "#3c7d98"),
    "clean": ("#9bfff1", "#32bad0", "#227897"),
}


def flask(sips=0, quality="clean"):
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
        colors = WATER_COLORS[quality]
        d.rectangle((14, top, 20, 25), fill=colors[1])
        d.rectangle((13, max(16, top + 1), 21, 24), fill=colors[1])
        d.line((14, top, 20, top), fill=colors[0])
        d.line((14, 25, 20, 25), fill=colors[2])
        d.line((20, top + 1, 20, 24), fill=colors[2])
        if quality in ("dirty", "slightly_dirty"):
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


def water_bottle(quality="clean"):
    # Use Mojang's original 16px bottle, not a newly drawn silhouette.
    assets = ROOT / "tools/assets/vanilla"
    image = Image.open(assets / "potion_bottle_drinkable.png").convert("RGBA")
    if quality == "clean":
        return image
    mask = Image.open(assets / "potion_overlay.png").convert("RGBA")
    color = WATER_COLORS[quality][1].lstrip("#")
    target = tuple(int(color[i:i + 2], 16) for i in (0, 2, 4))
    for y in range(image.height):
        for x in range(image.width):
            r, g, b, alpha = image.getpixel((x, y))
            # Recolor blue liquid only; keep the glass outline and highlights intact.
            if mask.getpixel((x, y))[3] and b > r * 2.5:
                image.putpixel((x, y), tuple(round(v * b / 196) for v in target) + (alpha,))
    return image


def tint(image, multiply):
    """Recolor a sprite's body while keeping its alpha and shading."""
    pixels = image.load()
    for y in range(image.height):
        for x in range(image.width):
            r, g, b, a = pixels[x, y]
            if a:
                pixels[x, y] = tuple(min(255, int(v * m)) for v, m in zip((r, g, b), multiply)) + (a,)
    return image


def scaled_flask(sips, capacity, quality, multiply):
    # Water height follows the 4-sip window, so other capacities map onto it.
    level = round(sips * 4 / capacity) if sips else 0
    if sips and not level:
        level = 1
    image = flask(level, quality)
    return tint(image, multiply) if multiply else image


def charcoal_filter():
    image = Image.new("RGBA", (32, 32))
    d = ImageDraw.Draw(image)
    d.rounded_rectangle((8, 6, 24, 27), radius=3, fill="#3b2a1e")
    d.rectangle((9, 7, 23, 11), fill="#efe6cf")
    d.rectangle((9, 12, 23, 19), fill="#2a2a2c")
    for x, y in ((11, 14), (15, 16), (19, 13), (13, 18), (21, 17)):
        d.point((x, y), fill="#5a5a60")
    d.rectangle((9, 20, 23, 26), fill="#d9c58c")
    for x, y in ((11, 22), (16, 24), (20, 21)):
        d.point((x, y), fill="#b29b5e")
    d.line((9, 7, 9, 25), fill="#7a5a3a")
    return image


# id -> (capacity, color multiplier or None)
CONTAINERS = {"waterskin": (2, (1.15, 0.85, 0.65)), "flask": (4, None), "large_flask": (8, (1.25, 0.8, 0.55))}


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    sprites = {"charcoal_filter": charcoal_filter()}
    for quality in ("dirty", "slightly_dirty", "acceptable"):
        sprites[f"{quality}_water_bottle"] = water_bottle(quality)
    for cid, (capacity, multiply) in CONTAINERS.items():
        sprites[cid] = scaled_flask(0, capacity, "clean", multiply)
        for quality in QUALITIES:
            for sips in range(1, capacity + 1):
                sprites[f"{quality}_{cid}_{sips}"] = scaled_flask(sips, capacity, quality, multiply)
    for name, sprite in sprites.items():
        sprite.save(OUTPUT / f"{name}.png")

    # Include vanilla water in the preview only; it is not a generated custom item.
    preview_sprites = {**sprites, "vanilla_water_bottle": water_bottle()}
    # Nearest-neighbor preview at a useful size.
    sheet = Image.new("RGB", (1000, 1180), "#18212a")
    d = ImageDraw.Draw(sheet)
    layout = ["waterskin", "flask", "large_flask", "charcoal_filter", "vanilla_water_bottle"]
    for quality in QUALITIES:
        bottle = "vanilla_water_bottle" if quality == "clean" else f"{quality}_water_bottle"
        layout += [bottle] + [f"{quality}_flask_{n}" for n in range(1, 5)]
    font = ImageFont.load_default(size=14)
    for i, name in enumerate(layout):
        x, y = (i % 5) * 200, (i // 5) * 236
        d.rounded_rectangle((x + 5, y + 5, x + 194, y + 228), radius=8, fill="#25333e")
        sprite = preview_sprites[name].resize((192, 192), Image.Resampling.NEAREST)
        sheet.paste(sprite, (x + 4, y + 5), sprite)
        label = name.replace("_", " ").title()
        label = label.replace("Slightly Dirty", "Slightly Dirty\n")
        if name == "vanilla_water_bottle":
            label = "Vanilla Water (Pure)"
        d.text((x + 100, y + 207), label, font=font, anchor="mm", fill="#d0e6e5")
    preview = ROOT / "build/texture_preview.png"
    preview.parent.mkdir(exist_ok=True)
    sheet.save(preview)
    print(f"Wrote {len(sprites)} sprites and {preview}")


if __name__ == "__main__":
    main()

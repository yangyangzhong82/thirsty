"""Generate Thirsty's item, recipe, texture-atlas, and language files.

Every water container is described once in CONTAINERS; run this after changing
the table, then run draw_item_textures.py to redraw the sprites.
"""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BEHAVIOR = ROOT / "packs/behavior_packs/thirsty"
RESOURCE = ROOT / "packs/resource_packs/thirsty"

# id, capacity, Chinese name, English name, crafting recipe.
CONTAINERS = [
    {
        "id": "waterskin", "capacity": 2, "zh": "水袋", "en": "Waterskin",
        "pattern": [" S ", "L L", " L "],
        "key": {"S": "minecraft:string", "L": "minecraft:leather"},
        "unlock": "minecraft:leather",
    },
    {
        "id": "flask", "capacity": 4, "zh": "水壶", "en": "Flask",
        "empty_zh": "空水壶", "empty_en": "Empty Flask",
        "pattern": ["I I", "I I", " I "],
        "key": {"I": "minecraft:iron_ingot"},
        "unlock": "minecraft:iron_ingot",
    },
    {
        "id": "large_flask", "capacity": 8, "zh": "铜水壶", "en": "Copper Canteen",
        "pattern": ["CCC", "CFC", "CCC"],
        "key": {"C": "minecraft:copper_ingot", "F": "thirsty:flask"},
        "unlock": "thirsty:flask",
    },
]

QUALITIES = {
    "dirty": ("脏水", "Dirty"),
    "slightly_dirty": ("有点脏的", "Slightly Dirty"),
    "acceptable": ("能接受的", "Acceptable"),
    "clean": ("纯净", "Pure"),
}
WATER_BOTTLE = {"item": "minecraft:potion", "data": 0}
BOTTLES = {
    "dirty": "thirsty:dirty_water_bottle",
    "slightly_dirty": "thirsty:slightly_dirty_water_bottle",
    "acceptable": "thirsty:acceptable_water_bottle",
    "clean": WATER_BOTTLE,
}
FILTER = "thirsty:charcoal_filter"


def write_json(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=4) + "\n", encoding="utf-8", newline="\n")


def filled(container, quality, sips):
    return f"thirsty:{quality}_{container['id']}_{sips}" if sips else f"thirsty:{container['id']}"


def item(identifier, category="none", stack=1, food_converts_to=None, liquid_clipped=False):
    name = identifier.split(":")[1]
    components = {
        "minecraft:display_name": {"value": f"item.{identifier}.name"},
        "minecraft:icon": {"textures": {"default": f"thirsty_{name}"}},
        "minecraft:max_stack_size": stack,
    }
    if liquid_clipped:
        components["minecraft:liquid_clipped"] = True
    if food_converts_to:
        components["minecraft:use_animation"] = "drink"
        components["minecraft:use_modifiers"] = {"use_duration": 1.6, "movement_modifier": 0.35}
        components["minecraft:food"] = {
            "nutrition": 0,
            "saturation_modifier": 0,
            "can_always_eat": True,
            "using_converts_to": food_converts_to,
        }
    return {
        "format_version": "1.21.90",
        "minecraft:item": {
            "description": {"identifier": identifier, "menu_category": {"category": category}},
            "components": components,
        },
    }


def shapeless(identifier, ingredients, results, unlock):
    return {
        "format_version": "1.20.10",
        "minecraft:recipe_shapeless": {
            "description": {"identifier": identifier},
            "tags": ["crafting_table"],
            "ingredients": [i if isinstance(i, dict) else {"item": i} for i in ingredients],
            "unlock": [{"item": unlock}],
            "result": [r if isinstance(r, dict) else {"item": r} for r in results],
        },
    }


def smelt(identifier, source, output):
    return {
        "format_version": "1.20.10",
        "minecraft:recipe_furnace": {
            "description": {"identifier": identifier},
            "tags": ["furnace", "campfire", "soul_campfire"],
            "input": source,
            "output": output["item"] if isinstance(output, dict) else output,
        },
    }


def main():
    for folder in (BEHAVIOR / "items", BEHAVIOR / "recipes"):
        folder.mkdir(parents=True, exist_ok=True)
        for old in folder.glob("*.json"):
            old.unlink()

    items, recipes = {}, {}
    zh = {}
    en = {}

    for quality in ("dirty", "slightly_dirty", "acceptable"):
        bottle = BOTTLES[quality]
        name = bottle.split(":")[1]
        items[name] = item(bottle, "equipment", food_converts_to="minecraft:glass_bottle")
        zh[name] = "脏水瓶" if quality == "dirty" else f"{QUALITIES[quality][0]}水瓶"
        en[name] = f"{QUALITIES[quality][1]} Water Bottle"
    items["charcoal_filter"] = item(FILTER, "items", stack=16)
    zh["charcoal_filter"], en["charcoal_filter"] = "木炭滤芯", "Charcoal Filter"

    grades = list(QUALITIES)
    for index, quality in enumerate(grades[:-1]):
        suffix = f"{quality}_water_bottle"
        source = BOTTLES[quality]
        improved = BOTTLES[grades[index + 1]]
        recipes[f"purify_{suffix}"] = smelt(f"thirsty:purify_{suffix}", source, improved)
        recipes[f"filter_{suffix}"] = shapeless(
            f"thirsty:filter_{suffix}", [source, FILTER], [BOTTLES[grades[index + 1]]], FILTER
        )
    recipes["charcoal_filter"] = {
        "format_version": "1.20.10",
        "minecraft:recipe_shaped": {
            "description": {"identifier": "thirsty:charcoal_filter"},
            "tags": ["crafting_table"],
            "pattern": ["P", "C", "S"],
            "key": {
                "P": {"item": "minecraft:paper"},
                "C": {"item": "minecraft:charcoal"},
                "S": {"item": "minecraft:sand"},
            },
            "unlock": [{"item": "minecraft:charcoal"}],
            "result": {"item": FILTER, "count": 2},
        },
    }

    for c in CONTAINERS:
        cid, cap = c["id"], c["capacity"]
        empty = filled(c, "clean", 0)
        items[cid] = item(empty, "equipment", liquid_clipped=True)
        zh[cid] = c.get("empty_zh", f"空{c['zh']}")
        en[cid] = c.get("empty_en", f"Empty {c['en']}")
        recipes[cid] = {
            "format_version": "1.20.10",
            "minecraft:recipe_shaped": {
                "description": {"identifier": f"thirsty:{cid}"},
                "tags": ["crafting_table"],
                "pattern": c["pattern"],
                "key": {k: {"item": v} for k, v in c["key"].items()},
                "unlock": [{"item": c["unlock"]}],
                "result": {"item": empty, "count": 1},
            },
        }
        recipes[f"fill_{cid}_bucket"] = shapeless(
            f"thirsty:fill_{cid}_bucket", [empty, "minecraft:water_bucket"],
            [filled(c, "dirty", cap), "minecraft:bucket"], empty,
        )

        for quality, (quality_zh, quality_en) in QUALITIES.items():
            bottle = BOTTLES[quality]
            for sips in range(1, cap + 1):
                name = f"{quality}_{cid}_{sips}"
                full = sips == cap
                items[name] = item(
                    filled(c, quality, sips), "equipment" if full else "none",
                    food_converts_to=filled(c, quality, sips - 1),
                )
                zh[name] = f"{c['zh']} · {quality_zh} {sips}/{cap}"
                en[name] = f"{quality_en} {c['en']} ({sips}/{cap})"
                recipes[f"fill_{name}"] = shapeless(
                    f"thirsty:fill_{name}", [filled(c, quality, sips - 1), bottle],
                    [filled(c, quality, sips), "minecraft:glass_bottle"], empty,
                )
                if quality != "clean":
                    suffix = f"{cid}_{sips}" if quality == "dirty" else name
                    improved = filled(c, grades[grades.index(quality) + 1], sips)
                    recipes[f"purify_{suffix}"] = smelt(f"thirsty:purify_{suffix}", filled(c, quality, sips), improved)
                    recipes[f"filter_{suffix}"] = shapeless(
                        f"thirsty:filter_{suffix}", [filled(c, quality, sips), FILTER], [improved], FILTER
                    )

    for name, data in items.items():
        write_json(BEHAVIOR / "items" / f"{name}.json", data)
    for name, data in recipes.items():
        write_json(BEHAVIOR / "recipes" / f"{name}.json", data)

    write_json(RESOURCE / "textures/item_texture.json", {
        "resource_pack_name": "thirsty",
        "texture_name": "atlas.items",
        "texture_data": {f"thirsty_{name}": {"textures": f"textures/items/{name}"} for name in items},
    })

    for code, names in (("zh_CN", zh), ("en_US", en)):
        lines = [f"item.thirsty:{name}.name={text}" for name, text in names.items()]
        lines += (
            ["thirsty.tooltip.restore=每口恢复 %s 点口渴值", "thirsty.tooltip.debuff=反胃概率：%s%%",
             "thirsty.tooltip.quality=水质：%s", "thirsty.tooltip.empty=空容器：水质由取水来源决定",
             "thirsty.tooltip.brewing.pure=可用于酿药", "thirsty.tooltip.brewing.impure=需净化至纯净才能酿药"]
            if code == "zh_CN" else
            ["thirsty.tooltip.restore=Restores %s thirst per sip", "thirsty.tooltip.debuff=Nausea chance: %s%%",
             "thirsty.tooltip.quality=Water quality: %s", "thirsty.tooltip.empty=Empty: quality depends on the water source",
             "thirsty.tooltip.brewing.pure=Ready for brewing",
             "thirsty.tooltip.brewing.impure=Purify fully before brewing"]
        )
        lines += [f"thirsty.quality.{quality}={labels[0 if code == 'zh_CN' else 1]}"
                  for quality, labels in QUALITIES.items()]
        (RESOURCE / f"texts/{code}.lang").write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")

    print(f"Wrote {len(items)} items and {len(recipes)} recipes")


if __name__ == "__main__":
    main()

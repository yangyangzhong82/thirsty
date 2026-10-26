#pragma once

#include <array>
#include <charconv>
#include <optional>
#include <string_view>
#include <utility>

namespace thirsty {

enum class WaterQuality : std::size_t { Dirty, SlightlyDirty, Acceptable, Pure };

inline constexpr std::array<std::string_view, 4> WaterQualityIds{
    "dirty", "slightly_dirty", "acceptable", "clean"
};

inline constexpr std::array<std::string_view, 4> WaterBottleIds{
    "thirsty:dirty_water_bottle", "thirsty:slightly_dirty_water_bottle", "thirsty:acceptable_water_bottle",
    "minecraft:potion"
};

inline std::optional<WaterQuality> parseWaterBottle(std::string_view name, int auxValue) {
    if (name == "minecraft:potion") {
        return auxValue == 0 ? std::optional{WaterQuality::Pure} : std::nullopt;
    }
    for (std::size_t i = 0; i < WaterBottleIds.size(); ++i) {
        if (name == WaterBottleIds[i]) return static_cast<WaterQuality>(i);
    }
    return std::nullopt;
}

struct WaterContainer {
    std::string_view kind;
    int              capacity;
    int              sips;
    WaterQuality     quality;
};

// Keep kinds in sync with the container tables in the two item generation scripts.
inline std::optional<WaterContainer> parseWaterContainer(std::string_view name) {
    constexpr std::pair<std::string_view, int> kinds[]{{"waterskin", 2}, {"flask", 4}, {"large_flask", 8}};
    if (!name.starts_with("thirsty:")) return std::nullopt;
    name.remove_prefix(8);
    for (auto [kind, capacity] : kinds) {
        if (name == kind) return WaterContainer{kind, capacity, 0, WaterQuality::Dirty};
    }
    std::optional<WaterQuality> quality;
    for (std::size_t i = 0; i < WaterQualityIds.size(); ++i) {
        auto prefix = WaterQualityIds[i];
        if (name.starts_with(prefix) && name.size() > prefix.size() && name[prefix.size()] == '_') {
            quality = static_cast<WaterQuality>(i);
            name.remove_prefix(prefix.size() + 1);
            break;
        }
    }
    if (!quality) return std::nullopt;
    for (auto [kind, capacity] : kinds) {
        if (name.size() < kind.size() + 2 || !name.starts_with(kind) || name[kind.size()] != '_') continue;
        int sips = 0;
        auto digits = name.substr(kind.size() + 1);
        auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), sips);
        if (error == std::errc{} && end == digits.data() + digits.size() && sips >= 1 && sips <= capacity) {
            return WaterContainer{kind, capacity, sips, *quality};
        }
    }
    return std::nullopt;
}

} // namespace thirsty

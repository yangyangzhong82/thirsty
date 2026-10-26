#pragma once

#include <map>
#include <string>

namespace thirsty {

inline constexpr int MaxThirst = 20;
inline constexpr int ThirstScale = 100;
inline constexpr int MaxThirstUnits = MaxThirst * ThirstScale;
inline constexpr int DrainPeriodTicks = 80;

struct ThirstConfig {
    int   version               = 7;
    double baseDrainPer4Seconds  = 0.07;
    std::map<std::string, double> biomeBaseDrainPer4Seconds{
        {"minecraft:desert", 0.14},
        {"minecraft:desert_hills", 0.14},
        {"minecraft:desert_mutated", 0.14},
        {"minecraft:savanna", 0.14},
        {"minecraft:savanna_mutated", 0.14},
        {"minecraft:savanna_plateau", 0.14},
        {"minecraft:savanna_plateau_mutated", 0.14},
        {"minecraft:mesa", 0.14},
        {"minecraft:mesa_bryce", 0.14},
        {"minecraft:mesa_plateau", 0.14},
        {"minecraft:mesa_plateau_mutated", 0.14},
        {"minecraft:mesa_plateau_stone", 0.14},
        {"minecraft:mesa_plateau_stone_mutated", 0.14},
        {"minecraft:jungle", 0.14},
        {"minecraft:jungle_edge", 0.14},
        {"minecraft:jungle_edge_mutated", 0.14},
        {"minecraft:jungle_hills", 0.14},
        {"minecraft:jungle_mutated", 0.14},
        {"minecraft:bamboo_jungle", 0.14},
        {"minecraft:bamboo_jungle_hills", 0.14},
        {"minecraft:mangrove_swamp", 0.14},
        {"minecraft:warm_ocean", 0.14},
        {"minecraft:deep_warm_ocean", 0.14},
        {"minecraft:hell", 0.21},
        {"minecraft:crimson_forest", 0.21},
        {"minecraft:warped_forest", 0.21},
        {"minecraft:soulsand_valley", 0.21},
        {"minecraft:basalt_deltas", 0.21},
    };
    double activityDrainPer4Seconds = 0.2;
    double jumpDrain             = 0.05;
    double breakBlockDrain       = 0.01;
    double attackDrain           = 0.02;
    double lowThirstThreshold    = 6.0;
    int   lowThirstFatigueAmplifier = 0;
    int   waterBottleRestore    = 6;
    int   dirtyWaterRestore      = 3;
    int   slightlyDirtyWaterRestore = 4;
    int   acceptableWaterRestore = 5;
    int   dirtyWaterDebuffChance = 60;
    int   slightlyDirtyWaterDebuffChance = 20;
    int   acceptableWaterDebuffChance = 1;
    int   dirtyWaterDebuffSeconds = 20;
    int   nauseaSecondsPerPoint  = 5;
    int   dirtyWaterPoisonChance = 30;
    int   dirtyWaterPoisonSeconds = 10;
    bool  poisonFromAnyImpureWater = false;
    int   handDrinkRestore       = 2;
    int   handDrinkDebuffChance  = 60;
    std::map<std::string, int> foodThirst{
        {"minecraft:melon_slice", 2},
        {"minecraft:sweet_berries", 1},
        {"minecraft:glow_berries", 1},
        {"minecraft:apple", 1},
        {"minecraft:honey_bottle", 3},
        {"minecraft:milk_bucket", 4},
        {"minecraft:mushroom_stew", 3},
        {"minecraft:beetroot_soup", 3},
        {"minecraft:rabbit_stew", 3},
        {"minecraft:rotten_flesh", -1},
        {"minecraft:dried_kelp", -1},
    };
    bool  damageWhenEmpty       = true;
    int   damageIntervalSeconds = 4;
    float damageAmount          = 1.0f;
    float hudOffsetX            = 0.0f;
    float hudOffsetY            = -20.0f;
};

} // namespace thirsty

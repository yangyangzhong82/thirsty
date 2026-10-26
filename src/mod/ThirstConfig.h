#pragma once

#include <map>
#include <string>

namespace thirsty {

inline constexpr int MaxThirst = 20;
inline constexpr int ThirstScale = 100;
inline constexpr int MaxThirstUnits = MaxThirst * ThirstScale;
inline constexpr int DrainPeriodTicks = 80;

struct ThirstConfig {
    int   version               = 7; // 配置版本号，不要手动修改。
    double baseDrainPer4Seconds  = 0.07; // 普通环境每 4 秒消耗的口渴值。
    // 指定生物群系每 4 秒的消耗；未列出的生物群系使用上面的默认值。
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
    double activityDrainPer4Seconds = 0.2; // 疾跑、游泳或滑翔时，每 4 秒额外消耗。
    double jumpDrain             = 0.05; // 每次跳跃消耗。
    double breakBlockDrain       = 0.01; // 成功挖掘一个方块消耗。
    double attackDrain           = 0.02; // 成功攻击一次消耗。
    double lowThirstThreshold    = 6.0; // 低于此值时获得挖掘疲劳。
    int   lowThirstFatigueAmplifier = 0; // 低口渴挖掘疲劳等级，0 表示 I 级。
    int   waterBottleRestore    = 6; // 纯净水瓶或纯净容器每口恢复。
    int   dirtyWaterRestore      = 3; // 脏水每口恢复。
    int   slightlyDirtyWaterRestore = 4; // 有点脏的水每口恢复。
    int   acceptableWaterRestore = 5; // 能接受的水每口恢复。
    int   dirtyWaterDebuffChance = 60; // 脏水导致反胃的概率（百分比）。
    int   slightlyDirtyWaterDebuffChance = 20; // 有点脏的水导致反胃的概率。
    int   acceptableWaterDebuffChance = 1; // 能接受的水导致反胃的概率。
    int   dirtyWaterDebuffSeconds = 20; // 反胃持续时间（秒）。
    int   nauseaSecondsPerPoint  = 5; // 反胃期间每隔多少秒额外减少 1 点，0 关闭额外消耗。
    int   dirtyWaterPoisonChance = 30; // 已经反胃时喝脏水导致中毒的概率。
    int   dirtyWaterPoisonSeconds = 10; // 上述中毒持续时间（秒）。
    bool  poisonFromAnyImpureWater = false; // 已经反胃时，是否喝所有非纯净水都可能中毒。
    int   handDrinkRestore       = 2; // 空手对水面饮水每次恢复。
    int   handDrinkDebuffChance  = 60; // 空手喝脏水的反胃概率；较干净的水使用对应水质概率。
    // 食物 ID 到口渴值变化的对应表，正数补水，负数扣除口渴值。
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
    bool  damageWhenEmpty       = true; // 口渴值为 0 时是否持续受到伤害。
    int   damageIntervalSeconds = 4; // 口渴值为 0 时的伤害间隔（秒）。
    float damageAmount          = 1.0f; // 每次伤害量，半颗心为 1.0。
    float hudOffsetX            = 0.0f; // 口渴条水平偏移，游戏 UI 单位。
    float hudOffsetY            = -20.0f; // 口渴条垂直偏移，负数表示向上。
};

inline const std::map<std::string, std::string> ThirstConfigComments{
    {"version", "配置版本号，不要修改。"},
    {"baseDrainPer4Seconds", "普通环境每 4 秒消耗多少口渴值，范围 0~20。"},
    {"biomeBaseDrainPer4Seconds", "各生物群系每 4 秒的消耗，范围 0~20；未列出的使用普通环境消耗。"},
    {"activityDrainPer4Seconds", "疾跑、游泳或滑翔时每 4 秒额外消耗，范围 0~20；同时进行只算一次。"},
    {"jumpDrain", "每次跳跃消耗，范围 0~20，0 关闭。"},
    {"breakBlockDrain", "成功挖掉一个方块消耗，范围 0~20，0 关闭。"},
    {"attackDrain", "成功攻击一次消耗，范围 0~20，0 关闭。"},
    {"lowThirstThreshold", "口渴值低于多少时获得挖掘疲劳，范围 0~20，0 关闭。"},
    {"lowThirstFatigueAmplifier", "挖掘疲劳等级，范围 0~255；0 是 I 级，1 是 II 级。"},
    {"waterBottleRestore", "纯净水每瓶或每口恢复多少口渴值，范围 0~20。"},
    {"dirtyWaterRestore", "脏水每瓶或每口恢复多少口渴值，范围 0~20。"},
    {"slightlyDirtyWaterRestore", "有点脏的水每瓶或每口恢复多少口渴值，范围 0~20。"},
    {"acceptableWaterRestore", "能接受的水每瓶或每口恢复多少口渴值，范围 0~20。"},
    {"dirtyWaterDebuffChance", "喝脏水的反胃概率，范围 0~100，0 关闭。"},
    {"slightlyDirtyWaterDebuffChance", "喝有点脏的水的反胃概率，范围 0~100，0 关闭。"},
    {"acceptableWaterDebuffChance", "喝能接受的水的反胃概率，范围 0~100，0 关闭。"},
    {"dirtyWaterDebuffSeconds", "喝水引起的反胃持续多少秒，范围 1~86400。"},
    {"nauseaSecondsPerPoint", "反胃期间每隔多少秒额外减少 1 点口渴值，范围 0~86400，0 关闭。"},
    {"dirtyWaterPoisonChance", "已经反胃时再喝脏水，中毒的概率，范围 0~100，0 关闭。"},
    {"dirtyWaterPoisonSeconds", "喝水引起的中毒持续多少秒，范围 1~86400。"},
    {"poisonFromAnyImpureWater", "true：已经反胃时喝任何非纯净水都可能中毒；false：只有脏水。"},
    {"handDrinkRestore", "空手对水面喝水每次恢复多少口渴值，范围 0~20。"},
    {"handDrinkDebuffChance", "空手喝脏水的反胃概率，范围 0~100；有点脏的水使用该水质的概率。"},
    {"foodThirst", "食物吃完后改变多少口渴值，范围 -20~20；正数补水，负数扣水，设为 0 可取消该食物的补水或扣水效果。"},
    {"damageWhenEmpty", "口渴值为 0 时是否持续受伤，true 开启，false 关闭。"},
    {"damageIntervalSeconds", "口渴值为 0 时每隔多少秒受伤，范围 1~86400。"},
    {"damageAmount", "每次受到多少伤害，不能小于 0；1.0 是半颗心。"},
    {"hudOffsetX", "口渴条左右位置，按游戏界面单位计算；正数向右，负数向左。"},
    {"hudOffsetY", "口渴条上下位置，按游戏界面单位计算；正数向下，负数向上。"},
};

} // namespace thirsty

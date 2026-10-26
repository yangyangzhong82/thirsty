#include "mod/MyMod.h"

#include <algorithm>
#include <cmath>

#include "ll/api/Config.h"
#include "ll/api/mod/RegisterHelper.h"
#include "mod/ThirstPacket.h"
#include "mod/ThirstPacks.h"
#include "mod/ThirstSystem.h"
#include "mod/WaterBrewing.h"

#ifdef LL_PLAT_C
#include "ThirstHud.h"
#endif

namespace my_mod {

MyMod& MyMod::getInstance() {
    static MyMod instance;
    return instance;
}

bool MyMod::load() {
    auto path = getSelf().getConfigDir() / "config.json";
    if (!ll::config::loadConfig(mConfig, path)) {
        ll::config::saveConfig(mConfig, path);
    }
    auto clampCost = [](double& value, double fallback) {
        value = std::isfinite(value) ? std::clamp(value, 0.0, static_cast<double>(thirsty::MaxThirst)) : fallback;
    };
    clampCost(mConfig.baseDrainPer4Seconds, 0.07);
    clampCost(mConfig.activityDrainPer4Seconds, 0.2);
    clampCost(mConfig.jumpDrain, 0.05);
    clampCost(mConfig.breakBlockDrain, 0.01);
    clampCost(mConfig.attackDrain, 0.02);
    clampCost(mConfig.lowThirstThreshold, 6.0);
    for (auto& [biome, cost] : mConfig.biomeBaseDrainPer4Seconds) {
        clampCost(cost, mConfig.baseDrainPer4Seconds);
    }
    mConfig.lowThirstFatigueAmplifier = std::clamp(mConfig.lowThirstFatigueAmplifier, 0, 255);
    mConfig.waterBottleRestore    = std::clamp(mConfig.waterBottleRestore, 0, thirsty::MaxThirst);
    mConfig.dirtyWaterRestore     = std::clamp(mConfig.dirtyWaterRestore, 0, thirsty::MaxThirst);
    mConfig.slightlyDirtyWaterRestore = std::clamp(mConfig.slightlyDirtyWaterRestore, 0, thirsty::MaxThirst);
    mConfig.acceptableWaterRestore = std::clamp(mConfig.acceptableWaterRestore, 0, thirsty::MaxThirst);
    mConfig.dirtyWaterDebuffChance = std::clamp(mConfig.dirtyWaterDebuffChance, 0, 100);
    mConfig.slightlyDirtyWaterDebuffChance = std::clamp(mConfig.slightlyDirtyWaterDebuffChance, 0, 100);
    mConfig.acceptableWaterDebuffChance = std::clamp(mConfig.acceptableWaterDebuffChance, 0, 100);
    mConfig.dirtyWaterDebuffSeconds = std::clamp(mConfig.dirtyWaterDebuffSeconds, 1, 86400);
    mConfig.nauseaSecondsPerPoint = std::clamp(mConfig.nauseaSecondsPerPoint, 0, 86400);
    mConfig.dirtyWaterPoisonChance = std::clamp(mConfig.dirtyWaterPoisonChance, 0, 100);
    mConfig.dirtyWaterPoisonSeconds = std::clamp(mConfig.dirtyWaterPoisonSeconds, 1, 86400);
    mConfig.handDrinkRestore      = std::clamp(mConfig.handDrinkRestore, 0, thirsty::MaxThirst);
    mConfig.handDrinkDebuffChance = std::clamp(mConfig.handDrinkDebuffChance, 0, 100);
    for (auto& [name, points] : mConfig.foodThirst) {
        points = std::clamp(points, -thirsty::MaxThirst, thirsty::MaxThirst);
    }
    mConfig.damageIntervalSeconds = std::clamp(mConfig.damageIntervalSeconds, 1, 86400);
    if (!std::isfinite(mConfig.damageAmount) || mConfig.damageAmount < 0.0f) mConfig.damageAmount = 1.0f;
    (void)thirsty::ThirstPacket::sRegistered;
    (void)thirsty::HandDrinkPacket::sRegistered;
    return thirsty::loadThirstPacks();
}

bool MyMod::enable() {
#ifdef LL_PLAT_C
    thirsty::enableThirstHud();
#endif
    thirsty::enableThirstSystem();
    thirsty::enableWaterBrewing();
    getSelf().getLogger().info("Thirsty enabled");
    return true;
}

bool MyMod::disable() {
    thirsty::disableWaterBrewing();
    thirsty::disableThirstSystem();
#ifdef LL_PLAT_C
    thirsty::disableThirstHud();
#endif
    return true;
}

} // namespace my_mod

LL_REGISTER_MOD(my_mod::MyMod, my_mod::MyMod::getInstance());

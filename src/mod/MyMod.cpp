#include "mod/MyMod.h"

#include <algorithm>
#include <cmath>

#include "ll/api/Config.h"
#include "ll/api/mod/RegisterHelper.h"
#include "mod/ThirstPacket.h"
#include "mod/ThirstPacks.h"
#include "mod/ThirstSystem.h"

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
    mConfig.secondsPerPoint       = std::clamp(mConfig.secondsPerPoint, 1, 86400);
    mConfig.sprintMultiplier      = std::clamp(mConfig.sprintMultiplier, 1, 100);
    mConfig.waterBottleRestore    = std::clamp(mConfig.waterBottleRestore, 0, thirsty::MaxThirst);
    mConfig.dirtyWaterDebuffChance = std::clamp(mConfig.dirtyWaterDebuffChance, 0, 100);
    mConfig.dirtyWaterDebuffSeconds = std::clamp(mConfig.dirtyWaterDebuffSeconds, 1, 86400);
    mConfig.damageIntervalSeconds = std::clamp(mConfig.damageIntervalSeconds, 1, 86400);
    if (!std::isfinite(mConfig.damageAmount) || mConfig.damageAmount < 0.0f) mConfig.damageAmount = 1.0f;
    (void)thirsty::ThirstPacket::sRegistered;
    return thirsty::loadThirstPacks();
}

bool MyMod::enable() {
#ifdef LL_PLAT_C
    thirsty::enableThirstHud();
#endif
    thirsty::enableThirstSystem();
    getSelf().getLogger().info("Thirsty enabled");
    return true;
}

bool MyMod::disable() {
    thirsty::disableThirstSystem();
#ifdef LL_PLAT_C
    thirsty::disableThirstHud();
#endif
    return true;
}

} // namespace my_mod

LL_REGISTER_MOD(my_mod::MyMod, my_mod::MyMod::getInstance());

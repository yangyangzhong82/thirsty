#pragma once

namespace thirsty {

inline constexpr int MaxThirst = 20;

struct ThirstConfig {
    int   version               = 2;
    int   secondsPerPoint       = 60;
    int   sprintMultiplier      = 2;
    int   waterBottleRestore    = 6;
    int   dirtyWaterDebuffChance = 30;
    int   dirtyWaterDebuffSeconds = 20;
    bool  damageWhenEmpty       = true;
    int   damageIntervalSeconds = 4;
    float damageAmount          = 1.0f;
    float hudOffsetX            = 0.0f;
    float hudOffsetY            = -20.0f;
};

} // namespace thirsty

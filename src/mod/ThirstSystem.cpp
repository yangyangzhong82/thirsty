#include "mod/ThirstSystem.h"

#include <algorithm>
#include <unordered_map>

#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerRespawnEvent.h"
#include "ll/api/event/world/ServerLevelTickEvent.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/utils/RandomUtils.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/deps/nbt/CompoundTagVariant.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/legacy/ActorRuntimeID.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/effect/MobEffectInstance.h"
#include "mc/world/gamemode/InteractionResult.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/PotionItem.h"
#include "mc/world/item/components/ComponentItem.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"
#include "mod/MyMod.h"
#include "mod/ThirstConfig.h"
#include "mod/ThirstPacket.h"

#ifdef LL_PLAT_C
#include "ThirstHud.h"
#endif

namespace thirsty {
namespace {

struct ThirstState {
    int value       = MaxThirst;
    int drainTicks  = 0;
    int damageTicks = 0;
    int syncTicks   = 0;
};

std::unordered_map<Player const*, ThirstState> states;
ll::event::ListenerPtr tickListener;
ll::event::ListenerPtr respawnListener;

ThirstConfig const& config() { return my_mod::MyMod::getInstance().getConfig(); }

bool usesThirst(Player const& player) {
    auto mode = player.getPlayerGameType();
    return mode == GameType::Survival || mode == GameType::Adventure;
}

void sync(Player const& player, ThirstState& state, bool hide = false) {
    ThirstPacket packet;
    packet.actorId = player.getRuntimeID();
    packet.value   = static_cast<uchar>(state.value);
    packet.visible = !hide && usesThirst(player) && player.isAlive();
#ifdef LL_PLAT_C
    // A hosted world's local HUD also receives the authoritative server value.
    updateThirstHud(packet);
#endif
    packet.sendTo(player);
    state.syncTicks = 20;
}

void tickPlayer(Player& player) {
    if (player.isClientSide()) return;
    auto& state = states[&player];
    auto  previous = state.value;
    if (usesThirst(player) && player.isAlive()) {
        if (state.value > 0) {
            state.drainTicks += player.isSprinting() ? config().sprintMultiplier : 1;
            auto interval = config().secondsPerPoint * 20;
            if (state.drainTicks >= interval) {
                state.value = std::max(0, state.value - state.drainTicks / interval);
                state.drainTicks %= interval;
            }
        }
        if (state.value == 0 && config().damageWhenEmpty) {
            if (++state.damageTicks >= config().damageIntervalSeconds * 20) {
                state.damageTicks = 0;
                player.hurtByCause(config().damageAmount, SharedTypes::Legacy::ActorDamageCause::Starve);
            }
        } else {
            state.damageTicks = 0;
        }
    } else {
        state.damageTicks = 0;
    }
    if (--state.syncTicks <= 0 || previous != state.value) sync(player, state);
}

void drinkWater(Player& player, bool dirty) {
    if (player.isClientSide() || !player.isAlive() || !usesThirst(player)) return;
    auto& state = states[&player];
    state.value = std::min(MaxThirst, state.value + config().waterBottleRestore);
    if (state.value > 0) state.damageTicks = 0;
    if (dirty && ll::random_utils::rand<int>(100) < config().dirtyWaterDebuffChance) {
        EffectDuration duration;
        duration.mValue = config().dirtyWaterDebuffSeconds * 20;
        player.addEffect(MobEffectInstance(static_cast<uint>(MobEffectIds::Confusion), duration));
    }
    sync(player, state);
}

LL_TYPE_INSTANCE_HOOK(
    DrinkWaterHook,
    ll::memory::HookPriority::Normal,
    PotionItem,
    &PotionItem::$useTimeDepleted,
    ItemUseMethod,
    ItemStack& item,
    Level* level,
    Player* player
) {
    // The original call replaces the potion with an empty bottle.
    bool water = player && !player->isClientSide() && player->isAlive() && usesThirst(*player)
              && item.getTypeName() == "minecraft:potion" && item.getAuxValue() == 0;
    auto result = origin(item, level, player);
    if (water && result == ItemUseMethod::Consume) {
        drinkWater(*player, true);
    }
    return result;
}

LL_TYPE_INSTANCE_HOOK(
    DrinkCustomWaterHook,
    ll::memory::HookPriority::Normal,
    ComponentItem,
    &ComponentItem::$useTimeDepleted,
    ItemUseMethod,
    ItemStack& item,
    Level* level,
    Player* player
) {
    // Food conversion replaces this stack with the bottle or the next flask state.
    auto name = item.getTypeName();
    bool dirtyFlask = name == "thirsty:dirty_flask_1" || name == "thirsty:dirty_flask_2"
                   || name == "thirsty:dirty_flask_3" || name == "thirsty:dirty_flask_4";
    bool cleanWater = name == "thirsty:purified_water_bottle" || name == "thirsty:clean_flask_1"
                   || name == "thirsty:clean_flask_2" || name == "thirsty:clean_flask_3"
                   || name == "thirsty:clean_flask_4";
    auto result = origin(item, level, player);
    if (player && (dirtyFlask || cleanWater)
        && (result == ItemUseMethod::Eat || result == ItemUseMethod::Consume)) {
        drinkWater(*player, dirtyFlask);
    }
    return result;
}

LL_TYPE_INSTANCE_HOOK(
    FillFlaskHook,
    ll::memory::HookPriority::Normal,
    ComponentItem,
    &ComponentItem::$_useOn,
    InteractionResult,
    ItemStack& item,
    Actor& actor,
    BlockPos pos,
    uchar face,
    HandSlot handSlot,
    Vec3 const& clickPos
) {
    if (item.getTypeName() != "thirsty:flask") return origin(item, actor, pos, face, handSlot, clickPos);

    auto& region = actor.getDimensionBlockSource();
    auto const& liquid = region.getLiquidBlock(pos).getTypeName();
    if (liquid != "minecraft:water" && liquid != "minecraft:flowing_water") {
        return origin(item, actor, pos, face, handSlot, clickPos);
    }

    // Predict the same stack change on the client; the normal use-on transaction
    // applies and synchronizes it on the server, including the integrated server.
    auto userData = std::move(item.mUserData);
    item.reinit("thirsty:dirty_flask_4", 1, 0);
    item.mUserData = std::move(userData);
    if (!actor.isClientSide()) {
        actor.playSound(region, SharedTypes::Legacy::LevelSoundEvent::BottleFill, pos.center(), -1);
    }
    return {true, true};
}

LL_TYPE_INSTANCE_HOOK(
    LoadThirstHook,
    ll::memory::HookPriority::Normal,
    Player,
    &Player::$readAdditionalSaveData,
    void,
    CompoundTag const& tag,
    DataLoadHelper& helper
) {
    origin(tag, helper);
    if (isClientSide()) return;
    auto& state = states[this];
    state = {};
    if (tag.contains("thirsty:value", Tag::Int)) {
        state.value = std::clamp(tag["thirsty:value"].get<IntTag>().data, 0, MaxThirst);
    }
    if (tag.contains("thirsty:drain_ticks", Tag::Int)) {
        state.drainTicks = std::clamp(
            tag["thirsty:drain_ticks"].get<IntTag>().data, 0, config().secondsPerPoint * 20 - 1
        );
    }
    if (tag.contains("thirsty:damage_ticks", Tag::Int)) {
        state.damageTicks = std::clamp(
            tag["thirsty:damage_ticks"].get<IntTag>().data, 0, config().damageIntervalSeconds * 20 - 1
        );
    }
}

LL_TYPE_INSTANCE_HOOK(
    SaveThirstHook,
    ll::memory::HookPriority::Normal,
    Player,
    &Player::$addAdditionalSaveData,
    void,
    CompoundTag& tag
) {
    origin(tag);
    if (isClientSide()) return;
    if (auto it = states.find(this); it != states.end()) {
        tag["thirsty:value"]        = IntTag(it->second.value);
        tag["thirsty:drain_ticks"]  = IntTag(it->second.drainTicks);
        tag["thirsty:damage_ticks"] = IntTag(it->second.damageTicks);
    }
}

LL_TYPE_INSTANCE_HOOK(
    RemovePlayerHook,
    ll::memory::HookPriority::Normal,
    Player,
    &Player::$dtor,
    void
) {
#ifdef LL_PLAT_C
    forgetThirstHud(getRuntimeID());
#endif
    if (!isClientSide()) states.erase(this);
    origin();
}

using ThirstHooks = ll::memory::HookRegistrar<
    DrinkWaterHook, DrinkCustomWaterHook, FillFlaskHook, LoadThirstHook, SaveThirstHook, RemovePlayerHook
>;

} // namespace

void enableThirstSystem() {
    ThirstHooks::hook();
    auto& bus = ll::event::EventBus::getInstance();
    tickListener = bus.emplaceListener<ll::event::ServerLevelTickEvent>([](auto& event) {
        event.level().forEachPlayer([](Player& player) {
            tickPlayer(player);
            return true;
        });
    });
    respawnListener = bus.emplaceListener<ll::event::PlayerRespawnEvent>([](auto& event) {
        auto& player = event.self();
        if (player.isClientSide()) return;
        auto& state = states[&player];
        state = {};
        sync(player, state);
    });
}

void disableThirstSystem() {
    auto& bus = ll::event::EventBus::getInstance();
    bus.removeListener(tickListener);
    bus.removeListener(respawnListener);
    tickListener.reset();
    respawnListener.reset();
    if (auto level = ll::service::getLevel()) {
        level->forEachPlayer([](Player& player) {
            if (auto it = states.find(&player); it != states.end()) sync(player, it->second, true);
            return true;
        });
    }
    ThirstHooks::unhook();
    states.clear();
}

} // namespace thirsty

#include "mod/ThirstSystem.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <unordered_map>

#include <fmt/format.h>

#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerRespawnEvent.h"
#include "ll/api/event/player/PlayerJumpEvent.h"
#include "ll/api/event/world/ServerLevelTickEvent.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/utils/RandomUtils.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/deps/nbt/CompoundTagVariant.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/legacy/ActorRuntimeID.h"
#include "mc/network/ServerNetworkHandler.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/actor/ActorHurtResult.h"
#include "mc/server/ServerPlayer.h"
#include "mc/world/effect/MobEffectInstance.h"
#include "mc/world/gamemode/InteractionResult.h"
#include "mc/world/gamemode/SurvivalMode.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/BucketItem.h"
#include "mc/world/item/BottleItem.h"
#include "mc/world/item/FoodItemComponentLegacy.h"
#include "mc/world/item/PotionItem.h"
#include "mc/world/item/components/ComponentItem.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Weather.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/biome/Biome.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/CauldronBlock.h"
#include "mc/world/level/block/VanillaStates.h"
#include "mc/world/level/block/actor/CauldronBlockActor.h"
#include "mc/world/level/block/block_events/BlockPlayerInteractEvent.h"
#include "mc/world/phys/HitResult.h"
#include "mod/MyMod.h"
#include "mod/ThirstConfig.h"
#include "mod/ThirstPacket.h"
#include "mod/WaterContainer.h"

#ifdef LL_PLAT_C
#include "ThirstHud.h"
#endif

namespace thirsty {
namespace {

struct ThirstState {
    int value       = MaxThirstUnits;
    int drainTicks  = 0;
    int pendingDrain = 0;
    int damageTicks = 0;
    int syncTicks   = 0;
    int handDrinkTicks = 0;
    int nauseaTicks = 0;
    bool ownsFatigue = false;
};

std::unordered_map<Player const*, ThirstState> states;
ll::event::ListenerPtr tickListener;
ll::event::ListenerPtr respawnListener;
ll::event::ListenerPtr jumpListener;

ThirstConfig const& config() { return my_mod::MyMod::getInstance().getConfig(); }

int thirstUnits(double points) { return static_cast<int>(std::lround(points * ThirstScale)); }

int baseDrainUnits(Player& player) {
    auto const& biome = player.getDimensionBlockSource().getBiome(BlockPos(player.getPosition()));
    auto name = biome.mHash->getString();
    auto const& costs = config().biomeBaseDrainPer4Seconds;
    auto it = costs.find(name);
    if (it == costs.end() && name.find(':') == std::string::npos) it = costs.find("minecraft:" + name);
    if (it == costs.end() && name.starts_with("minecraft:")) it = costs.find(name.substr(10));
    return thirstUnits(it == costs.end() ? config().baseDrainPer4Seconds : it->second);
}

std::array<uchar, 4> waterRestorePoints() {
    return {
        static_cast<uchar>(config().dirtyWaterRestore),
        static_cast<uchar>(config().slightlyDirtyWaterRestore),
        static_cast<uchar>(config().acceptableWaterRestore),
        static_cast<uchar>(config().waterBottleRestore)
    };
}

std::array<uchar, 4> waterDebuffChances() {
    return {
        static_cast<uchar>(config().dirtyWaterDebuffChance),
        static_cast<uchar>(config().slightlyDirtyWaterDebuffChance),
        static_cast<uchar>(config().acceptableWaterDebuffChance), 0
    };
}

bool isWater(Block const& block) {
    auto const& name = block.getTypeName();
    return name == "minecraft:water" || name == "minecraft:flowing_water";
}

WaterQuality sourceWaterQuality(BlockSource& region, BlockPos const& pos) {
    return pos.y > 100 || (pos.y < 48 && !region.canSeeSky(pos))
        ? WaterQuality::SlightlyDirty : WaterQuality::Dirty;
}

std::optional<BlockPos> waterTarget(Player& player) {
    auto& region = player.getDimensionBlockSource();
    auto eye = player.getEyePos();
    auto hit = region.clip(
        eye, eye + player.getViewVector() * 5.0f, true, ShapeType::Outline, 5, false, false, &player,
        [](BlockSource const&, Block const&, bool) { return true; }, true
    );
    if (hit.mIsHitLiquid && isWater(region.getLiquidBlock(hit.mLiquid))) return hit.mLiquid;
    return std::nullopt;
}

bool usesThirst(Player const& player) {
    auto mode = player.getPlayerGameType();
    return mode == GameType::Survival || mode == GameType::Adventure;
}

void updateLowThirstEffect(Player& player, ThirstState& state, bool enabled = true) {
    auto effectId = static_cast<uint>(MobEffectIds::DigSlowdown);
    auto const* current = player.getEffect(effectId);
    bool ours = state.ownsFatigue && current && current->mAmplifier == config().lowThirstFatigueAmplifier
             && current->mDuration->mValue > 0 && current->mDuration->mValue <= 40;
    bool low = enabled && usesThirst(player) && player.isAlive()
            && state.value < thirstUnits(config().lowThirstThreshold);
    if (!low) {
        if (ours) player.removeEffect(static_cast<int>(effectId));
        state.ownsFatigue = false;
        return;
    }
    // Leave fatigue from other sources, including stronger effects, under vanilla control.
    if (current && !ours) {
        state.ownsFatigue = false;
        return;
    }
    if (!current || current->mDuration->mValue <= 20) {
        EffectDuration duration;
        duration.mValue = 40;
        MobEffectInstance fatigue(effectId, duration);
        fatigue.mAmplifier = config().lowThirstFatigueAmplifier;
        player.addEffect(fatigue);
        state.ownsFatigue = true;
    }
}

void sync(Player const& player, ThirstState& state, bool hide = false) {
    ThirstPacket packet;
    packet.actorId = player.getRuntimeID();
    packet.value   = static_cast<uint>(state.value);
    packet.visible = !hide && usesThirst(player) && player.isAlive();
    packet.restores = waterRestorePoints();
    packet.debuffChances = waterDebuffChances();
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
    if (state.handDrinkTicks > 0) --state.handDrinkTicks;
    auto  previous = state.value;
    if (usesThirst(player) && player.isAlive()) {
        if (state.value > 0) {
            // Accumulate time spent in each biome/activity; charge once per four seconds.
            state.pendingDrain += baseDrainUnits(player);
            if (player.isSprinting() || player.isSwimming() || player.isGliding()) {
                state.pendingDrain += thirstUnits(config().activityDrainPer4Seconds);
            }
            if (++state.drainTicks >= DrainPeriodTicks) {
                state.value = std::max(0, state.value - state.pendingDrain / DrainPeriodTicks);
                state.pendingDrain %= DrainPeriodTicks;
                state.drainTicks = 0;
            }
        } else {
            state.drainTicks = 0;
            state.pendingDrain = 0;
        }
        if (state.value > 0 && config().nauseaSecondsPerPoint > 0
            && player.getEffect(static_cast<uint>(MobEffectIds::Confusion))) {
            if (++state.nauseaTicks >= config().nauseaSecondsPerPoint * 20) {
                state.nauseaTicks = 0;
                state.value = std::max(0, state.value - ThirstScale);
            }
        } else {
            state.nauseaTicks = 0;
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
        state.nauseaTicks = 0;
    }
    updateLowThirstEffect(player, state);
    if (--state.syncTicks <= 0 || previous != state.value) sync(player, state);
}

void changeThirstUnits(Player& player, int units) {
    if (player.isClientSide() || !player.isAlive() || !usesThirst(player)) return;
    auto& state = states[&player];
    state.value = std::clamp(state.value + units, 0, MaxThirstUnits);
    if (state.value > 0) state.damageTicks = 0;
    updateLowThirstEffect(player, state);
    sync(player, state);
}

void changeThirst(Player& player, int points) { changeThirstUnits(player, points * ThirstScale); }

void consumeActionThirst(Player& player, double points) {
    auto units = thirstUnits(points);
    if (units > 0) changeThirstUnits(player, -units);
}

void drinkWater(Player& player, WaterQuality quality, int restore, int debuffChance) {
    if (player.isClientSide() || !player.isAlive() || !usesThirst(player)) return;
    // Check before this drink can apply nausea: the first contaminated drink cannot poison itself.
    bool alreadyNauseated = static_cast<bool>(player.getEffect(static_cast<uint>(MobEffectIds::Confusion)));
    changeThirst(player, restore);
    bool poisonRisk = quality == WaterQuality::Dirty
                   || (config().poisonFromAnyImpureWater && quality != WaterQuality::Pure);
    if (alreadyNauseated && poisonRisk && ll::random_utils::rand<int>(100) < config().dirtyWaterPoisonChance) {
        EffectDuration duration;
        duration.mValue = config().dirtyWaterPoisonSeconds * 20;
        MobEffectInstance poison(static_cast<uint>(MobEffectIds::Poison), duration);
        poison.mAmplifier = 0; // Poison I; never stack amplifiers on repeated drinks.
        player.addEffect(poison);
    }
    if (ll::random_utils::rand<int>(100) < debuffChance) {
        EffectDuration duration;
        duration.mValue = config().dirtyWaterDebuffSeconds * 20;
        player.addEffect(MobEffectInstance(static_cast<uint>(MobEffectIds::Confusion), duration));
    }
}

void drinkWater(Player& player, WaterQuality quality) {
    auto index = static_cast<std::size_t>(quality);
    drinkWater(player, quality, waterRestorePoints()[index], waterDebuffChances()[index]);
}

void restoreFromFood(Player& player, std::string const& name) {
    if (auto it = config().foodThirst.find(name); it != config().foodThirst.end()) {
        changeThirst(player, it->second);
    }
}

LL_TYPE_INSTANCE_HOOK(
    BreakBlockThirstHook,
    ll::memory::HookPriority::Normal,
    SurvivalMode,
    &SurvivalMode::$destroyBlock,
    bool,
    BlockPos const& pos,
    uchar face
) {
    Player& player = mPlayer;
    bool breakingBlock = !player.isClientSide() && usesThirst(player) && player.isAlive()
                      && !player.getDimensionBlockSource().getBlock(pos).isAir();
    bool result = origin(pos, face);
    if (result && breakingBlock) consumeActionThirst(player, config().breakBlockDrain);
    return result;
}

LL_TYPE_INSTANCE_HOOK(
    AttackThirstHook,
    ll::memory::HookPriority::Normal,
    Player,
    &Player::attack,
    ActorHurtResult,
    Actor& target,
    SharedTypes::Legacy::ActorDamageCause const& cause,
    Player::AttackParameters const& parameters
) {
    auto result = origin(target, cause, parameters);
    if (result) consumeActionThirst(*this, config().attackDrain);
    return result;
}

class HandDrinkHandler final : public ll::network::IPacketHandler {
    void handle(NetworkIdentifier const& netId, NetEventCallback&, ll::network::Packet const& packet) const override {
        auto handler = ll::service::getServerNetworkHandler();
        if (!handler) return;
        auto* player = handler->_getServerPlayer(netId, packet.mSenderSubId);
        if (!player || player->isClientSide() || !player->isAlive() || !usesThirst(*player)
            || !player->isSneaking() || !player->getSelectedItem().isNull()) return;
        auto& state = states[player];
        if (state.handDrinkTicks > 0 || state.value >= MaxThirstUnits) return;
        auto pos = waterTarget(*player);
        if (!pos) return;
        auto quality = sourceWaterQuality(player->getDimensionBlockSource(), *pos);
        auto chance = quality == WaterQuality::Dirty ? config().handDrinkDebuffChance
                      : waterDebuffChances()[static_cast<std::size_t>(quality)];
        state.handDrinkTicks = 20;
        drinkWater(*player, quality, config().handDrinkRestore, chance);
        player->playSound(
            player->getDimensionBlockSource(), SharedTypes::Legacy::LevelSoundEvent::Drink, player->getEyePos(), -1
        );
    }
};

HandDrinkHandler handDrinkHandler;

void fillContainer(ItemStack& item, Actor& actor, WaterContainer const& container, WaterQuality quality, BlockPos const& pos) {
    auto userData = std::move(item.mUserData);
    item.reinit(
        fmt::format("thirsty:{}_{}_{}", WaterQualityIds[static_cast<std::size_t>(quality)], container.kind, container.capacity),
        1, 0
    );
    item.mUserData = std::move(userData);
    if (!actor.isClientSide()) {
        actor.playSound(actor.getDimensionBlockSource(), SharedTypes::Legacy::LevelSoundEvent::BottleFill, pos.center(), -1);
    }
}

bool isRainingOn(Actor& actor) {
    auto& region = actor.getDimensionBlockSource();
    auto pos = BlockPos(actor.getEyePos());
    auto const& weather = actor.getDimension().mWeather;
    return weather && region.canSeeSky(pos) && weather->isRainingAt(region, pos);
}

bool fillFromRain(ItemStack& item, Actor& actor, WaterContainer const& container) {
    if (!isRainingOn(actor)) return false;
    auto pos = BlockPos(actor.getEyePos());
    fillContainer(item, actor, container, WaterQuality::SlightlyDirty, pos);
    return true;
}

std::optional<WaterQuality> takeCauldronWater(Actor& actor, BlockPos const& pos) {
    auto& region = actor.getDimensionBlockSource();
    auto const& block = region.getBlock(pos);
    if (block.getTypeName() != "minecraft:cauldron") return std::nullopt;
    auto fill = block.getState<int>(VanillaStates::FillLevel()).value_or(0);
    auto liquid = block.getState<CauldronLiquidType>(VanillaStates::CauldronLiquid());
    if (fill <= 0 || liquid != CauldronLiquidType::Water) return std::nullopt;
    auto* blockActor = region.getBlockEntity(pos);
    if (!blockActor || blockActor->getType() != BlockActorType::Cauldron) return std::nullopt;
    auto& cauldron = static_cast<CauldronBlockActor&>(*blockActor);
    if (cauldron.mPotionId > 0 || cauldron.mHasCustomColor) return std::nullopt;

    auto const& heat = region.getBlock(BlockPos{pos.x, pos.y - 1, pos.z});
    auto const& name = heat.getTypeName();
    bool clean = name == "minecraft:fire" || name == "minecraft:soul_fire"
              || name == "minecraft:lava" || name == "minecraft:flowing_lava"
              || ((name == "minecraft:campfire" || name == "minecraft:soul_campfire")
                  && !heat.getState<bool>(VanillaStates::Extinguished()).value_or(false));
    // Bedrock uses 0..6; one bottle-sized layer is two fill-level units.
    static_cast<CauldronBlock const&>(block.getBlockType())
        .setLiquidLevel(region, pos, std::max(0, fill - 2), CauldronLiquidType::Water);
    return clean ? WaterQuality::Acceptable : WaterQuality::Dirty;
}

bool fillFromCauldron(ItemStack& item, Actor& actor, WaterContainer const& container, BlockPos const& pos) {
    auto quality = takeCauldronWater(actor, pos);
    if (!quality) return false;
    fillContainer(item, actor, container, *quality, pos);
    return true;
}

void fillWaterBottle(BottleItem const& bottle, ItemStack& item, Actor& actor, WaterQuality quality, BlockPos const& pos) {
    // Let vanilla handle stacked glass bottles, creative mode and a full inventory.
    bottle._createBottledItem(actor, item, WaterBottleIds[static_cast<std::size_t>(quality)]);
    if (!actor.isClientSide()) {
        actor.playSound(actor.getDimensionBlockSource(), SharedTypes::Legacy::LevelSoundEvent::BottleFill, pos.center(), -1);
    }
}

LL_TYPE_INSTANCE_HOOK(
    CreateSourceWaterBottleHook,
    ll::memory::HookPriority::Normal,
    BottleItem,
    &BottleItem::_createBottledItem,
    void,
    Actor& actor,
    ItemStack& item,
    std::string_view const itemToCreate
) {
    // Cover vanilla fill fallbacks as well as the explicit water-surface path.
    origin(actor, item, itemToCreate == "minecraft:potion" ? WaterBottleIds[0] : itemToCreate);
}

LL_TYPE_INSTANCE_HOOK(
    DispenseSourceWaterBottleHook,
    ll::memory::HookPriority::Normal,
    BottleItem,
    &BottleItem::_fillBottleViaDispenser,
    void,
    BlockSource& region,
    std::string_view const bottledItem,
    Container& container,
    int slot,
    Vec3 const& pos,
    uchar face
) {
    origin(region, bottledItem == "minecraft:potion" ? WaterBottleIds[0] : bottledItem, container, slot, pos, face);
}

LL_TYPE_INSTANCE_HOOK(
    FillWaterBottleHook,
    ll::memory::HookPriority::Normal,
    BottleItem,
    &BottleItem::$_useOn,
    InteractionResult,
    ItemStack& item,
    Actor& actor,
    BlockPos pos,
    uchar face,
    HandSlot handSlot,
    Vec3 const& clickPos
) {
    if (item.getTypeName() != "minecraft:glass_bottle") return origin(item, actor, pos, face, handSlot, clickPos);
    if (auto quality = takeCauldronWater(actor, pos)) {
        fillWaterBottle(*this, item, actor, *quality, pos);
        return {true, true};
    }
    auto& region = actor.getDimensionBlockSource();
    if (isWater(region.getLiquidBlock(pos))) {
        fillWaterBottle(*this, item, actor, WaterQuality::Dirty, pos);
        return {true, true};
    }
    return origin(item, actor, pos, face, handSlot, clickPos);
}

LL_TYPE_INSTANCE_HOOK(
    FillWaterBottleInAirHook,
    ll::memory::HookPriority::Normal,
    Item,
    &Item::$use,
    ItemStack&,
    ItemStack& item,
    Player& player,
    HandSlot handSlot
) {
    if (item.getTypeName() == "minecraft:glass_bottle") {
        auto const& bottle = static_cast<BottleItem const&>(static_cast<Item const&>(*this));
        if (auto pos = waterTarget(player)) {
            fillWaterBottle(bottle, item, player, WaterQuality::Dirty, *pos);
            return item;
        }
        if (isRainingOn(player)) {
            fillWaterBottle(bottle, item, player, WaterQuality::SlightlyDirty, BlockPos(player.getEyePos()));
            return item;
        }
    }
    return origin(item, player, handSlot);
}

LL_TYPE_INSTANCE_HOOK(
    FillCauldronWaterBottleHook,
    ll::memory::HookPriority::Normal,
    CauldronBlock,
    &CauldronBlock::use,
    void,
    BlockEvents::BlockPlayerInteractEvent& event
) {
    Player& player = event.mPlayer;
    auto item = player.getItemInHandSlot(event.mHandSlot).clone();
    if (item.getTypeName() != "minecraft:glass_bottle") return origin(event);
    auto quality = takeCauldronWater(player, event.mPos);
    if (!quality) return origin(event);
    auto const& bottle = static_cast<BottleItem const&>(*item.mItem);
    fillWaterBottle(bottle, item, player, *quality, event.mPos);
    player._setHandContainerItem(item, event.mHandSlot);
    event.mSuccessful = true;
}

LL_TYPE_INSTANCE_HOOK(
    EatFoodHook,
    ll::memory::HookPriority::Normal,
    FoodItemComponentLegacy,
    &FoodItemComponentLegacy::$useTimeDepleted,
    Item const*,
    ItemUseMethod& method,
    ItemStack const& originalItem,
    ItemStack& item,
    Player& player,
    Level& level
) {
    auto name = item.getTypeName();
    auto result = origin(method, originalItem, item, player, level);
    if (method == ItemUseMethod::Eat || method == ItemUseMethod::Consume) restoreFromFood(player, name);
    return result;
}

LL_TYPE_INSTANCE_HOOK(
    DrinkMilkHook,
    ll::memory::HookPriority::Normal,
    BucketItem,
    &BucketItem::$useTimeDepleted,
    ItemUseMethod,
    ItemStack& item,
    Level* level,
    Player* player
) {
    auto name = item.getTypeName();
    auto result = origin(item, level, player);
    if (player && name == "minecraft:milk_bucket" && result == ItemUseMethod::Consume) restoreFromFood(*player, name);
    return result;
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
        drinkWater(*player, WaterQuality::Pure);
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
    // Food conversion replaces this stack with the bottle or the next container state.
    auto name = item.getTypeName();
    auto container = parseWaterContainer(name);
    auto quality = container && container->sips > 0 ? std::optional{container->quality}
                                                   : parseWaterBottle(name, item.getAuxValue());
    auto result    = origin(item, level, player);
    if (player && quality && (result == ItemUseMethod::Eat || result == ItemUseMethod::Consume)) {
        drinkWater(*player, *quality);
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
    auto container = parseWaterContainer(item.getTypeName());
    if (!container || container->sips > 0) return origin(item, actor, pos, face, handSlot, clickPos);

    auto& region = actor.getDimensionBlockSource();
    if (fillFromCauldron(item, actor, *container, pos)) return {true, true};
    if (isWater(region.getLiquidBlock(pos))) {
        fillContainer(item, actor, *container, sourceWaterQuality(region, pos), pos);
        return {true, true};
    }
    if (fillFromRain(item, actor, *container)) return {true, true};
    return origin(item, actor, pos, face, handSlot, clickPos);
}

LL_TYPE_INSTANCE_HOOK(
    FillContainerInAirHook,
    ll::memory::HookPriority::Normal,
    ComponentItem,
    &ComponentItem::$use,
    ItemStack&,
    ItemStack& item,
    Player& player,
    HandSlot handSlot
) {
    auto container = parseWaterContainer(item.getTypeName());
    if (container && container->sips == 0) {
        if (auto pos = waterTarget(player)) {
            fillContainer(item, player, *container, sourceWaterQuality(player.getDimensionBlockSource(), *pos), *pos);
            return item;
        }
        if (fillFromRain(item, player, *container)) return item;
    }
    return origin(item, player, handSlot);
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
    if (tag.contains("thirsty:value", Tag::Double)) {
        state.value = std::clamp(thirstUnits(tag["thirsty:value"].get<DoubleTag>().data), 0, MaxThirstUnits);
    } else if (tag.contains("thirsty:value", Tag::Int)) {
        state.value = std::clamp(tag["thirsty:value"].get<IntTag>().data, 0, MaxThirst) * ThirstScale;
    }
    if (tag.contains("thirsty:pending_drain", Tag::Int)) {
        state.pendingDrain = std::clamp(
            tag["thirsty:pending_drain"].get<IntTag>().data, 0, MaxThirstUnits * DrainPeriodTicks * 2
        );
    }
    if (tag.contains("thirsty:drain_ticks", Tag::Int) && tag.contains("thirsty:pending_drain", Tag::Int)) {
        state.drainTicks = std::clamp(
            tag["thirsty:drain_ticks"].get<IntTag>().data, 0, DrainPeriodTicks - 1
        );
    }
    if (tag.contains("thirsty:damage_ticks", Tag::Int)) {
        state.damageTicks = std::clamp(
            tag["thirsty:damage_ticks"].get<IntTag>().data, 0, config().damageIntervalSeconds * 20 - 1
        );
    }
    if (tag.contains("thirsty:nausea_ticks", Tag::Int) && config().nauseaSecondsPerPoint > 0) {
        state.nauseaTicks = std::clamp(
            tag["thirsty:nausea_ticks"].get<IntTag>().data, 0, config().nauseaSecondsPerPoint * 20 - 1
        );
    }
    if (tag.contains("thirsty:fatigue_applied", Tag::Int)) {
        state.ownsFatigue = tag["thirsty:fatigue_applied"].get<IntTag>().data != 0;
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
        tag["thirsty:value"]        = DoubleTag(static_cast<double>(it->second.value) / ThirstScale);
        tag["thirsty:drain_ticks"]  = IntTag(it->second.drainTicks);
        tag["thirsty:pending_drain"] = IntTag(it->second.pendingDrain);
        tag["thirsty:damage_ticks"] = IntTag(it->second.damageTicks);
        tag["thirsty:nausea_ticks"] = IntTag(it->second.nauseaTicks);
        tag["thirsty:fatigue_applied"] = IntTag(it->second.ownsFatigue ? 1 : 0);
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
    DrinkWaterHook, DrinkCustomWaterHook, EatFoodHook, DrinkMilkHook, FillFlaskHook, FillContainerInAirHook,
    FillWaterBottleHook, FillWaterBottleInAirHook, FillCauldronWaterBottleHook,
    CreateSourceWaterBottleHook, DispenseSourceWaterBottleHook,
    BreakBlockThirstHook, AttackThirstHook,
    LoadThirstHook, SaveThirstHook, RemovePlayerHook
>;

} // namespace

bool isLookingAtWater(Player& player) { return waterTarget(player).has_value(); }

void enableThirstSystem() {
    ThirstHooks::hook();
    HandDrinkPacket packet;
    ll::network::PacketRegistrar::getInstance().registerHandler(packet.getName(), packet.getRuntimeId(), handDrinkHandler);
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
        updateLowThirstEffect(player, state, false);
        state = {};
        sync(player, state);
    });
    jumpListener = bus.emplaceListener<ll::event::PlayerJumpEvent>([](auto& event) {
        if (!event.isCancelled()) consumeActionThirst(event.self(), config().jumpDrain);
    }, ll::event::EventPriority::Lowest);
}

void disableThirstSystem() {
    HandDrinkPacket packet;
    ll::network::PacketRegistrar::getInstance().registerHandler(packet.getName(), packet.getRuntimeId(), std::nullopt);
    auto& bus = ll::event::EventBus::getInstance();
    bus.removeListener(tickListener);
    bus.removeListener(respawnListener);
    bus.removeListener(jumpListener);
    tickListener.reset();
    respawnListener.reset();
    jumpListener.reset();
    if (auto level = ll::service::getLevel()) {
        level->forEachPlayer([](Player& player) {
            if (auto it = states.find(&player); it != states.end()) {
                updateLowThirstEffect(player, it->second, false);
                sync(player, it->second, true);
            }
            return true;
        });
    }
    ThirstHooks::unhook();
    states.clear();
}

} // namespace thirsty

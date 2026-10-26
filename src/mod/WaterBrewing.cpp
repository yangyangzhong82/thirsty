#include "mod/WaterBrewing.h"

#include "ll/api/memory/Hook.h"
#include "mc/world/inventory/simulation/validation/BrewingStandResultContainerValidation.h"
#include "mc/world/item/ItemDescriptor.h"
#include "mc/world/item/ItemInstance.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/alchemy/PotionBrewing.h"
#include "mc/world/level/block/actor/BrewingStandBlockActor.h"
#include "mod/WaterContainer.h"

namespace thirsty {
namespace {

bool isImpureWater(ItemStackBase const& item) {
    auto quality = parseWaterBottle(item.getTypeName(), item.getAuxValue());
    return quality && *quality != WaterQuality::Pure;
}

LL_TYPE_STATIC_HOOK(
    BrewingWaterInputHook,
    ll::memory::HookPriority::Normal,
    PotionBrewing,
    &PotionBrewing::isPotionRecipeInput,
    bool,
    ItemDescriptor const& item
) {
    auto quality = parseWaterBottle(item.getRawNameId(), item.getAuxValue());
    if (quality && *quality != WaterQuality::Pure) return false;
    return origin(item);
}

LL_TYPE_STATIC_HOOK(
    BrewingWaterHasMixHook,
    ll::memory::HookPriority::Normal,
    PotionBrewing,
    &PotionBrewing::hasMix,
    bool,
    ItemInstance const& source,
    ItemDescriptor const& ingredient
) {
    if (isImpureWater(source)) return false;
    return origin(source, ingredient);
}

LL_TYPE_STATIC_HOOK(
    BrewingWaterMixHook,
    ll::memory::HookPriority::Normal,
    PotionBrewing,
    &PotionBrewing::mix,
    ItemInstance,
    ItemDescriptor const& ingredient,
    ItemInstance const& source
) {
    if (isImpureWater(source)) return ItemInstance(source);
    return origin(ingredient, source);
}

LL_TYPE_INSTANCE_HOOK(
    BrewingWaterSlotHook,
    ll::memory::HookPriority::Normal,
    BrewingStandResultContainerValidation,
    &BrewingStandResultContainerValidation::$isItemAllowedInSlot,
    bool,
    ContainerScreenContext const& context,
    int const slot,
    ItemStackBase const& item,
    int const count,
    bool check
) {
    if (isImpureWater(item)) return false;
    return origin(context, slot, item, count, check);
}

LL_TYPE_INSTANCE_HOOK(
    BrewingWaterHopperHook,
    ll::memory::HookPriority::Normal,
    BrewingStandBlockActor,
    &BrewingStandBlockActor::$canPushInItem,
    bool,
    int slot,
    int face,
    ItemStack const& item
) {
    if (slot < BrewingStandBlockActor::Potion1 || slot > BrewingStandBlockActor::Potion3) {
        return origin(slot, face, item);
    }
    if (isImpureWater(item)) return false;
    return origin(slot, face, item);
}

using WaterBrewingHooks = ll::memory::HookRegistrar<
    BrewingWaterInputHook, BrewingWaterHasMixHook, BrewingWaterMixHook, BrewingWaterSlotHook, BrewingWaterHopperHook
>;

} // namespace

void enableWaterBrewing() { WaterBrewingHooks::hook(); }
void disableWaterBrewing() { WaterBrewingHooks::unhook(); }

} // namespace thirsty

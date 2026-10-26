#include "ThirstHud.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <mutex>
#include <optional>
#include <unordered_map>

#include "ll/api/memory/Hook.h"
#include "ll/api/network/packet/PacketRegistrar.h"
#include "ll/api/service/Bedrock.h"
#include "mc/client/game/ClientInstance.h"
#include "mc/client/game/IClientInstance.h"
#include "mc/client/gui/controls/UIControl.h"
#include "mc/client/gui/controls/renderers/HudHungerRenderer.h"
#include "mc/client/player/LocalPlayer.h"
#include "mc/client/renderer/screen/MinecraftUIRenderContext.h"
#include "mc/legacy/ActorRuntimeID.h"
#include "mc/locale/I18n.h"
#include "mc/safety/RedactableString.h"
#include "mc/world/effect/MobEffectIds.h"
#include "mc/world/item/ItemStackBase.h"
#include "mc/world/item/PotionItem.h"
#include "mc/world/item/components/ComponentItem.h"
#include "mod/MyMod.h"
#include "mod/ThirstConfig.h"
#include "mod/ThirstPacket.h"
#include "mod/WaterContainer.h"
#include "WaterInput.h"

namespace thirsty {
namespace {

using Clock = std::chrono::steady_clock;

struct HudState {
    int               value;
    bool              visible;
    std::array<uchar, 4> restores;
    std::array<uchar, 4> debuffChances;
    Clock::time_point received;
    Clock::time_point dropped; // last time the value went down, for the flash
};

std::mutex hudMutex;
std::unordered_map<std::uint64_t, HudState> hudStates;

class ThirstHandler final : public ll::network::IPacketHandler {
    void handle(NetworkIdentifier const&, NetEventCallback&, ll::network::Packet const& packet) const override {
        updateThirstHud(static_cast<ThirstPacket const&>(packet));
    }
};

ThirstHandler packetHandler;

void appendWaterTooltip(ItemStackBase const& stack, Bedrock::Safety::RedactableString& text) {
    auto name = stack.getTypeName();
    auto container = parseWaterContainer(name);
    auto quality = container && container->sips > 0 ? std::optional{container->quality}
                                                   : parseWaterBottle(name, stack.getAuxValue());
    if (!container && !quality) return;
    auto client = ll::service::getClientInstance();
    auto* player = client ? client->getLocalPlayer() : nullptr;
    if (!player) return;
    std::optional<HudState> state;
    {
        std::lock_guard lock(hudMutex);
        if (auto it = hudStates.find(player->getRuntimeID()); it != hudStates.end()) state = it->second;
    }
    if (!state || Clock::now() - state->received > std::chrono::seconds(5)) return;
    auto& i18n = getI18n();
    auto locale = i18n.getCurrentLanguage();
    if (quality) {
        auto index = static_cast<std::size_t>(*quality);
        auto label = i18n.get("thirsty.quality." + std::string(WaterQualityIds[index]), locale);
        text += "\n§7" + i18n.get("thirsty.tooltip.quality", {label}, locale);
        text += "\n§7" + i18n.get("thirsty.tooltip.restore", {std::to_string(state->restores[index])}, locale);
        text += "\n§7" + i18n.get("thirsty.tooltip.debuff", {std::to_string(state->debuffChances[index])}, locale);
        if (!container) {
            text += "\n§7" + i18n.get(
                *quality == WaterQuality::Pure ? "thirsty.tooltip.brewing.pure" : "thirsty.tooltip.brewing.impure",
                locale
            );
        }
    } else {
        text += "\n§7" + i18n.get("thirsty.tooltip.empty", locale);
    }
}

LL_TYPE_INSTANCE_HOOK(
    WaterTooltipHook,
    ll::memory::HookPriority::Normal,
    ComponentItem,
    &ComponentItem::$appendFormattedHovertext,
    void,
    ItemStackBase const& stack,
    Level& level,
    Bedrock::Safety::RedactableString& text,
    bool const showCategory
) {
    origin(stack, level, text, showCategory);
    appendWaterTooltip(stack, text);
}

LL_TYPE_INSTANCE_HOOK(
    WaterBottleTooltipHook,
    ll::memory::HookPriority::Normal,
    PotionItem,
    &PotionItem::$appendFormattedHovertext,
    void,
    ItemStackBase const& stack,
    Level& level,
    Bedrock::Safety::RedactableString& text,
    bool const showCategory
) {
    origin(stack, level, text, showCategory);
    appendWaterTooltip(stack, text);
}

void fill(MinecraftUIRenderContext& context, float x, float y, float width, float height, mce::Color const& color,
          float alpha) {
    RectangleArea rect;
    rect._x0 = x;
    rect._x1 = x + width;
    rect._y0 = y;
    rect._y1 = y + height;
    context.fillRectangle(rect, color, alpha);
}

void drawDrop(MinecraftUIRenderContext& context, float x, float y, float points, mce::Color const& water, float alpha) {
    // Pixel art drawn directly through the game UI; no resource pack is needed.
    constexpr std::array<int, 9> widths{1, 3, 3, 5, 7, 7, 7, 5, 3};
    constexpr mce::Color outline{12, 30, 48};
    constexpr mce::Color empty{40, 62, 77};
    for (int row = 0; row < 9; ++row) {
        float width = static_cast<float>(widths[row]);
        float left  = x + (7.0f - width) / 2.0f;
        fill(context, left, y + row, width, 1.0f, outline, alpha);
        if (row > 1 && row < 8) {
            fill(context, left + 1.0f, y + row, width - 2.0f, 1.0f, empty, alpha);
            float filled = std::clamp(static_cast<float>(row + 1) - (8.0f - points * 3.0f), 0.0f, 1.0f);
            if (filled > 0.0f) {
                fill(context, left + 1.0f, y + row + 1.0f - filled, width - 2.0f, filled, water, alpha);
            }
        }
    }
    if (points >= 2.0f) fill(context, x + 2.0f, y + 3.0f, 1.0f, 2.0f, mce::Color{153, 232, 255}, alpha);
}

LL_TYPE_INSTANCE_HOOK(
    ThirstHudHook,
    ll::memory::HookPriority::Normal,
    HudHungerRenderer,
    &HudHungerRenderer::$render,
    void,
    MinecraftUIRenderContext& context,
    IClientInstance& client,
    UIControl& owner,
    int pass
) {
    origin(context, client, owner, pass);
    auto* player = client.getLocalPlayer();
    if (!player || !player->isAlive() || !mShouldRender || !owner.mIsVisibleInTree) return;
    auto mode = player->getPlayerGameType();
    if (mode != GameType::Survival && mode != GameType::Adventure) return;

    std::optional<HudState> state;
    {
        std::lock_guard lock(hudMutex);
        if (auto it = hudStates.find(player->getRuntimeID()); it != hudStates.end()) state = it->second;
    }
    if (!state || !state->visible || Clock::now() - state->received > std::chrono::seconds(5)) return;

    auto const& config = my_mod::MyMod::getInstance().getConfig();
    if (owner.mCachedPositionDirty) owner._setCachedPosition();
    auto position = *owner.mCachedPosition;
    float alpha   = owner.mAlpha;
    auto now      = Clock::now();
    auto water    = state->value <= 4 * ThirstScale ? mce::Color{244, 139, 69} : mce::Color{42, 174, 237};
    if (player->getEffect(static_cast<uint>(MobEffectIds::Confusion))) water = mce::Color{150, 186, 74};
    // Flash white briefly after losing a point, like the vanilla hunger bar.
    auto sinceDrop = std::chrono::duration_cast<std::chrono::milliseconds>(now - state->dropped).count();
    if (sinceDrop < 600 && (sinceDrop / 150) % 2 == 0) water = mce::Color{220, 245, 255};
    // Drops jitter when nearly empty; the phase is per-drop so they don't move in lockstep.
    auto ticks = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() / 50;
    context.saveCurrentClippingRectangle();
    context.setFullClippingRectangle();
    // The hunger control is anchored at its right edge; icons extend to the left.
    for (int i = 0; i < 10; ++i) {
        float jitter = state->value <= 4 * ThirstScale && (ticks + i * 7) % 11 == 0 ? -1.0f : 0.0f;
        drawDrop(
            context,
            position.x + config.hudOffsetX - static_cast<float>((i + 1) * 8),
            position.y + config.hudOffsetY + jitter,
            static_cast<float>(std::clamp(state->value - i * 2 * ThirstScale, 0, 2 * ThirstScale)) / ThirstScale,
            water,
            alpha
        );
    }
    context.restoreSavedClippingRectangle();
}

} // namespace

void updateThirstHud(ThirstPacket const& packet) {
    std::lock_guard lock(hudMutex);
    auto  now   = Clock::now();
    int   value = static_cast<int>(std::min<uint>(packet.value, MaxThirstUnits));
    auto  it    = hudStates.find(packet.actorId);
    auto  dropped = it != hudStates.end() && value < it->second.value ? now
                  : it != hudStates.end()                            ? it->second.dropped
                                                                     : Clock::time_point{};
    hudStates[packet.actorId] = {value, packet.visible, packet.restores, packet.debuffChances, now, dropped};
}

bool hasThirstHud(std::uint64_t actorId) {
    std::lock_guard lock(hudMutex);
    auto it = hudStates.find(actorId);
    return it != hudStates.end() && it->second.visible
        && Clock::now() - it->second.received <= std::chrono::seconds(5);
}

void forgetThirstHud(std::uint64_t actorId) {
    std::lock_guard lock(hudMutex);
    hudStates.erase(actorId);
}

void enableThirstHud() {
    ThirstPacket packet;
    ll::network::PacketRegistrar::getInstance().registerHandler(packet.getName(), packet.getRuntimeId(), packetHandler);
    ThirstHudHook::hook();
    WaterTooltipHook::hook();
    WaterBottleTooltipHook::hook();
    enableWaterInput();
}

void disableThirstHud() {
    disableWaterInput();
    WaterTooltipHook::unhook();
    WaterBottleTooltipHook::unhook();
    ThirstHudHook::unhook();
    ThirstPacket packet;
    ll::network::PacketRegistrar::getInstance().registerHandler(packet.getName(), packet.getRuntimeId(), std::nullopt);
    std::lock_guard lock(hudMutex);
    hudStates.clear();
}

} // namespace thirsty

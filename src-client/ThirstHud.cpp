#include "ThirstHud.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <mutex>
#include <optional>
#include <unordered_map>

#include "ll/api/memory/Hook.h"
#include "ll/api/network/packet/PacketRegistrar.h"
#include "mc/client/game/IClientInstance.h"
#include "mc/client/gui/controls/UIControl.h"
#include "mc/client/gui/controls/renderers/HudHungerRenderer.h"
#include "mc/client/player/LocalPlayer.h"
#include "mc/client/renderer/screen/MinecraftUIRenderContext.h"
#include "mc/legacy/ActorRuntimeID.h"
#include "mod/MyMod.h"
#include "mod/ThirstConfig.h"
#include "mod/ThirstPacket.h"

namespace thirsty {
namespace {

using Clock = std::chrono::steady_clock;

struct HudState {
    int               value;
    bool              visible;
    Clock::time_point received;
};

std::mutex hudMutex;
std::unordered_map<std::uint64_t, HudState> hudStates;

class ThirstHandler final : public ll::network::IPacketHandler {
    void handle(NetworkIdentifier const&, NetEventCallback&, ll::network::Packet const& packet) const override {
        updateThirstHud(static_cast<ThirstPacket const&>(packet));
    }
};

ThirstHandler packetHandler;

void fill(MinecraftUIRenderContext& context, float x, float y, float width, float height, mce::Color const& color,
          float alpha) {
    RectangleArea rect;
    rect._x0 = x;
    rect._x1 = x + width;
    rect._y0 = y;
    rect._y1 = y + height;
    context.fillRectangle(rect, color, alpha);
}

void drawDrop(MinecraftUIRenderContext& context, float x, float y, int points, mce::Color const& water, float alpha) {
    // Pixel art drawn directly through the game UI; no resource pack is needed.
    constexpr std::array<int, 9> widths{1, 3, 3, 5, 7, 7, 7, 5, 3};
    constexpr mce::Color outline{12, 30, 48};
    constexpr mce::Color empty{40, 62, 77};
    for (int row = 0; row < 9; ++row) {
        float width = static_cast<float>(widths[row]);
        float left  = x + (7.0f - width) / 2.0f;
        fill(context, left, y + row, width, 1.0f, outline, alpha);
        if (row > 1 && row < 8) {
            bool filled = points == 2 || (points == 1 && row >= 5);
            fill(context, left + 1.0f, y + row, width - 2.0f, 1.0f, filled ? water : empty, alpha);
        }
    }
    if (points == 2) fill(context, x + 2.0f, y + 3.0f, 1.0f, 2.0f, mce::Color{153, 232, 255}, alpha);
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
    auto water    = state->value <= 4 ? mce::Color{244, 139, 69} : mce::Color{42, 174, 237};
    context.saveCurrentClippingRectangle();
    context.setFullClippingRectangle();
    // The hunger control is anchored at its right edge; icons extend to the left.
    for (int i = 0; i < 10; ++i) {
        drawDrop(
            context,
            position.x + config.hudOffsetX - static_cast<float>((i + 1) * 8),
            position.y + config.hudOffsetY,
            std::clamp(state->value - i * 2, 0, 2),
            water,
            alpha
        );
    }
    context.restoreSavedClippingRectangle();
}

} // namespace

void updateThirstHud(ThirstPacket const& packet) {
    std::lock_guard lock(hudMutex);
    hudStates[packet.actorId] = {std::min<int>(packet.value, MaxThirst), packet.visible, Clock::now()};
}

void forgetThirstHud(std::uint64_t actorId) {
    std::lock_guard lock(hudMutex);
    hudStates.erase(actorId);
}

void enableThirstHud() {
    ThirstPacket packet;
    ll::network::PacketRegistrar::getInstance().registerHandler(packet.getName(), packet.getRuntimeId(), packetHandler);
    ThirstHudHook::hook();
}

void disableThirstHud() {
    ThirstHudHook::unhook();
    ThirstPacket packet;
    ll::network::PacketRegistrar::getInstance().registerHandler(packet.getName(), packet.getRuntimeId(), std::nullopt);
    std::lock_guard lock(hudMutex);
    hudStates.clear();
}

} // namespace thirsty

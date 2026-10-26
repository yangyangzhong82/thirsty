#include "WaterInput.h"

#include <unordered_set>

#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"
#include "mc/client/game/ClientInstance.h"
#include "mc/client/game/IClientInstance.h"
#include "mc/client/input/ClientInputHandler.h"
#include "mc/client/input/MinecraftInputHandler.h"
#include "mc/client/player/LocalPlayer.h"
#include "mc/deps/input/InputHandler.h"
#include "mc/legacy/ActorRuntimeID.h"
#include "mc/world/item/ItemStack.h"
#include "mod/ThirstPacket.h"
#include "mod/ThirstSystem.h"
#include "ThirstHud.h"

namespace thirsty {
namespace {

std::unordered_set<InputHandler*> inputs;

void requestHandDrink(FocusImpact, IClientInstance& client) {
    auto* player = client.getLocalPlayer();
    if (!player || !hasThirstHud(player->getRuntimeID()) || !player->isSneaking()
        || !player->getSelectedItem().isNull() || !isLookingAtWater(*player)) return;
    HandDrinkPacket packet;
    packet.mSenderSubId = player->getClientSubId();
    packet.sendToServer();
}

void installInput(InputHandler& input) {
    if (!inputs.insert(&input).second) return;
    // This is the game's mapped Use/Place action, so rebinding the key still works.
    input.registerButtonDownHandler("button.build_or_interact", requestHandDrink, true);
}

LL_TYPE_INSTANCE_HOOK(
    RegisterWaterInputHook,
    ll::memory::HookPriority::Normal,
    MinecraftInputHandler,
    &MinecraftInputHandler::$_registerInputHandlers,
    void
) {
    origin();
    installInput(*mInputHandler);
}

LL_TYPE_INSTANCE_HOOK(
    RemoveWaterInputHook,
    ll::memory::HookPriority::Normal,
    InputHandler,
    &InputHandler::$dtor,
    void
) {
    inputs.erase(this);
    origin();
}

using WaterInputHooks = ll::memory::HookRegistrar<RegisterWaterInputHook, RemoveWaterInputHook>;

} // namespace

void enableWaterInput() {
    WaterInputHooks::hook();
    if (auto client = ll::service::getClientInstance(); client && client->getInput()) {
        InputHandler& input = client->getInput()->mInputHandler;
        installInput(input);
    }
}

void disableWaterInput() {
    for (auto* input : inputs) {
        auto& handlers = *input->mButtonDownHandlerMap;
        for (auto it = handlers.begin(); it != handlers.end();) {
            auto target = it->second.second.target<decltype(&requestHandDrink)>();
            if (target && *target == &requestHandDrink) it = handlers.erase(it);
            else ++it;
        }
    }
    inputs.clear();
    WaterInputHooks::unhook();
}

} // namespace thirsty

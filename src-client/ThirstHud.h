#pragma once

#include <cstdint>

namespace thirsty {

class ThirstPacket;

void enableThirstHud();
void disableThirstHud();
void updateThirstHud(ThirstPacket const& packet);
void forgetThirstHud(std::uint64_t actorId);
bool hasThirstHud(std::uint64_t actorId);

} // namespace thirsty

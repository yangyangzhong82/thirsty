#pragma once

#include <array>

#include "mc/deps/core/utility/optional_ref.h"
#include "ll/api/network/packet/Packet.h"

namespace thirsty {

// Both builds register the same packet name and wire format.
class ThirstPacket final : public ll::network::PacketBase<ThirstPacket> {
public:
    uint64 actorId = 0;
    uint   value   = 2000; // Hundredths of a thirst point.
    bool   visible = false;
    std::array<uchar, 4> restores{3, 4, 5, 6};
    std::array<uchar, 4> debuffChances{60, 20, 1, 0};

    void                  write(BinaryStream& stream) const override;
    Bedrock::Result<void> read(ReadOnlyBinaryStream& stream) override;
};

// The sender is resolved from the connection; the server checks the water and cooldown.
class HandDrinkPacket final : public ll::network::PacketBase<HandDrinkPacket> {
public:
    void                  write(BinaryStream& stream) const override;
    Bedrock::Result<void> read(ReadOnlyBinaryStream& stream) override;
};

} // namespace thirsty

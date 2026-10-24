#pragma once

#include "mc/deps/core/utility/optional_ref.h"
#include "ll/api/network/packet/Packet.h"

namespace thirsty {

// Both builds register the same packet name and wire format.
class ThirstPacket final : public ll::network::PacketBase<ThirstPacket> {
public:
    uint64 actorId = 0;
    uchar  value   = 20;
    bool   visible = false;

    void                  write(BinaryStream& stream) const override;
    Bedrock::Result<void> read(ReadOnlyBinaryStream& stream) override;
};

} // namespace thirsty

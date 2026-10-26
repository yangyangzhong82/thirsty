#include "mod/ThirstPacket.h"

#include "mc/deps/core/utility/BinaryStream.h"

namespace thirsty {

void ThirstPacket::write(BinaryStream& stream) const {
    stream.writeUnsignedVarInt64(actorId, nullptr, nullptr);
    stream.writeUnsignedVarInt(value, nullptr, nullptr);
    stream.writeBool(visible, nullptr, nullptr);
    for (auto points : restores) stream.writeByte(points, nullptr, nullptr);
    for (auto chance : debuffChances) stream.writeByte(chance, nullptr, nullptr);
}

Bedrock::Result<void> ThirstPacket::read(ReadOnlyBinaryStream& stream) {
    auto id = stream.getUnsignedVarInt64();
    if (!id) return nonstd::make_unexpected(id.error());
    auto points = stream.getUnsignedVarInt();
    if (!points) return nonstd::make_unexpected(points.error());
    auto show = stream.getBool();
    if (!show) return nonstd::make_unexpected(show.error());
    for (auto& pointsRestored : restores) {
        auto result = stream.getByte();
        if (!result) return nonstd::make_unexpected(result.error());
        pointsRestored = *result;
    }
    for (auto& chance : debuffChances) {
        auto result = stream.getByte();
        if (!result) return nonstd::make_unexpected(result.error());
        chance = *result;
    }
    actorId = *id;
    value   = *points;
    visible = *show;
    return stream.ensureReadCompleted();
}

void HandDrinkPacket::write(BinaryStream&) const {}

Bedrock::Result<void> HandDrinkPacket::read(ReadOnlyBinaryStream& stream) {
    return stream.ensureReadCompleted();
}

} // namespace thirsty

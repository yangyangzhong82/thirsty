#include "mod/ThirstPacket.h"

#include "mc/deps/core/utility/BinaryStream.h"

namespace thirsty {

void ThirstPacket::write(BinaryStream& stream) const {
    stream.writeUnsignedVarInt64(actorId, nullptr, nullptr);
    stream.writeByte(value, nullptr, nullptr);
    stream.writeBool(visible, nullptr, nullptr);
}

Bedrock::Result<void> ThirstPacket::read(ReadOnlyBinaryStream& stream) {
    auto id = stream.getUnsignedVarInt64();
    if (!id) return nonstd::make_unexpected(id.error());
    auto points = stream.getByte();
    if (!points) return nonstd::make_unexpected(points.error());
    auto show = stream.getBool();
    if (!show) return nonstd::make_unexpected(show.error());
    actorId = *id;
    value   = *points;
    visible = *show;
    return stream.ensureReadCompleted();
}

} // namespace thirsty

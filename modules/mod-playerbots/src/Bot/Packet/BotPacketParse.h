/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITY_BOT_PACKET_PARSE_H
#define TRINITY_BOT_PACKET_PARSE_H

#include "ByteBuffer.h"
#include "Log.h"
#include "WorldPacket.h"

namespace Playerbots::PacketParse
{
// Living pin for Layer 3 fixtures — bump when baseline client build advances and re-capture.
inline constexpr uint32 VERIFIED_BUILD = 68453; // 12.0.7.68453

enum class Result
{
    Ok,
    Exception,
    Incomplete, // remaining bytes after reader returned
};

// Layer 1: run reader on a copy; require full consume (rpos == size) unless allowTrailingBytes.
// Never throws out of this helper — ByteBuffer exceptions become Result::Exception + ERROR log.
template <typename Reader>
Result TryReadFully(WorldPacket const& packet, char const* opcodeLabel, Reader&& reader,
    size_t allowTrailingBytes = 0)
{
    WorldPacket copy(packet);
    copy.rpos(0);
    try
    {
        reader(copy);
    }
    catch (ByteBufferException const& ex)
    {
        TC_LOG_ERROR("playerbots.packet",
            "BotPacketParse {}: ByteBuffer exception at rpos={} size={} verifiedBuild={}: {}",
            opcodeLabel, copy.rpos(), copy.size(), VERIFIED_BUILD, ex.what());
        return Result::Exception;
    }
    catch (std::exception const& ex)
    {
        TC_LOG_ERROR("playerbots.packet",
            "BotPacketParse {}: std::exception at rpos={} size={} verifiedBuild={}: {}",
            opcodeLabel, copy.rpos(), copy.size(), VERIFIED_BUILD, ex.what());
        return Result::Exception;
    }

    size_t const remaining = copy.size() > copy.rpos() ? copy.size() - copy.rpos() : 0;
    if (remaining > allowTrailingBytes)
    {
        TC_LOG_ERROR("playerbots.packet",
            "BotPacketParse {}: incomplete consume rpos={} size={} remaining={} allowTrailing={} verifiedBuild={}",
            opcodeLabel, copy.rpos(), copy.size(), remaining, allowTrailingBytes, VERIFIED_BUILD);
        return Result::Incomplete;
    }

    return Result::Ok;
}
}

#endif

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

#include "BotBattlefieldStatusPackets.h"
#include "BotPacketParse.h"
#include "PacketOperators.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadBattlefieldStatusHeader(ByteBuffer& data, WorldPackets::Battleground::BattlefieldStatusHeader& out)
{
    // Mirror WorldPackets::Battleground::operator<<(BattlefieldStatusHeader) field order exactly.
    // Size precedes Range*/TeamSize/InstanceID; QueueID payload follows those fields.
    data >> out.Ticket;
    data >> WorldPackets::Size<uint32>(out.QueueID);
    data >> out.RangeMin;
    data >> out.RangeMax;
    data >> out.TeamSize;
    data >> out.InstanceID;
    for (uint64& queueID : out.QueueID)
        data >> queueID;

    data >> WorldPackets::Bits<1>(out.RegisteredMatch);
    data >> WorldPackets::Bits<1>(out.TournamentRules);
    data.ResetBitPos();
}

void ReadNeedConfirmationBody(ByteBuffer& data, BattlefieldStatusNeedConfirmationPayload& out)
{
    // Mirror BattlefieldStatusNeedConfirmation::Write() field order exactly.
    ReadBattlefieldStatusHeader(data, out.Hdr);
    data >> out.Mapid;
    data >> out.Timeout;
    data >> out.Role;
}

void ReadQueuedBody(ByteBuffer& data, BattlefieldStatusQueuedPayload& out)
{
    // Mirror BattlefieldStatusQueued::Write() field order exactly.
    ReadBattlefieldStatusHeader(data, out.Hdr);
    data >> out.AverageWaitTime;
    data >> out.WaitTime;
    data >> out.SpecSelected;
    data >> WorldPackets::Bits<1>(out.AsGroup);
    data >> WorldPackets::Bits<1>(out.EligibleForMatchmaking);
    data >> WorldPackets::Bits<1>(out.SuspendedQueue);
    data.ResetBitPos();
}

void ReadActiveBody(ByteBuffer& data, BattlefieldStatusActivePayload& out)
{
    // Mirror BattlefieldStatusActive::Write() field order exactly.
    ReadBattlefieldStatusHeader(data, out.Hdr);
    data >> out.Mapid;
    data >> out.ShutdownTimer;
    data >> out.StartTimer;
    data >> out.ArenaFaction;
    data >> WorldPackets::Bits<1>(out.LeftEarly);
    data >> WorldPackets::Bits<1>(out.Brawl);
    data.ResetBitPos();
}
}

bool TryReadBattlefieldStatusNeedConfirmation(WorldPacket const& packet, BattlefieldStatusNeedConfirmationPayload& out)
{
    BattlefieldStatusNeedConfirmationPayload parsed;
    Result const result = TryReadFully(packet, "SMSG_BATTLEFIELD_STATUS_NEED_CONFIRMATION", [&parsed](WorldPacket& copy)
    {
        ReadNeedConfirmationBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}

bool TryReadBattlefieldStatusQueued(WorldPacket const& packet, BattlefieldStatusQueuedPayload& out)
{
    BattlefieldStatusQueuedPayload parsed;
    Result const result = TryReadFully(packet, "SMSG_BATTLEFIELD_STATUS_QUEUED", [&parsed](WorldPacket& copy)
    {
        ReadQueuedBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}

bool TryReadBattlefieldStatusActive(WorldPacket const& packet, BattlefieldStatusActivePayload& out)
{
    BattlefieldStatusActivePayload parsed;
    Result const result = TryReadFully(packet, "SMSG_BATTLEFIELD_STATUS_ACTIVE", [&parsed](WorldPacket& copy)
    {
        ReadActiveBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}

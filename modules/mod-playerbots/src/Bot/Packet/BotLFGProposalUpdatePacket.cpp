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

#include "BotLFGProposalUpdatePacket.h"
#include "BotPacketParse.h"
#include "PacketOperators.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadLFGProposalUpdatePlayer(ByteBuffer& data, LFGProposalUpdatePlayerPayload& out)
{
    // Mirror WorldPackets::LFG::operator<<(LFGProposalUpdatePlayer) field order exactly.
    data >> out.Roles;
    data >> WorldPackets::Bits<1>(out.Me);
    data >> WorldPackets::Bits<1>(out.SameParty);
    data >> WorldPackets::Bits<1>(out.MyParty);
    data >> WorldPackets::Bits<1>(out.Responded);
    data >> WorldPackets::Bits<1>(out.Accepted);
    data.ResetBitPos();
}

void ReadLFGProposalUpdateBody(ByteBuffer& data, LFGProposalUpdatePayload& out)
{
    // Mirror WorldPackets::LFG::LFGProposalUpdate::Write() field order exactly.
    // RideTicket via TC operator>> (Guid + Id + Type + Time + IsCrossFaction + ResetBitPos).
    data >> out.Ticket;
    data >> out.InstanceID;
    data >> out.ProposalID;
    data >> out.Slot;
    data >> out.State;
    data >> out.CompletedMask;
    data >> out.EncounterMask;
    data >> WorldPackets::Size<uint32>(out.Players);
    data >> out.PromisedShortageRolePriority;
    data >> WorldPackets::Bits<1>(out.ValidCompletedMask);
    data >> WorldPackets::Bits<1>(out.ProposalSilent);
    data >> WorldPackets::Bits<1>(out.FailedByMyParty);
    data.ResetBitPos();

    for (LFGProposalUpdatePlayerPayload& player : out.Players)
        ReadLFGProposalUpdatePlayer(data, player);
}
}

bool TryReadLFGProposalUpdate(WorldPacket const& packet, LFGProposalUpdatePayload& out)
{
    LFGProposalUpdatePayload parsed;
    Result const result = TryReadFully(packet, "SMSG_LFG_PROPOSAL_UPDATE", [&parsed](WorldPacket& copy)
    {
        ReadLFGProposalUpdateBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}

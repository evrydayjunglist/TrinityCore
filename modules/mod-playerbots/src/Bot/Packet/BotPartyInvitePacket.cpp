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

#include "BotPartyInvitePacket.h"
#include "BotPacketParse.h"
#include "PacketOperators.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadVirtualRealmNameInfo(ByteBuffer& data, WorldPackets::Auth::VirtualRealmNameInfo& nameInfo)
{
    nameInfo.IsLocal = data.ReadBit();
    nameInfo.IsInternalRealm = data.ReadBit();
    data >> WorldPackets::SizedString::BitsSize<8>(nameInfo.RealmNameActual);
    data >> WorldPackets::SizedString::BitsSize<8>(nameInfo.RealmNameNormalized);
    data.ResetBitPos();
    data >> WorldPackets::SizedString::Data(nameInfo.RealmNameActual);
    data >> WorldPackets::SizedString::Data(nameInfo.RealmNameNormalized);
}

void ReadVirtualRealmInfo(ByteBuffer& data, WorldPackets::Auth::VirtualRealmInfo& realmInfo)
{
    data >> realmInfo.RealmAddress;
    ReadVirtualRealmNameInfo(data, realmInfo.RealmNameInfo);
}

void ReadPartyInviteBody(ByteBuffer& data, PartyInvitePayload& out)
{
    // Mirror WorldPackets::Party::PartyInvite::Write() field order exactly.
    data >> WorldPackets::Bits<1>(out.CanAccept);
    data >> WorldPackets::Bits<1>(out.IsXRealm);
    data >> WorldPackets::Bits<1>(out.IsXNativeRealm);
    data >> WorldPackets::Bits<1>(out.ShouldSquelch);
    data >> WorldPackets::Bits<1>(out.AllowMultipleRoles);
    data >> WorldPackets::Bits<1>(out.QuestSessionActive);
    data >> WorldPackets::SizedString::BitsSize<6>(out.InviterName);
    data >> WorldPackets::Bits<1>(out.IsCrossFaction);

    ReadVirtualRealmInfo(data, out.InviterRealm);
    data >> out.InviterGUID;
    data >> out.InviterBNetAccountId;
    data >> out.InviterCfgRealmID;
    data >> out.ProposedRoles;
    data >> WorldPackets::Size<uint32>(out.LfgSlots);
    data >> out.LfgCompletedMask;

    data >> WorldPackets::SizedString::Data(out.InviterName);

    for (uint32& slot : out.LfgSlots)
        data >> slot;
}
}

bool TryReadPartyInvite(WorldPacket const& packet, PartyInvitePayload& out)
{
    PartyInvitePayload parsed;
    Result const result = TryReadFully(packet, "SMSG_PARTY_INVITE", [&parsed](WorldPacket& copy)
    {
        ReadPartyInviteBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}

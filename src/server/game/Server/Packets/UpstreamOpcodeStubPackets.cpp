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

#include "UpstreamOpcodeStubPackets.h"
#include "PacketOperators.h"

namespace WorldPackets::UpstreamStub
{
void GetRemainingGameTime::Read()
{
    _worldPacket >> UnkInt32;
}

void RequestStoreFrontInfoUpdate::Read()
{
    _worldPacket >> StoreFrontID;

    uint32 currencyCount;
    _worldPacket >> currencyCount;
    CurrencyIDs.resize(currencyCount);
    for (int32& currencyId : CurrencyIDs)
        _worldPacket >> currencyId;
}

void GetRafAccountInfo::Read()
{
    _worldPacket >> UnkUInt32;
}

void RequestInstanceEncounterEventSync::Read()
{
    // Capture E F2-6: consume-only at login (no SMSG when idle). Wire is 9 bytes on 68275;
    // field layout deferred until encounter sync gameplay — consume full payload retail-like.
    _worldPacket.rfinish();
}

void QuickJoinAutoAcceptRequests::Read()
{
    AutoAccept = _worldPacket.ReadBit();
}

void OpenTradeskillNpc::Read()
{
    _worldPacket >> NpcGuid;
}

void OverrideScreenFlash::Read()
{
    Override = _worldPacket.ReadBit();
}

void SetExcludedChatCensorSources::Read()
{
    Exclude = _worldPacket.ReadBit();
}

WorldPacket const* LastCatalogFetchResponse::Write()
{
    _worldPacket << uint32(CatalogVersion);
    _worldPacket << uint32(UnkUInt32);
    return &_worldPacket;
}

WorldPacket const* AccountStoreFrontUpdate::Write()
{
    _worldPacket << uint32(Token);
    _worldPacket << uint64(UnkUInt64);
    _worldPacket << uint16(UnkUInt16);
    return &_worldPacket;
}

WorldPacket const* GetLandingPageShipmentsResponse::Write()
{
    _worldPacket << uint32(UnkUInt32);
    _worldPacket << Size<uint32>(Shipments);
    for (int32 shipment : Shipments)
        _worldPacket << int32(shipment);

    return &_worldPacket;
}

WorldPacket const* GetRemainingGameTimeResponse::Write()
{
    _worldPacket << int32(RemainingSeconds);
    _worldPacket << int32(UnkInt32);
    _worldPacket << uint8(UnkByte);
    return &_worldPacket;
}

WorldPacket const* RafAccountInfo::Write()
{
    _worldPacket.WriteBit(HasActiveRecruit);
    _worldPacket.FlushBits();
    return &_worldPacket;
}

WorldPacket const* RequestScheduledPvpInfoResponse::Write()
{
    _worldPacket.WriteBit(BrawlInfo.has_value());
    _worldPacket.WriteBit(SpecialEventInfo.has_value());
    _worldPacket.FlushBits();

    if (BrawlInfo)
    {
        _worldPacket << int32(BrawlInfo->PvpBrawlId);
        _worldPacket << int32(BrawlInfo->Time);
        _worldPacket.WriteBit(BrawlInfo->Started);
        _worldPacket.FlushBits();
    }

    if (SpecialEventInfo)
    {
        _worldPacket << int32(SpecialEventInfo->PvpBrawlID);
        _worldPacket << int32(SpecialEventInfo->AchievementId);
        _worldPacket.WriteBit(SpecialEventInfo->CanQueue);
        _worldPacket.FlushBits();
    }

    return &_worldPacket;
}

WorldPacket const* WeeklyRewardsResult::Write()
{
    _worldPacket << Size<uint32>(ActivityIDs);
    for (int32 activityId : ActivityIDs)
        _worldPacket << int32(activityId);

    return &_worldPacket;
}

WorldPacket const* WeeklyRewardsProgressResult::Write()
{
    _worldPacket << Size<uint32>(ActivityIDs);
    for (int32 activityId : ActivityIDs)
        _worldPacket << int32(activityId);

    return &_worldPacket;
}

WorldPacket const* MythicPlusSeasonData::Write()
{
    _worldPacket.WriteBit(IsMythicPlusActive);
    _worldPacket.FlushBits();
    return &_worldPacket;
}

WorldPacket const* LfgListUpdateStatus::Write()
{
    _worldPacket << Ticket;
    _worldPacket << RemainingTime;
    _worldPacket << uint8(ResultId);
    _worldPacket << int32(JoinRequest.ActivityID);
    _worldPacket << float(JoinRequest.RequiredItemLevel);
    _worldPacket << SizedString::BitsSize<8>(JoinRequest.Name);
    _worldPacket << SizedString::BitsSize<11>(JoinRequest.Comment);
    _worldPacket << SizedString::BitsSize<8>(JoinRequest.VoiceChat);
    _worldPacket.FlushBits();
    _worldPacket << SizedString::Data(JoinRequest.Name);
    _worldPacket << SizedString::Data(JoinRequest.Comment);
    _worldPacket << SizedString::Data(JoinRequest.VoiceChat);
    _worldPacket.WriteBit(Listed);
    _worldPacket.FlushBits();
    return &_worldPacket;
}

WorldPacket const* LfgListUpdateBlacklist::Write()
{
    _worldPacket << int32(Entries.size());
    for (LfgListBlacklistEntry const& entry : Entries)
    {
        _worldPacket << int32(entry.ActivityID);
        _worldPacket << int32(entry.Reason);
    }

    return &_worldPacket;
}

WorldPacket const* InstanceEncounterEventSequence::Write()
{
    _worldPacket << Size<uint32>(Events);
    for (uint32 eventId : Events)
        _worldPacket << uint32(eventId);

    return &_worldPacket;
}

WorldPacket const* PerksProgramDisabled::Write()
{
    return &_worldPacket;
}

WorldPacket const* GetAccountCharacterListResult::Write()
{
    _worldPacket << uint32(Token);
    _worldPacket << Size<uint32>(Characters);
    _worldPacket.WriteBit(ConsoleCommand);
    _worldPacket.FlushBits();

    for (AccountCharacterListEntry const& character : Characters)
    {
        _worldPacket << character.Guid;
        _worldPacket << character.WowAccountGuid;
        _worldPacket << uint32(character.VirtualRealmAddress);
        _worldPacket << uint32(character.NativeRealmAddress);
        _worldPacket << SizedString::BitsSize<6>(character.Name);
        _worldPacket << SizedString::BitsSize<9>(character.RealmName);
        _worldPacket.FlushBits();
        _worldPacket << SizedString::Data(character.Name);
        _worldPacket << SizedString::Data(character.RealmName);
    }

    return &_worldPacket;
}
}

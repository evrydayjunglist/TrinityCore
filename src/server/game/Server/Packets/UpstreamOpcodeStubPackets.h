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

#ifndef TRINITYCORE_UPSTREAM_OPCODE_STUB_PACKETS_H
#define TRINITYCORE_UPSTREAM_OPCODE_STUB_PACKETS_H

#include "LFGPacketsCommon.h"
#include "ObjectGuid.h"
#include "Packet.h"
#include "PacketUtilities.h"

namespace WorldPackets::UpstreamStub
{
    // --- Tier B / C CMSG (Read) ------------------------------------------------

    class GetLastCatalogFetch final : public ClientPacket
    {
    public:
        explicit GetLastCatalogFetch(WorldPacket&& packet) : ClientPacket(CMSG_GET_LAST_CATALOG_FETCH, std::move(packet)) { }

        void Read() override { }
    };

    class RequestStoreFrontInfoUpdate final : public ClientPacket
    {
    public:
        explicit RequestStoreFrontInfoUpdate(WorldPacket&& packet) : ClientPacket(CMSG_REQUEST_STORE_FRONT_INFO_UPDATE, std::move(packet)) { }

        void Read() override;

        uint32 StoreFrontID = 0;
        std::vector<int32> CurrencyIDs;
    };

    class GetLandingPageShipments final : public ClientPacket
    {
    public:
        explicit GetLandingPageShipments(WorldPacket&& packet) : ClientPacket(CMSG_GET_LANDING_PAGE_SHIPMENTS, std::move(packet)) { }

        void Read() override { }
    };

    class GetRemainingGameTime final : public ClientPacket
    {
    public:
        explicit GetRemainingGameTime(WorldPacket&& packet) : ClientPacket(CMSG_GET_REMAINING_GAME_TIME, std::move(packet)) { }

        void Read() override;

        uint32 UnkInt32 = 0;
    };

    class GetRafAccountInfo final : public ClientPacket
    {
    public:
        explicit GetRafAccountInfo(WorldPacket&& packet) : ClientPacket(CMSG_GET_RAF_ACCOUNT_INFO, std::move(packet)) { }

        void Read() override;

        uint32 UnkUInt32 = 0;
    };

    class QuickJoinAutoAcceptRequests final : public ClientPacket
    {
    public:
        explicit QuickJoinAutoAcceptRequests(WorldPacket&& packet) : ClientPacket(CMSG_QUICK_JOIN_AUTO_ACCEPT_REQUESTS, std::move(packet)) { }

        void Read() override;

        bool AutoAccept = false;
    };

    class RequestScheduledPvpInfo final : public ClientPacket
    {
    public:
        explicit RequestScheduledPvpInfo(WorldPacket&& packet) : ClientPacket(CMSG_REQUEST_SCHEDULED_PVP_INFO, std::move(packet)) { }

        void Read() override { }
    };

    class RequestWeeklyRewards final : public ClientPacket
    {
    public:
        explicit RequestWeeklyRewards(WorldPacket&& packet) : ClientPacket(CMSG_REQUEST_WEEKLY_REWARDS, std::move(packet)) { }

        void Read() override { }
    };

    class RequestMythicPlusSeasonData final : public ClientPacket
    {
    public:
        explicit RequestMythicPlusSeasonData(WorldPacket&& packet) : ClientPacket(CMSG_REQUEST_MYTHIC_PLUS_SEASON_DATA, std::move(packet)) { }

        void Read() override { }
    };

    class LfgListGetStatus final : public ClientPacket
    {
    public:
        explicit LfgListGetStatus(WorldPacket&& packet) : ClientPacket(CMSG_LFG_LIST_GET_STATUS, std::move(packet)) { }

        void Read() override { }
    };

    class RequestLfgListBlacklist final : public ClientPacket
    {
    public:
        explicit RequestLfgListBlacklist(WorldPacket&& packet) : ClientPacket(CMSG_REQUEST_LFG_LIST_BLACKLIST, std::move(packet)) { }

        void Read() override { }
    };

    class RequestInstanceEncounterEventSync final : public ClientPacket
    {
    public:
        explicit RequestInstanceEncounterEventSync(WorldPacket&& packet) : ClientPacket(CMSG_REQUEST_INSTANCE_ENCOUNTER_EVENT_SYNC, std::move(packet)) { }

        void Read() override;

        ObjectGuid EncounterGuid;
        uint8 SequenceIndex = 0;
    };

    class OpenTradeskillNpc final : public ClientPacket
    {
    public:
        explicit OpenTradeskillNpc(WorldPacket&& packet) : ClientPacket(CMSG_OPEN_TRADESKILL_NPC, std::move(packet)) { }

        void Read() override;

        ObjectGuid NpcGuid;
    };

    class PerksProgramStatusRequest final : public ClientPacket
    {
    public:
        explicit PerksProgramStatusRequest(WorldPacket&& packet) : ClientPacket(CMSG_PERKS_PROGRAM_STATUS_REQUEST, std::move(packet)) { }

        void Read() override { }
    };

    class GetAccountCharacterList final : public ClientPacket
    {
    public:
        explicit GetAccountCharacterList(WorldPacket&& packet) : ClientPacket(CMSG_GET_ACCOUNT_CHARACTER_LIST, std::move(packet)) { }

        void Read() override { }
    };

    class OverrideScreenFlash final : public ClientPacket
    {
    public:
        explicit OverrideScreenFlash(WorldPacket&& packet) : ClientPacket(CMSG_OVERRIDE_SCREEN_FLASH, std::move(packet)) { }

        void Read() override;

        bool Override = false;
    };

    class SetExcludedChatCensorSources final : public ClientPacket
    {
    public:
        explicit SetExcludedChatCensorSources(WorldPacket&& packet) : ClientPacket(CMSG_SET_EXCLUDED_CHAT_CENSOR_SOURCES, std::move(packet)) { }

        void Read() override;

        bool Exclude = false;
    };

    // --- SMSG (Write) ------------------------------------------------------------

    class LastCatalogFetchResponse final : public ServerPacket
    {
    public:
        LastCatalogFetchResponse() : ServerPacket(SMSG_LAST_CATALOG_FETCH_RESPONSE, 8) { }

        WorldPacket const* Write() override;

        uint32 CatalogVersion = 0;
        uint32 UnkUInt32 = 0;
    };

    class AccountStoreFrontUpdate final : public ServerPacket
    {
    public:
        AccountStoreFrontUpdate() : ServerPacket(SMSG_ACCOUNT_STORE_FRONT_UPDATE, 14) { }

        WorldPacket const* Write() override;

        uint32 Token = 0;
        uint64 UnkUInt64 = 0;
        uint16 UnkUInt16 = 0;
    };

    class GetLandingPageShipmentsResponse final : public ServerPacket
    {
    public:
        GetLandingPageShipmentsResponse() : ServerPacket(SMSG_GET_LANDING_PAGE_SHIPMENTS_RESPONSE, 8) { }

        WorldPacket const* Write() override;

        uint32 UnkUInt32 = 0;
        std::vector<int32> Shipments;
    };

    class GetRemainingGameTimeResponse final : public ServerPacket
    {
    public:
        GetRemainingGameTimeResponse() : ServerPacket(SMSG_GET_REMAINING_GAME_TIME_RESPONSE, 9) { }

        WorldPacket const* Write() override;

        int32 RemainingSeconds = 0;
        int32 UnkInt32 = 0;
        uint8 UnkByte = 0;
    };

    class RafAccountInfo final : public ServerPacket
    {
    public:
        RafAccountInfo() : ServerPacket(SMSG_RAF_ACCOUNT_INFO, 1) { }

        WorldPacket const* Write() override;

        bool HasActiveRecruit = false;
    };

    struct ScheduledPvpBrawlInfo
    {
        int32 PvpBrawlId = 0;
        int32 Time = 0;
        bool Started = false;
    };

    struct ScheduledPvpSpecialEventInfo
    {
        int32 PvpBrawlID = 0;
        int32 AchievementId = 0;
        bool CanQueue = false;
    };

    class RequestScheduledPvpInfoResponse final : public ServerPacket
    {
    public:
        RequestScheduledPvpInfoResponse() : ServerPacket(SMSG_REQUEST_SCHEDULED_PVP_INFO_RESPONSE, 1) { }

        WorldPacket const* Write() override;

        Optional<ScheduledPvpBrawlInfo> BrawlInfo;
        Optional<ScheduledPvpSpecialEventInfo> SpecialEventInfo;
    };

    class WeeklyRewardsResult final : public ServerPacket
    {
    public:
        WeeklyRewardsResult() : ServerPacket(SMSG_WEEKLY_REWARDS_RESULT, 4) { }

        WorldPacket const* Write() override;

        std::vector<int32> ActivityIDs;
    };

    class WeeklyRewardsProgressResult final : public ServerPacket
    {
    public:
        WeeklyRewardsProgressResult() : ServerPacket(SMSG_WEEKLY_REWARDS_PROGRESS_RESULT, 4) { }

        WorldPacket const* Write() override;

        std::vector<int32> ActivityIDs;
    };

    class MythicPlusSeasonData final : public ServerPacket
    {
    public:
        MythicPlusSeasonData() : ServerPacket(SMSG_MYTHIC_PLUS_SEASON_DATA, 1) { }

        WorldPacket const* Write() override;

        bool IsMythicPlusActive = false;
    };

    struct LfgListJoinRequest
    {
        int32 ActivityID = 0;
        float RequiredItemLevel = 0.0f;
        std::string Name;
        std::string Comment;
        std::string VoiceChat;
    };

    class LfgListUpdateStatus final : public ServerPacket
    {
    public:
        LfgListUpdateStatus() : ServerPacket(SMSG_LFG_LIST_UPDATE_STATUS, 16) { }

        WorldPacket const* Write() override;

        LFG::RideTicket Ticket;
        Timestamp<> RemainingTime;
        uint8 ResultId = 0;
        LfgListJoinRequest JoinRequest;
        bool Listed = false;
    };

    struct LfgListBlacklistEntry
    {
        int32 ActivityID = 0;
        int32 Reason = 0;
    };

    class LfgListUpdateBlacklist final : public ServerPacket
    {
    public:
        LfgListUpdateBlacklist() : ServerPacket(SMSG_LFG_LIST_UPDATE_BLACKLIST, 4) { }

        WorldPacket const* Write() override;

        std::vector<LfgListBlacklistEntry> Entries;
    };

    class InstanceEncounterEventSequence final : public ServerPacket
    {
    public:
        InstanceEncounterEventSequence() : ServerPacket(SMSG_INSTANCE_ENCOUNTER_EVENT_SEQUENCE, 4) { }

        WorldPacket const* Write() override;

        std::vector<uint32> Events;
    };

    class PerksProgramDisabled final : public ServerPacket
    {
    public:
        PerksProgramDisabled() : ServerPacket(SMSG_PERKS_PROGRAM_DISABLED, 0) { }

        WorldPacket const* Write() override;
    };

    struct AccountCharacterListEntry
    {
        ObjectGuid Guid;
        ObjectGuid WowAccountGuid;
        uint32 VirtualRealmAddress = 0;
        uint32 NativeRealmAddress = 0;
        std::string Name;
        std::string RealmName;
    };

    class GetAccountCharacterListResult final : public ServerPacket
    {
    public:
        GetAccountCharacterListResult() : ServerPacket(SMSG_GET_ACCOUNT_CHARACTER_LIST_RESULT, 4) { }

        WorldPacket const* Write() override;

        uint32 Token = 0;
        bool ConsoleCommand = false;
        std::vector<AccountCharacterListEntry> Characters;
    };
}

#endif // TRINITYCORE_UPSTREAM_OPCODE_STUB_PACKETS_H

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

#include "WorldSession.h"
#include "UpstreamOpcodeStubPackets.h"

void WorldSession::HandleGetLastCatalogFetch(WorldPackets::UpstreamStub::GetLastCatalogFetch& /*packet*/)
{
    WorldPackets::UpstreamStub::LastCatalogFetchResponse response;
    /// @todo: catalog shop backend
    SendPacket(response.Write());
}

void WorldSession::HandleRequestStoreFrontInfoUpdate(WorldPackets::UpstreamStub::RequestStoreFrontInfoUpdate& /*packet*/)
{
    WorldPackets::UpstreamStub::AccountStoreFrontUpdate response;
    /// @todo: account store front
    SendPacket(response.Write());
}

void WorldSession::HandleGetLandingPageShipments(WorldPackets::UpstreamStub::GetLandingPageShipments& /*packet*/)
{
    WorldPackets::UpstreamStub::GetLandingPageShipmentsResponse response;
    /// @todo: garrison landing page shipments
    SendPacket(response.Write());
}

void WorldSession::HandleGetRemainingGameTime(WorldPackets::UpstreamStub::GetRemainingGameTime& /*packet*/)
{
    WorldPackets::UpstreamStub::GetRemainingGameTimeResponse response;
    /// @todo: remaining game time subscription UI
    SendPacket(response.Write());
}

void WorldSession::HandleGetRafAccountInfo(WorldPackets::UpstreamStub::GetRafAccountInfo& /*packet*/)
{
    WorldPackets::UpstreamStub::RafAccountInfo response;
    /// @todo: recruit-a-friend account panel
    SendPacket(response.Write());
}

void WorldSession::HandleQuickJoinAutoAcceptRequests(WorldPackets::UpstreamStub::QuickJoinAutoAcceptRequests& /*packet*/)
{
    /// @todo: quick join auto-accept preference persistence
}

void WorldSession::HandleRequestScheduledPvpInfo(WorldPackets::UpstreamStub::RequestScheduledPvpInfo& /*packet*/)
{
    WorldPackets::UpstreamStub::RequestScheduledPvpInfoResponse response;
    /// @todo: scheduled PvP brawl / special event data
    SendPacket(response.Write());
}

void WorldSession::HandleRequestWeeklyRewards(WorldPackets::UpstreamStub::RequestWeeklyRewards& /*packet*/)
{
    WorldPackets::UpstreamStub::WeeklyRewardsProgressResult progress;
    /// @todo: great vault weekly rewards — Capture E: retail sends PROGRESS only (no RESULT at login)
    SendPacket(progress.Write());
}

void WorldSession::HandleRequestMythicPlusSeasonData(WorldPackets::UpstreamStub::RequestMythicPlusSeasonData& /*packet*/)
{
    WorldPackets::UpstreamStub::MythicPlusSeasonData response;
    /// @todo: mythic+ season active flag from DB2
    SendPacket(response.Write());
}

void WorldSession::HandleLfgListGetStatus(WorldPackets::UpstreamStub::LfgListGetStatus& /*packet*/)
{
    WorldPackets::UpstreamStub::LfgListUpdateStatus response;
    /// @todo: premade group finder listing status
    SendPacket(response.Write());
}

void WorldSession::HandleRequestLfgListBlacklist(WorldPackets::UpstreamStub::RequestLfgListBlacklist& /*packet*/)
{
    WorldPackets::UpstreamStub::LfgListUpdateBlacklist response;
    /// @todo: premade group finder blacklist
    SendPacket(response.Write());
}

void WorldSession::HandleRequestInstanceEncounterEventSync(WorldPackets::UpstreamStub::RequestInstanceEncounterEventSync& /*packet*/)
{
    /// @todo: instance encounter event sync — Capture E: no SMSG when no active encounter sequence
}

void WorldSession::HandleOpenTradeskillNpc(WorldPackets::UpstreamStub::OpenTradeskillNpc& /*packet*/)
{
    /// @todo: professions NPC recipes (SMSG_UPDATE_CRAFTING_NPC_RECIPES)
}

void WorldSession::HandlePerksProgramStatusRequest(WorldPackets::UpstreamStub::PerksProgramStatusRequest& /*packet*/)
{
    WorldPackets::UpstreamStub::PerksProgramDisabled response;
    /// @todo: perks program vendor tray
    SendPacket(response.Write());
}

void WorldSession::HandleGetAccountCharacterList(WorldPackets::UpstreamStub::GetAccountCharacterList& /*packet*/)
{
    WorldPackets::UpstreamStub::GetAccountCharacterListResult response;
    /// @todo: account-wide character list (not char enum)
    SendPacket(response.Write());
}

void WorldSession::HandleOverrideScreenFlash(WorldPackets::UpstreamStub::OverrideScreenFlash& /*packet*/)
{
    /// @todo: client screen flash override preference
}

void WorldSession::HandleSetExcludedChatCensorSources(WorldPackets::UpstreamStub::SetExcludedChatCensorSources& /*packet*/)
{
    /// @todo: chat censor source preferences
}

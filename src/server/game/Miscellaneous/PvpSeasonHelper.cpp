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

#include "PvpSeasonHelper.h"
#include "BattlegroundPackets.h"
#include "DB2Stores.h"
#include "Log.h"
#include "World.h"

namespace PvpSeasonHelper
{
namespace
{
// Capture Q (12.0.7.68275) — gap (stub retained): no DB2 column mapping on build 68275.
constexpr int32 MythicPlusMilestoneSeasonIdStub = 117;
constexpr int32 SeasonInfoUnknown1027_1Stub = 1069;

bool HasPvpSeasonMilestone(int32 milestone)
{
    for (PvpSeasonEntry const* entry : sPvpSeasonStore)
        if (entry->MilestoneSeason == milestone)
            return true;

    return false;
}

bool HasMythicPlusMilestone(int32 milestone)
{
    for (MythicPlusSeasonEntry const* entry : sMythicPlusSeasonStore)
        if (entry->MilestoneSeason == milestone)
            return true;

    return false;
}

bool HasRatedMilestone(int32 milestone)
{
    return HasPvpSeasonMilestone(milestone) || HasMythicPlusMilestone(milestone);
}
}

void Validate()
{
    uint32 const arenaSeasonId = sWorld->getIntConfig(CONFIG_ARENA_SEASON_ID);
    if (!sPvpSeasonStore.LookupEntry(arenaSeasonId))
        TC_LOG_ERROR("server.loading", "Arena.ArenaSeason.ID ({}) is not a valid PvpSeason.db2 ID.", arenaSeasonId);

    uint32 const packetSeasonId = sWorld->getIntConfig(CONFIG_PVP_SEASON_PACKET_ID);
    if (!sPvpSeasonStore.LookupEntry(packetSeasonId))
        TC_LOG_ERROR("server.loading", "Arena.PvpSeason.PacketID ({}) is not a valid PvpSeason.db2 ID.", packetSeasonId);

    int32 const previousPacketSeasonId = GetPreviousPvpSeasonId();
    if (sWorld->getBoolConfig(CONFIG_ARENA_SEASON_IN_PROGRESS) && !sPvpSeasonStore.LookupEntry(previousPacketSeasonId))
        TC_LOG_ERROR("server.loading", "Previous PvpSeason id ({}) derived from Arena.PvpSeason.PacketID is not in PvpSeason.db2.", previousPacketSeasonId);

    int32 const currentMilestone = sWorld->getIntConfig(CONFIG_RATED_SEASON_CURRENT_MILESTONE);
    int32 const previousMilestone = sWorld->getIntConfig(CONFIG_RATED_SEASON_PREVIOUS_MILESTONE);
    if (!HasRatedMilestone(currentMilestone))
        TC_LOG_ERROR("server.loading", "Arena.RatedSeason.CurrentMilestone ({}) has no matching PvpSeason or MythicPlusSeason row.", currentMilestone);
    if (!HasRatedMilestone(previousMilestone))
        TC_LOG_ERROR("server.loading", "Arena.RatedSeason.PreviousMilestone ({}) has no matching PvpSeason or MythicPlusSeason row.", previousMilestone);

    uint32 const displaySeasonId = sWorld->getIntConfig(CONFIG_DISPLAY_SEASON_ID);
    if (!sDisplaySeasonStore.LookupEntry(displaySeasonId))
        TC_LOG_ERROR("server.loading", "Arena.DisplaySeason.ID ({}) is not a valid DisplaySeason.db2 ID.", displaySeasonId);

    TC_LOG_INFO("server.loading", ">> PvP season: game_event id {} packet id {} rated milestones {}/{} display season {} (Capture Q stubs: M+ milestone {}, unknown1027_1 {})",
        arenaSeasonId, packetSeasonId, currentMilestone, previousMilestone, displaySeasonId,
        MythicPlusMilestoneSeasonIdStub, SeasonInfoUnknown1027_1Stub);
}

PvpSeasonEntry const* GetActivePvpSeasonEntry()
{
    return sPvpSeasonStore.LookupEntry(sWorld->getIntConfig(CONFIG_PVP_SEASON_PACKET_ID));
}

int32 GetActivePvpSeasonId()
{
    return sWorld->getIntConfig(CONFIG_PVP_SEASON_PACKET_ID);
}

int32 GetPreviousPvpSeasonId()
{
    return GetActivePvpSeasonId() - sWorld->getBoolConfig(CONFIG_ARENA_SEASON_IN_PROGRESS);
}

int32 GetCurrentRatedMilestone()
{
    if (!sWorld->getBoolConfig(CONFIG_ARENA_SEASON_IN_PROGRESS))
        return 0;

    return sWorld->getIntConfig(CONFIG_RATED_SEASON_CURRENT_MILESTONE);
}

int32 GetPreviousRatedMilestone()
{
    if (sWorld->getBoolConfig(CONFIG_ARENA_SEASON_IN_PROGRESS))
        return sWorld->getIntConfig(CONFIG_RATED_SEASON_PREVIOUS_MILESTONE);

    return sWorld->getIntConfig(CONFIG_RATED_SEASON_CURRENT_MILESTONE);
}

void FillSeasonInfo(WorldPackets::Battleground::SeasonInfo& seasonInfo)
{
    if (DisplaySeasonEntry const* displaySeason = sDisplaySeasonStore.LookupEntry(sWorld->getIntConfig(CONFIG_DISPLAY_SEASON_ID)))
        seasonInfo.MythicPlusDisplaySeasonID = displaySeason->ID;

    seasonInfo.MythicPlusMilestoneSeasonID = MythicPlusMilestoneSeasonIdStub;
    seasonInfo.CurrentArenaSeason = GetCurrentRatedMilestone();
    seasonInfo.PreviousArenaSeason = GetPreviousRatedMilestone();
    seasonInfo.ConquestWeeklyProgressCurrencyID = 0;
    seasonInfo.PvpSeasonID = GetActivePvpSeasonId();
    seasonInfo.Unknown1027_1 = SeasonInfoUnknown1027_1Stub;
    seasonInfo.WeeklyRewardChestsEnabled = sWorld->getBoolConfig(CONFIG_ARENA_SEASON_IN_PROGRESS);
    seasonInfo.CurrentArenaSeasonUsesTeams = false;
    seasonInfo.PreviousArenaSeasonUsesTeams = false;
}

}

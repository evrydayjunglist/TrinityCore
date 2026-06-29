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

#ifndef TRINITY_PVP_SEASON_HELPER_H
#define TRINITY_PVP_SEASON_HELPER_H

#include "Define.h"

struct PvpSeasonEntry;

namespace WorldPackets::Battleground
{
    class SeasonInfo;
}

namespace PvpSeasonHelper
{
    void Validate();

    PvpSeasonEntry const* GetActivePvpSeasonEntry();
    int32 GetActivePvpSeasonId();
    int32 GetPreviousPvpSeasonId();
    int32 GetCurrentRatedMilestone();
    int32 GetPreviousRatedMilestone();

    void FillSeasonInfo(WorldPackets::Battleground::SeasonInfo& seasonInfo);
}

#endif // TRINITY_PVP_SEASON_HELPER_H

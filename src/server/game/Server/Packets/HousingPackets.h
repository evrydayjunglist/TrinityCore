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

#ifndef TRINITYCORE_HOUSING_PACKETS_H
#define TRINITYCORE_HOUSING_PACKETS_H

#include "Packet.h"

namespace WorldPackets::Housing
{
    class DeclineNeighborhoodInvites final : public ClientPacket
    {
    public:
        explicit DeclineNeighborhoodInvites(WorldPacket&& packet) : ClientPacket(CMSG_DECLINE_NEIGHBORHOOD_INVITES, std::move(packet)) { }

        void Read() override;

        bool Allow = false;
    };

    class GetPlayerHousesInfo final : public ClientPacket
    {
    public:
        explicit GetPlayerHousesInfo(WorldPacket&& packet) : ClientPacket(CMSG_HOUSING_SVCS_GET_PLAYER_HOUSES_INFO, std::move(packet)) { }

        void Read() override { }
    };

    class GetPlayerHousesInfoResponse final : public ServerPacket
    {
    public:
        GetPlayerHousesInfoResponse() : ServerPacket(SMSG_HOUSING_SVCS_GET_PLAYER_HOUSES_INFO_RESPONSE, 5) { }

        WorldPacket const* Write() override;

        uint32 PlayerHousesCount = 0;
        uint8 Result = 0;
    };
}

#endif // TRINITYCORE_HOUSING_PACKETS_H

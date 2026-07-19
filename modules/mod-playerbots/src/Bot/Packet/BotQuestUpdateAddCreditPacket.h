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

#ifndef TRINITY_BOT_QUEST_UPDATE_ADD_CREDIT_PACKET_H
#define TRINITY_BOT_QUEST_UPDATE_ADD_CREDIT_PACKET_H

#include "ObjectGuid.h"
#include "WorldPacket.h"

namespace Playerbots::PacketParse
{
// Fields mirrored from WorldPackets::Quest::QuestUpdateAddCredit::Write() — module-first dual reader.
// Registry signal keeps AC name "quest update add kill" (Midnight opcode is ADD_CREDIT).
struct QuestUpdateAddCreditPayload
{
    ObjectGuid VictimGUID;
    int32 QuestID = 0;
    int32 ObjectID = 0;
    uint16 Count = 0;
    uint16 Required = 0;
    uint32 ObjectiveType = 0;
};

// Returns true on Layer-1 success (full consume, no exception). Does not perform Layer 2.
bool TryReadQuestUpdateAddCredit(WorldPacket const& packet, QuestUpdateAddCreditPayload& out);
}

#endif

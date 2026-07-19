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

#ifndef TRINITY_BOT_QUEST_CONFIRM_ACCEPT_PACKET_H
#define TRINITY_BOT_QUEST_CONFIRM_ACCEPT_PACKET_H

#include "ObjectGuid.h"
#include "WorldPacket.h"
#include <string>

namespace Playerbots::PacketParse
{
// Fields mirrored from WorldPackets::Quest::QuestConfirmAcceptResponse::Write() —
// module-first dual reader (SizedString BitsSize<10> QuestTitle).
struct QuestConfirmAcceptPayload
{
    uint32 QuestID = 0;
    ObjectGuid InitiatedBy;
    std::string QuestTitle;
};

// Returns true on Layer-1 success (full consume, no exception). Does not perform Layer 2.
bool TryReadQuestConfirmAccept(WorldPacket const& packet, QuestConfirmAcceptPayload& out);
}

#endif

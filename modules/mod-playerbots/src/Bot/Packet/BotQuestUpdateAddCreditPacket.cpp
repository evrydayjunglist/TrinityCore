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

#include "BotQuestUpdateAddCreditPacket.h"
#include "BotPacketParse.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadQuestUpdateAddCreditBody(ByteBuffer& data, QuestUpdateAddCreditPayload& out)
{
    // Mirror WorldPackets::Quest::QuestUpdateAddCredit::Write() field order exactly.
    // Do not paste AC era questId >> entry >> available >> required.
    data >> out.VictimGUID;
    data >> out.QuestID;
    data >> out.ObjectID;
    data >> out.Count;
    data >> out.Required;
    data >> out.ObjectiveType;
}
}

bool TryReadQuestUpdateAddCredit(WorldPacket const& packet, QuestUpdateAddCreditPayload& out)
{
    QuestUpdateAddCreditPayload parsed;
    Result const result = TryReadFully(packet, "SMSG_QUEST_UPDATE_ADD_CREDIT", [&parsed](WorldPacket& copy)
    {
        ReadQuestUpdateAddCreditBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}

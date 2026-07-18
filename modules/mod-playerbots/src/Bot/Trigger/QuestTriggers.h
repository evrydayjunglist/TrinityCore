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

#ifndef TRINITY_PLAYERBOT_QUEST_TRIGGERS_H
#define TRINITY_PLAYERBOT_QUEST_TRIGGERS_H

#include "Trigger.h"

class BotPlayerbotAI;

// Enable=0 / lost-signal twin for SMSG_QUEST_CONFIRM_ACCEPT.
// Packet path uses SignalTrigger("confirm quest"); this covers PayloadParse off when
// GetSharedQuestID() is set and the sharer is the bot's master.
class QuestConfirmAcceptTrigger : public Trigger
{
public:
    explicit QuestConfirmAcceptTrigger(BotPlayerbotAI* botAI) : Trigger(botAI, "confirm quest poll") { }

    bool IsActive() override;
};

#endif

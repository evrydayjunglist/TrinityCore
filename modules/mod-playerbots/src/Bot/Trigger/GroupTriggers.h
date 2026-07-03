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

#ifndef TRINITY_PLAYERBOT_GROUP_TRIGGERS_H
#define TRINITY_PLAYERBOT_GROUP_TRIGGERS_H

#include "Trigger.h"

class BotPlayerbotAI;

// Active while the bot has a pending party invite whose leader is its own master. AC's
// equivalent is packet-driven (SMSG_GROUP_INVITE -> "group invite" handler in
// WorldPacketHandlerStrategy); this fork's bot sessions are socketless and never receive that
// outbound SMSG, so we poll TC's own Player::GetGroupInvite() state each tick instead.
class GroupInviteTrigger : public Trigger
{
public:
    explicit GroupInviteTrigger(BotPlayerbotAI* botAI) : Trigger(botAI, "group invite") { }

    bool IsActive() override;
};

#endif

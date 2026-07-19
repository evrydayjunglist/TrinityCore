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

#ifndef TRINITY_PLAYERBOT_RECEIVE_EMOTE_ACTION_H
#define TRINITY_PLAYERBOT_RECEIVE_EMOTE_ACTION_H

#include "Action.h"

class BotPlayerbotAI;

// Minimal AC "receive text emote" / "receive emote": signal wake-up + optional
// TellMaster when source == master. No ReceiveEmote matrix / EmoteStrategy / Handle* reply.
class ReceiveEmoteAction : public Action
{
public:
    ReceiveEmoteAction(BotPlayerbotAI* botAI, char const* name)
        : Action(botAI, name) { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

#endif

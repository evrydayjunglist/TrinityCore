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

#ifndef TRINITY_PLAYERBOT_TELL_MASTER_ACTIONS_H
#define TRINITY_PLAYERBOT_TELL_MASTER_ACTIONS_H

#include "Action.h"
#include <string>
#include <utility>

class BotPlayerbotAI;

// Minimal AC TellMasterAction shape: whisper a fixed string to the master.
// No AC security/facing / TellMasterNoFacing port — V1 whisper only.
class TellMasterAction : public Action
{
public:
    TellMasterAction(BotPlayerbotAI* botAI, std::string name, std::string text)
        : Action(botAI, std::move(name)), _text(std::move(text)) { }

    bool Execute(Event event) override;
    bool IsUseful() override;

private:
    std::string _text;
};

#endif

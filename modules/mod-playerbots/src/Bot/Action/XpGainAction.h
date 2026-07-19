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

#ifndef TRINITY_PLAYERBOT_XP_GAIN_ACTION_H
#define TRINITY_PLAYERBOT_XP_GAIN_ACTION_H

#include "Action.h"

class BotPlayerbotAI;

// Minimal AC "xpgain": signal wake-up + optional TellMaster "gained <Amount> xp".
// Signal and action both use "xpgain" (AC splits trigger "xpgain" → action "xp gain").
// No GiveXP re-apply, kill broadcast, or death-count reset.
class XpGainAction : public Action
{
public:
    explicit XpGainAction(BotPlayerbotAI* botAI)
        : Action(botAI, "xpgain") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

#endif

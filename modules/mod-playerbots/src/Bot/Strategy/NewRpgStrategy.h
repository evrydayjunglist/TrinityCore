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

#ifndef TRINITY_PLAYERBOT_NEW_RPG_STRATEGY_H
#define TRINITY_PLAYERBOT_NEW_RPG_STRATEGY_H

#include "Strategy.h"

class BotPlayerbotAI;

// Gate 10 — World RPG loop for masterless random bots (AC reference: mod-playerbots-master's
// "new rpg" strategy, Ai/World/Rpg/Strategy/NewRpgStrategy.cpp). Gate 10b rewired it to AC's
// actual shape: the status-update action runs as the top default and drives the NewRpgInfo
// state machine, while NewRpgStatusTrigger nodes gate exactly one per-status action (grind
// travel / wander / do quest) at low relevance. Opportunistic quest-giver pickup and the
// always-on "attack anything" kill role sit above the status machine as default actions
// (AC runs those through every RPG action / its separate grind strategy respectively).
class NewRpgStrategy : public Strategy
{
public:
    explicit NewRpgStrategy(BotPlayerbotAI* botAI) : Strategy(botAI) { }

    std::string GetName() override { return "newrpg"; }
    uint32 GetType() const override { return STRATEGY_TYPE_NONCOMBAT; }
    std::vector<NextAction> GetDefaultActions() override;
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
};

#endif

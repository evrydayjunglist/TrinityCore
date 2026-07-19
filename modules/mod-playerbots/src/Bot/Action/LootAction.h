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

#ifndef TRINITY_PLAYERBOT_LOOT_ACTION_H
#define TRINITY_PLAYERBOT_LOOT_ACTION_H

#include "Action.h"

class BotPlayerbotAI;

// Find / approach / open lootable corpses. AC OpenLootAction shape; open still uses
// Player::SendLoot (HandleLootOpcode migration is a later retirement step). Store/money/release
// are NOT done here — SMSG_LOOT_RESPONSE → StoreLootAction (Handle*Opcode).
// Finder gates on Player::isAllowedToLoot + quest-relevant slots (LootQuestItemsOnly).
class LootAction : public Action
{
public:
    explicit LootAction(BotPlayerbotAI* botAI) : Action(botAI, "loot") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

// Minimal AC "store loot": drain an acquired SMSG_LOOT_RESPONSE window via
// HandleLootMoneyOpcode / HandleAutostoreLootItemOpcode / HandleLootReleaseOpcode.
// No QueuePacket, no Player::StoreLootItem / DoLootRelease.
class StoreLootAction : public Action
{
public:
    explicit StoreLootAction(BotPlayerbotAI* botAI) : Action(botAI, "store loot") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

#endif

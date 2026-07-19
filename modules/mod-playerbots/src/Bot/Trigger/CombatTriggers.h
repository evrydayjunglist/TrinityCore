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

#ifndef TRINITY_PLAYERBOT_COMBAT_TRIGGERS_H
#define TRINITY_PLAYERBOT_COMBAT_TRIGGERS_H

#include "Trigger.h"

class BotPlayerbotAI;

// Gate 12 — minimal combat brain triggers (AI_VALUE-backed).

class HasCombatTargetTrigger : public Trigger
{
public:
    explicit HasCombatTargetTrigger(BotPlayerbotAI* botAI) : Trigger(botAI, "has combat target") { }

    bool IsActive() override;
};

class HasAttackersTrigger : public Trigger
{
public:
    explicit HasAttackersTrigger(BotPlayerbotAI* botAI) : Trigger(botAI, "has attackers") { }

    bool IsActive() override;
};

// Low health + combat/attackers, with hysteresis via ManualSetValue "is fleeing".
class FleeHealthTrigger : public Trigger
{
public:
    explicit FleeHealthTrigger(BotPlayerbotAI* botAI) : Trigger(botAI, "flee health") { }

    bool IsActive() override;
};

#endif

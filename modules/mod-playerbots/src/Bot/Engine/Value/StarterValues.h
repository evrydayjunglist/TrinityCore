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

#ifndef TRINITY_PLAYERBOT_STARTER_VALUES_H
#define TRINITY_PLAYERBOT_STARTER_VALUES_H

#include "Value.h"

class BotPlayerbotAI;

// Gate 11 starter general-value set (AC Ai/Base/Value shape; TC Unit/Player APIs).
// AC name map (closeout): "old target" → "master target"; "can melee" → "in melee range".

class CurrentTargetValue : public CalculatedValue<Unit*>
{
public:
    CurrentTargetValue(BotPlayerbotAI* botAI, std::string const name = "current target")
        : CalculatedValue<Unit*>(botAI, name) { }

    Unit* Calculate() override;
};

class MasterTargetValue : public CalculatedValue<Unit*>
{
public:
    MasterTargetValue(BotPlayerbotAI* botAI, std::string const name = "master target")
        : CalculatedValue<Unit*>(botAI, name) { }

    Unit* Calculate() override;
};

class AttackersValue : public CalculatedValue<GuidVector>
{
public:
    AttackersValue(BotPlayerbotAI* botAI, std::string const name = "attackers")
        : CalculatedValue<GuidVector>(botAI, name, 1) { }

    GuidVector Calculate() override;
};

class AttackerCountValue : public CalculatedValue<uint32>
{
public:
    AttackerCountValue(BotPlayerbotAI* botAI, std::string const name = "attacker count")
        : CalculatedValue<uint32>(botAI, name, 1) { }

    uint32 Calculate() override;
};

class HealthValue : public CalculatedValue<uint8>
{
public:
    HealthValue(BotPlayerbotAI* botAI, std::string const name = "health")
        : CalculatedValue<uint8>(botAI, name) { }

    uint8 Calculate() override;
};

class DistanceValue : public CalculatedValue<float>, public Qualified
{
public:
    // registryName is the AiObjectContext map key ("distance" or "distance::master").
    DistanceValue(BotPlayerbotAI* botAI, std::string const qualifier, std::string const registryName)
        : CalculatedValue<float>(botAI, registryName)
    {
        Qualify(qualifier.empty() ? "current target" : qualifier);
    }

    float Calculate() override;
};

class HasAuraValue : public CalculatedValue<bool>, public Qualified
{
public:
    HasAuraValue(BotPlayerbotAI* botAI, std::string const qualifier,
        std::string const name = "has aura")
        : CalculatedValue<bool>(botAI, MakeQualifiedValueName(name, qualifier))
    {
        Qualify(qualifier);
    }

    bool Calculate() override;
};

class TargetHasAuraValue : public CalculatedValue<bool>, public Qualified
{
public:
    TargetHasAuraValue(BotPlayerbotAI* botAI, std::string const qualifier,
        std::string const name = "target has aura")
        : CalculatedValue<bool>(botAI, MakeQualifiedValueName(name, qualifier))
    {
        Qualify(qualifier);
    }

    bool Calculate() override;
};

class InMeleeRangeValue : public CalculatedValue<bool>
{
public:
    InMeleeRangeValue(BotPlayerbotAI* botAI, std::string const name = "in melee range")
        : CalculatedValue<bool>(botAI, name) { }

    bool Calculate() override;
};

class IsCastingValue : public CalculatedValue<bool>
{
public:
    IsCastingValue(BotPlayerbotAI* botAI, std::string const name = "is casting")
        : CalculatedValue<bool>(botAI, name) { }

    bool Calculate() override;
};

#endif

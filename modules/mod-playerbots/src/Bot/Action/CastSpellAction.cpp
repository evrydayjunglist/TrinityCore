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

#include "CastSpellAction.h"
#include "AttackValidity.h"
#include "BotPlayerbotAI.h"
#include "CombatPositioning.h"
#include "Log.h"
#include "Map.h"
#include "Player.h"
#include "Playerbots.h"
#include "PlayerbotsConfig.h"
#include "Spell.h"
#include "SpellHistory.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Unit.h"

#include <cmath>

CastSpellAction::CastSpellAction(BotPlayerbotAI* botAI, std::string name, uint32 spellId)
    : Action(botAI, std::move(name)), _spellId(spellId)
{
}

Unit* CastSpellAction::GetSpellTarget() const
{
    if (!_botAI || !_botAI->GetAiObjectContext())
        return nullptr;

    return AI_VALUE(Unit*, "current target");
}

bool CastSpellAction::CanCastOn(Player* bot, Unit* target, SpellInfo const* spellInfo) const
{
    if (!bot || !target || !spellInfo)
        return false;

    if (!bot->IsAlive() || !bot->IsInWorld())
        return false;

    if (!bot->HasSpell(_spellId))
        return false;

    if (_botAI && _botAI->GetAiObjectContext() && AI_VALUE(bool, "is casting"))
        return false;

    // Yield while actively fleeing (Gate 12 — flee relevance 40 outranks casts).
    if (_botAI && _botAI->GetAiObjectContext() && AI_VALUE(bool, "is fleeing") &&
        AI_VALUE(uint8, "health") <= Playerbots::GetCombatFleeHealthExitPct())
        return false;

    SpellHistory* history = bot->GetSpellHistory();
    if (!history || !history->IsReady(spellInfo))
        return false;

    if (history->GetRemainingGlobalCooldown(spellInfo) > 0ms)
        return false;

    if (bot->IsSilenced(spellInfo->GetSchoolMask()))
        return false;

    if (target != bot)
    {
        // Gate 12 hostility — blocks post-duel friendly swing re-arm (BAD_TARGETS casts).
        if (!IsValidAttackTarget(bot, target))
            return false;

        if (target->GetMapId() != bot->GetMapId())
            return false;

        float maxRange = spellInfo->GetMaxRange(false, bot);
        if (maxRange <= 0.0f)
        {
            // Melee / "touch" spells — Gate 12 melee band.
            if (!bot->IsWithinMeleeRange(target))
                return false;
        }
        else
        {
            maxRange += bot->GetCombatReach() + target->GetCombatReach();
            if (!bot->IsWithinDist(target, maxRange))
                return false;
        }
    }

    return true;
}

bool CastSpellAction::IsPossible()
{
    Player* bot = GetBot();
    if (!bot || !_spellId)
        return false;

    Difficulty const difficulty = bot->GetMap() ? bot->GetMap()->GetDifficultyID() : DIFFICULTY_NONE;
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(_spellId, difficulty);
    if (!spellInfo)
        spellInfo = sSpellMgr->GetSpellInfo(_spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    Unit* target = GetSpellTarget();
    return CanCastOn(bot, target, spellInfo);
}

bool CastSpellAction::IsUseful()
{
    Player* bot = GetBot();
    if (!bot || !_spellId)
        return false;

    Difficulty const difficulty = bot->GetMap() ? bot->GetMap()->GetDifficultyID() : DIFFICULTY_NONE;
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(_spellId, difficulty);
    if (!spellInfo)
        spellInfo = sSpellMgr->GetSpellInfo(_spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    Unit* target = GetSpellTarget();
    if (!CanCastOn(bot, target, spellInfo))
        return false;

    return IsUsefulExtra(bot, target);
}

bool CastSpellAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot || !_spellId)
        return false;

    Unit* target = GetSpellTarget();
    if (!target)
        return false;

    if (ShouldPrepareCombatSwing() && target != bot)
    {
        // Never Attack() a now-friendly/invalid target (duel end) — CastSpell alone would
        // fail BAD_TARGETS but melee would already be re-armed.
        if (!IsValidAttackTarget(bot, target))
        {
            if (bot->GetVictim() == target)
                bot->AttackStop();
            return false;
        }

        if (!bot->HasInArc(float(M_PI), target))
            bot->SetFacingToObject(target);

        bot->Attack(target, true);
        ApplyCombatMovement(bot, target);
    }

    // Same public entry MountActions / creature scripts use — Spell::prepare + CheckCast.
    SpellCastResult const result = bot->CastSpell(target, _spellId);
    if (result != SPELL_CAST_OK)
    {
        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots", "CastSpellAction bot={} spell={} '{}' failed result={}",
                bot->GetName(), _spellId, GetName(), int32(result));
        return false;
    }

    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots", "CastSpellAction bot={} spell={} '{}'",
            bot->GetName(), _spellId, GetName());

    return true;
}

CastSelfBuffSpellAction::CastSelfBuffSpellAction(BotPlayerbotAI* botAI, std::string name, uint32 spellId)
    : CastSpellAction(botAI, std::move(name), spellId)
{
}

Unit* CastSelfBuffSpellAction::GetSpellTarget() const
{
    return GetBot();
}

bool CastSelfBuffSpellAction::IsUsefulExtra(Player* bot, Unit* /*target*/) const
{
    return bot && !bot->HasAura(_spellId);
}

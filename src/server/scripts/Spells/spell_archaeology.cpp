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

#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"

// 80451 - Survey
// Archaeology survey: the profession loop lives in Player::HandleArchaeologySurvey; this hook just
// runs it after the survey cast completes for a player caster.
class spell_archaeology_survey : public SpellScript
{
    void HandleAfterCast()
    {
        if (Player* player = GetCaster()->ToPlayer())
            player->HandleArchaeologySurvey();
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_archaeology_survey::HandleAfterCast);
    }
};

void AddSC_archaeology_spell_scripts()
{
    RegisterSpellScript(spell_archaeology_survey);
}

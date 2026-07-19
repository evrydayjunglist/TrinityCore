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

#include "TellCastFailedAction.h"
#include "BotPlayerbotAI.h"
#include "DBCEnums.h"
#include "Log.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "SharedDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include <sstream>

namespace
{
char const* CastFailedTellSuffix(int32 reason)
{
    switch (reason)
    {
        case SPELL_FAILED_NOT_READY:
            return "not ready";
        case SPELL_FAILED_REQUIRES_SPELL_FOCUS:
            return "requires spell focus";
        case SPELL_FAILED_REQUIRES_AREA:
            return "cannot cast here";
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS:
            return "requires item";
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS_MAINHAND:
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS_OFFHAND:
            return "requires weapon";
        case SPELL_FAILED_PREVENTED_BY_MECHANIC:
            return "interrupted";
        default:
            return "cannot cast";
    }
}
}

bool TellCastFailedAction::IsUseful()
{
    return _botAI && _botAI->GetBot() && _botAI->GetPendingCastFailed().has_value();
}

bool TellCastFailedAction::Execute(Event /*event*/)
{
    if (!_botAI)
        return false;

    std::optional<BotPlayerbotAI::PendingCastFailed> const pending = _botAI->GetPendingCastFailed();
    _botAI->ClearPendingCastFailed();
    if (!pending)
        return false;

    Player* bot = _botAI->GetBot();
    if (!bot)
        return false;

    // Signal wake-up always succeeds; TellMaster only when master present + ≥2s cast.
    if (!_botAI->HasMaster())
        return true;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(uint32(pending->SpellID), DIFFICULTY_NONE);
    if (!spellInfo || spellInfo->CalcCastTime() < 2000)
        return true;

    std::ostringstream out;
    out << "spell " << pending->SpellID << ": " << CastFailedTellSuffix(pending->Reason);
    bool const told = _botAI->TellMaster(out.str());
    if (Playerbots::GetLogLevel() >= 1)
    {
        Player* master = _botAI->GetMaster();
        TC_LOG_DEBUG("playerbots", "TellCastFailedAction bot={} master={} text='{}' ok={}",
            bot->GetName(),
            master ? master->GetName() : "none",
            out.str(),
            told ? "yes" : "no");
    }

    return true;
}

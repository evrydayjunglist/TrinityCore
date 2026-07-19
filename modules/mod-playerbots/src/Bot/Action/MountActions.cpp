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

#include "MountActions.h"
#include "BotPlayerbotAI.h"
#include "DBCEnums.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "Spell.h"
#include "SpellAuraDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellPackets.h"
#include "WorldPacket.h"
#include "WorldSession.h"

namespace
{
bool IsFlightMountSpell(SpellInfo const* spellInfo)
{
    return spellInfo->HasAura(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED)
        || spellInfo->HasAura(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED)
        || spellInfo->HasAura(SPELL_AURA_MOD_MOUNTED_FLIGHT_SPEED_ALWAYS)
        || spellInfo->HasAura(SPELL_AURA_FLY);
}

// First active learned ground mount on the bot (SPELL_AURA_MOUNTED, non-passive, non-flight).
uint32 FindGroundMountSpellId(Player* bot)
{
    for (auto const& [spellId, spellState] : bot->GetSpellMap())
    {
        if (spellState.state == PLAYERSPELL_REMOVED || !spellState.active)
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo || spellInfo->IsPassive() || !spellInfo->HasAura(SPELL_AURA_MOUNTED))
            continue;

        if (IsFlightMountSpell(spellInfo))
            continue;

        return spellId;
    }

    return 0;
}

void DismountBot(Player* bot)
{
    if (bot->isMoving())
        bot->StopMoving();

    // Same server path the client's Cancel Mount Aura button reaches.
    WorldPacket packet(CMSG_CANCEL_MOUNT_AURA);
    WorldPackets::Spells::CancelMountAura cancel(std::move(packet));
    bot->GetSession()->HandleCancelMountAuraOpcode(cancel);
}

bool TryMountBot(Player* bot)
{
    uint32 const mountSpellId = FindGroundMountSpellId(bot);
    if (!mountSpellId)
    {
        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots",
                "CheckMountStateAction bot={} mount-up skipped (no learned ground mount spell)",
                bot->GetName());
        return false;
    }

    SpellCastResult const result = bot->CastSpell(bot, mountSpellId);
    if (result != SPELL_CAST_OK)
    {
        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots",
                "CheckMountStateAction bot={} CastSpell({}) failed result={}",
                bot->GetName(), mountSpellId, int32(result));
        return false;
    }

    return true;
}
}

bool CheckMountStateAction::IsUseful()
{
    if (!_botAI)
        return false;

    Player* bot = _botAI->GetBot();
    Player* master = _botAI->GetMaster();
    if (!bot || !master || !bot->IsAlive() || !master->IsAlive())
        return false;

    // Cheap AC usefulness spirit — no GetMapHeight / remote terrain.
    if (bot->IsInFlight() || bot->GetVehicle() || !bot->IsOutdoors())
        return false;

    if (bot->HasAuraType(SPELL_AURA_TRANSFORM) && bot->IsInDisallowedMountForm())
        return false;

    bool const masterMounted = master->IsMounted();
    bool const botMounted = bot->IsMounted();
    return masterMounted != botMounted;
}

bool CheckMountStateAction::Execute(Event /*event*/)
{
    if (!_botAI)
        return false;

    Player* bot = _botAI->GetBot();
    Player* master = _botAI->GetMaster();
    if (!bot || !master)
        return false;

    bool const masterMounted = master->IsMounted();
    bool const botMounted = bot->IsMounted();

    if (!masterMounted && botMounted)
    {
        DismountBot(bot);
        bool const ok = !bot->IsMounted();
        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots", "CheckMountStateAction bot={} master={} dismount={}",
                bot->GetName(), master->GetName(), ok ? "yes" : "no");
        return ok;
    }

    if (masterMounted && !botMounted)
    {
        bool const ok = TryMountBot(bot);
        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots", "CheckMountStateAction bot={} master={} mount-up={}",
                bot->GetName(), master->GetName(), ok ? "yes" : "no");
        return ok;
    }

    return false;
}

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

#include "DeathActions.h"
#include "BotPlayerbotAI.h"
#include "Corpse.h"
#include "GameTime.h"
#include "Log.h"
#include "MotionMaster.h"
#include "MoveSpline.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "SafeMovement.h"
#include "SpellAuraDefines.h"

namespace
{
// The base death predicate shared by all three actions: a real, in-world bot that is not alive and
// is NOT a master-alt (V1 is solo random/newrpg bots only — handoff §2). Mirrors the core handlers'
// IsAlive() gate exactly (a released ghost is !IsAlive()), rather than isDead()/getDeathState()
// shorthand, so the packetless replication tracks the handler guard set faithfully.
bool IsDeadSoloBot(Player* bot, BotPlayerbotAI* botAI)
{
    return bot && bot->IsInWorld() && !bot->IsAlive() && botAI && !botAI->HasMaster();
}
} // namespace

// --- 1) Release spirit ---------------------------------------------------------------------------

bool ReleaseSpiritAction::IsUseful()
{
    Player* bot = GetBot();
    if (!IsDeadSoloBot(bot, _botAI))
    {
        _firstSeenDeadAt = 0; // not in the pre-release window — re-arm for the next death
        return false;
    }

    // HandleRepopRequest guard: body already released (ghost) or resurrection prevented.
    if (bot->HasPlayerFlag(PLAYER_FLAGS_GHOST))
    {
        _firstSeenDeadAt = 0;
        return false;
    }

    if (bot->HasAuraType(SPELL_AURA_PREVENT_RESURRECTION))
        return false;

    // Hold for a short, configurable delay after death so the release reads lifelike (0 = instant).
    // The handler's JUST_DIED->KillPlayer finalize (replicated in Execute) covers the same-tick race
    // regardless of this pause.
    time_t const now = GameTime::GetGameTime();
    if (_firstSeenDeadAt == 0)
        _firstSeenDeadAt = now;

    return uint32(now - _firstSeenDeadAt) >= Playerbots::GetRpgDeathReleaseDelaySeconds();
}

bool ReleaseSpiritAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    // Replicate WorldSession::HandleRepopRequest packetlessly.
    // Finalize a same-tick death first (the exact JUST_DIED race the handler guards).
    if (bot->getDeathState() == JUST_DIED)
        bot->KillPlayer();

    bot->RemovePet(nullptr, PET_SAVE_NOT_IN_SLOT, true);
    bot->BuildPlayerRepop();   // spawns the Corpse at the death spot, sets PLAYER_FLAGS_GHOST
    bot->RepopAtGraveyard();   // teleports the ghost to the core's data-first nearest graveyard

    _firstSeenDeadAt = 0;
    _botAI->GetRpgStatistics().deaths++;
    TC_LOG_DEBUG("playerbots", "[New RPG] {} released spirit (repop at graveyard)", bot->GetName());

    return true;
}

// --- 2) Run to corpse ----------------------------------------------------------------------------

bool RunToCorpseAction::IsUseful()
{
    Player* bot = GetBot();
    if (!IsDeadSoloBot(bot, _botAI))
        return false;

    // Must have released first (ghost) and have a corpse to run to.
    if (!bot->HasPlayerFlag(PLAYER_FLAGS_GHOST))
        return false;

    Corpse* corpse = bot->GetCorpse();
    if (!corpse)
        return false;

    // Only useful while the corpse is out of reclaim range; a small buffer under the radius so the
    // handoff hands off cleanly to ReclaimCorpseAction once the ghost is close enough.
    return !corpse->IsWithinDistInMap(bot, CORPSE_RECLAIM_RADIUS - 5.0f, true);
}

bool RunToCorpseAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    Corpse* corpse = bot->GetCorpse();
    if (!corpse)
        return false;

    WorldLocation const& dest = bot->GetCorpseLocation();

    // Bounded stranding fallback (handoff §4a): if the ghost has spent longer than the configured
    // timeout unable to walk back (e.g. corpse across genuinely unwalkable terrain / water, which
    // the SafeMovement water rejection refuses), teleport to the corpse once and let
    // ReclaimCorpseAction fire next tick. This is the only non-walking path in V1 — log it loudly.
    time_t const ghostSeconds = GameTime::GetGameTime() - corpse->GetGhostTime();
    if (ghostSeconds >= time_t(Playerbots::GetRpgDeathCorpseRunTimeoutSeconds()))
    {
        TC_LOG_INFO("playerbots", "[New RPG] {} could not walk to its corpse within {}s — teleporting to corpse (stranding fallback)",
            bot->GetName(), Playerbots::GetRpgDeathCorpseRunTimeoutSeconds());
        return bot->TeleportTo(dest.GetMapId(), dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ(),
            bot->GetOrientation());
    }

    // Let a previously committed long spline finish before recomputing (AC MoveFarTo pattern):
    // re-pathing mid-walk from a new position yields a different partial-route endpoint and makes
    // the bot oscillate around obstacles instead of walking around them.
    if (!bot->movespline->Finalized())
    {
        G3D::Vector3 const end = bot->movespline->FinalDestination();
        if (bot->GetExactDist(end.x, end.y, end.z) > 10.0f)
            return true;
    }

    // Walk toward the corpse via the validated-path contract (slope/path gate honoured; the
    // minProgress guard rejects a zero-progress partial route the same way MoveFarTo does).
    if (TryMoveTowardValidatedPoint(bot, dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ(), 5.0f))
        return true;

    // No walkable progress this tick — stand still and try again next tick (the timeout fallback
    // above is the eventual backstop for a genuinely unreachable corpse).
    return false;
}

// --- 3) Reclaim corpse ---------------------------------------------------------------------------

bool ReclaimCorpseAction::IsUseful()
{
    Player* bot = GetBot();
    if (!IsDeadSoloBot(bot, _botAI))
        return false;

    // Replicate the WorldSession::HandleReclaimCorpse guard set exactly.
    if (bot->InArena())
        return false;

    if (!bot->HasPlayerFlag(PLAYER_FLAGS_GHOST))
        return false; // must have released first

    Corpse* corpse = bot->GetCorpse();
    if (!corpse)
        return false;

    // Reclaim-delay window (~30s+). Stay NOT-useful (never spam Execute) until it elapses.
    if (time_t(corpse->GetGhostTime() + bot->GetCorpseReclaimDelay(corpse->GetType() == CORPSE_RESURRECTABLE_PVP))
            > time_t(GameTime::GetGameTime()))
        return false;

    if (!corpse->IsWithinDistInMap(bot, CORPSE_RECLAIM_RADIUS, true))
        return false;

    return true;
}

bool ReclaimCorpseAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    // Replicate WorldSession::HandleReclaimCorpse packetlessly: resurrect in place (50% HP, no
    // sickness; 100% in a battleground exactly as the handler does — BG is never hit in V1) and
    // turn the corpse into bones.
    bot->ResurrectPlayer(bot->InBattleground() ? 1.0f : 0.5f);
    bot->SpawnCorpseBones();

    // Drop any leftover selection so the newly-alive bot resumes the interact band cleanly.
    bot->SetSelection(ObjectGuid::Empty);

    _botAI->GetRpgStatistics().revived++;
    TC_LOG_DEBUG("playerbots", "[New RPG] {} reclaimed corpse and resurrected", bot->GetName());

    return true;
}

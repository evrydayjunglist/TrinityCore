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

#ifndef TRINITY_PLAYERBOT_DEATH_ACTIONS_H
#define TRINITY_PLAYERBOT_DEATH_ACTIONS_H

#include "Action.h"
#include <ctime>

class BotPlayerbotAI;

// Bot death handling V1 — the corpse run (see playerbots-bot-death-corpse-run-handoff.md).
//
// Three death-state-gated actions that sit in NewRpgStrategy's always-on band ABOVE the interact
// band (relevance 55/54/53 > "quest giver" 30). Each is IsUseful() only while the bot is in the
// matching death phase, so exactly one is live at a time and only while the bot is dead — a live
// bot's behaviour is completely unchanged. They mirror the SHAPE of AC mod-playerbots'
// DeadStrategy (ReleaseSpiritAction -> ReviveFromCorpseAction) but are TC-native and packetless:
// bot sessions have no inbound packet queue, so instead of feeding synthetic CMSG_REPOP_REQUEST /
// CMSG_RECLAIM_CORPSE packets the way AC does, each action replicates the guard set of the modern
// core handler (WorldSession::HandleRepopRequest / HandleReclaimCorpse in MiscHandler.cpp) and
// calls the exact public Player methods those thin handlers wrap — the same precedent as
// TalkToQuestNpcAction (Player::TalkedToCreature) and LootAction open (Player::SendLoot).
//
// Scope: V1 handles SOLO random/newrpg bots only. Spirit-healer res, other-bot/player res,
// self-res (soulstone/reincarnation), death-count teleport, and master-alt/group death are all
// NYI (handoff §2/§9). Master-alt bots never get the "newrpg" strategy (they run follow/attack),
// so the death band cannot reach them; every IsUseful() below also short-circuits on HasMaster()
// as belt-and-suspenders so it never fires on a grouped/master-alt bot even if wiring changes.

class Corpse;
class Player;

// 1) Release spirit — dead but body not yet released. Replicates HandleRepopRequest:
//    RemovePet -> BuildPlayerRepop (spawns the corpse, sets PLAYER_FLAGS_GHOST) -> RepopAtGraveyard
//    (teleports the ghost to the core's data-first nearest graveyard). Holds for a short,
//    configurable RpgDeathReleaseDelaySeconds after death so it reads lifelike (the handler's
//    JUST_DIED->KillPlayer finalize covers the same-tick death race regardless).
class ReleaseSpiritAction : public Action
{
public:
    explicit ReleaseSpiritAction(BotPlayerbotAI* botAI) : Action(botAI, "release spirit") { }

    bool Execute(Event event) override;
    bool IsUseful() override;

private:
    // Timestamp (server game time, seconds) the bot was first seen dead-and-not-yet-a-ghost, so the
    // release delay is measured without any core death-onset hook. Reset to 0 whenever the bot is
    // alive or already a ghost, so the next death re-arms it cleanly.
    time_t _firstSeenDeadAt = 0;
};

// 2) Run to corpse — released ghost whose corpse is farther than the reclaim radius. Walks toward
//    the bot's own GetCorpseLocation() via the SafeMovement validated-path contract (slope/path
//    gate honoured, same as the interact band). Bounded stranding fallback: if the ghost cannot
//    walk back within RpgDeathCorpseRunTimeoutSeconds, teleport to the corpse once and let
//    ReclaimCorpseAction fire next tick (the only non-walking path in V1).
class RunToCorpseAction : public Action
{
public:
    explicit RunToCorpseAction(BotPlayerbotAI* botAI) : Action(botAI, "run to corpse") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

// 3) Reclaim corpse — released ghost standing on its corpse past the reclaim delay. Replicates
//    every HandleReclaimCorpse guard (dead, not arena, ghost, has corpse, reclaim-delay window
//    elapsed, within CORPSE_RECLAIM_RADIUS) then ResurrectPlayer + SpawnCorpseBones. IsUseful()
//    stays FALSE during the ~30s reclaim-delay window (no retry-loop / no per-tick Execute no-op).
class ReclaimCorpseAction : public Action
{
public:
    explicit ReclaimCorpseAction(BotPlayerbotAI* botAI) : Action(botAI, "reclaim corpse") { }

    bool Execute(Event event) override;
    bool IsUseful() override;
};

#endif

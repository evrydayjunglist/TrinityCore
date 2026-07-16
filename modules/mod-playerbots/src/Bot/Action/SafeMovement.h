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

#ifndef TRINITY_PLAYERBOT_SAFE_MOVEMENT_H
#define TRINITY_PLAYERBOT_SAFE_MOVEMENT_H

class Player;

// Shared movement-target validation for bot actions (Wander/Grind/QuestGiver), mirroring AC
// mod-playerbots' NewRpgBaseAction::MoveRandomNear/MoveFarTo pattern: never hand a raw XY(Z)
// guess straight to MotionMaster::MovePoint. Instead run a real mmap PathGenerator query first,
// reject shortcut/no-path results (the exact case that walks a bot in a straight line through
// terrain on a steep incline instead of following the navmesh), reject any corridor segment
// steeper than a configurable fork-native slope ceiling (`Playerbots.MaxWalkableSlopeDegrees` —
// not an AC port, AC has no equivalent check; the mmap navmesh's own 55-degree "normal ground"
// cutoff alone still let a bot climb terrain a human watching wouldn't call walkable), snap to the
// path's own validated ground endpoint, and refuse water. See
// playerbots-bot-wander-ground-clip-handoff.md for the AC comparison and root-cause writeup —
// reimplemented with this fork's own TC-native PathGenerator/PhaseShift APIs, not ported AC `Map`
// extensions (those don't exist on this core).
//
// Returns true and issues MotionMaster::MovePoint(0, ...) at the validated destination if one was
// found; returns false (no move issued) if the requested point has no real navmesh route, the
// route is too steep, or the bot should not go there (e.g. it's underwater) — callers should treat
// false as "stand still this tick", not retry the same raw coordinates.
bool TryMoveToValidatedPoint(Player* bot, float x, float y, float z);

// Gate 10b variant for far destinations (MoveFarTo legs, AC: NewRpgBaseAction::MoveFarTo's
// "endDistToDest + 5.0f < disToDest" progress guard): same validation contract as
// TryMoveToValidatedPoint, but additionally requires the committed (possibly partial-route)
// endpoint to close the gap toward (x,y,z) by at least minProgress yards. Rejecting
// zero-progress PATHFIND_INCOMPLETE endpoints lets the caller fall back to cone sampling
// instead of burning ticks "moving" to its own feet.
bool TryMoveTowardValidatedPoint(Player* bot, float x, float y, float z, float minProgress);

// Gate for continuous movement generators (MotionMaster::MoveChase/MoveFollow) that manage their
// own re-pathing every tick and issue no MovePoint themselves — TryMoveToValidatedPoint's
// contract doesn't apply directly since there's nothing here to commit. Callers (combat chase,
// master-alt follow) should check this before starting/continuing to chase or follow and refuse
// (stand down, same "not this tick" contract as the rest of SafeMovement) rather than calling
// MoveChase/MoveFollow when the path to the target's current position is unwalkable — otherwise
// ChaseMovementGenerator/FollowMovementGenerator build their own PathGenerator internally with
// only the engine's default 55-degree NAV_GROUND filter and no slope ceiling on top, which is
// exactly the gap that let a bot climb an unrealistic incline while chasing/following instead of
// wandering. Deliberately skips the water rejection the MovePoint-committing checks above apply
// (aquatic combat/follow is normal) and only re-validates on demand — it does not itself track or
// interrupt an already-running chase/follow generator.
bool IsApproachPathWalkable(Player* bot, float x, float y, float z);

#endif

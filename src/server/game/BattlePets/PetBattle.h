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

#ifndef TRINITYCORE_PET_BATTLE_H
#define TRINITYCORE_PET_BATTLE_H

#include "BattlePetPackets.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <vector>

class WorldSession;

namespace BattlePets
{
// V1 wild PvE vertical slice (Midnight MoP wild footprint). One battle per session.
// Structural lead from BfaCore Reforged; wire from Midnight 12.0.7 retail sniffs.

enum class PetBattleStatus : uint8
{
    Creation,               // request accepted, waiting for the initial REPLACE_FRONT_PET
    Running,                // rounds are being exchanged
    WaitingReplacement,     // player front pet died, waiting for REPLACE_FRONT_PET
    PendingFinish,          // FINAL_ROUND sent, waiting for CMSG_PET_BATTLE_FINAL_NOTIFY
    Finished
};

enum PetBattleInputMoveType : uint32
{
    PET_BATTLE_INPUT_MOVE_FORFEIT = 0,
    PET_BATTLE_INPUT_MOVE_ABILITY = 1,
    PET_BATTLE_INPUT_MOVE_PASS    = 2,
    PET_BATTLE_INPUT_MOVE_TRAP    = 3
};

struct PetBattleCombatantAbility
{
    uint32 AbilityID = 0;
    uint8 SlotIndex = 0;
    int16 CooldownRemaining = 0;
};

struct PetBattleCombatant
{
    ObjectGuid JournalGuid;     // empty for the wild pet
    uint32 SpeciesID = 0;
    uint32 DisplayID = 0;
    uint16 Level = 0;
    uint16 InitialLevel = 0;
    uint16 Xp = 0;
    uint16 BreedQuality = 0;
    int32 Health = 0;
    int32 MaxHealth = 0;
    int32 Power = 0;
    int32 Speed = 0;
    int8 PetType = 0;
    uint8 Pboid = 0;
    uint8 TeamIndex = 0;
    uint8 SlotIndex = 0;
    bool IsWild = false;
    bool SeenAction = false;
    bool AwardedXP = false;
    std::vector<PetBattleCombatantAbility> Abilities;

    bool IsAlive() const { return Health > 0; }
};

class PetBattle
{
public:
    explicit PetBattle(WorldSession* owner);
    PetBattle(PetBattle const&) = delete;
    PetBattle& operator=(PetBattle const&) = delete;

    // Build the two teams from the player journal + wild creature. Returns false if ineligible.
    bool SetupWild(ObjectGuid wildGuid, WorldPackets::BattlePet::PetBattleLocation const& location);

    void Start();
    void HandleReplaceFrontPet(uint8 frontPet);
    void HandleInput(WorldPackets::BattlePet::PetBattleInput const& input);
    void HandleQuit();
    void HandleFinalNotify();

    bool IsFinished() const { return _status == PetBattleStatus::Finished; }

private:
    PetBattleCombatant* GetActivePlayerPet();
    PetBattleCombatant* GetActiveWildPet();

    void BuildPetUpdate(PetBattleCombatant const& combatant, WorldPackets::BattlePet::PetBattlePetUpdate& out) const;
    void SendInitialUpdate();
    void SendFirstRound();
    void SendRoundResult(std::vector<WorldPackets::BattlePet::PetBattleEffect> effects, std::vector<uint8> petsDied, bool awaitingReplacement);

    void ResolveRound(uint32 playerAbilityId, bool playerPasses, std::vector<WorldPackets::BattlePet::PetBattleEffect>& effects, std::vector<uint8>& petsDied);
    uint32 PickWildAbility(PetBattleCombatant const& wild) const;
    int32 ApplyAbilityDamage(PetBattleCombatant const& caster, PetBattleCombatant& target, uint32 abilityId, std::vector<WorldPackets::BattlePet::PetBattleEffect>& effects);

    void FinishBattle(bool playerWon, bool abandoned);
    void WriteJournalResults(bool playerWon);

    WorldSession* _owner;
    PetBattleStatus _status = PetBattleStatus::Creation;
    ObjectGuid _wildCreatureGuid;
    uint32 _wildCreatureId = 0;
    uint32 _round = 0;
    uint8 _playerActive = 0;
    bool _finalPlayerWon = false;
    bool _finalAbandoned = false;
    WorldPackets::BattlePet::PetBattleLocation _location;
    std::vector<PetBattleCombatant> _playerTeam;
    std::vector<PetBattleCombatant> _wildTeam;
};
}

#endif // TRINITYCORE_PET_BATTLE_H

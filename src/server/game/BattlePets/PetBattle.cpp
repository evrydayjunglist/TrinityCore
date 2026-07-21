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

#include "PetBattle.h"
#include "BattlePetMgr.h"
#include "Creature.h"
#include "DB2Stores.h"
#include "DB2Structure.h"
#include "GameTables.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Util.h"
#include "WorldSession.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <unordered_map>
#include <unordered_set>

namespace BattlePets
{
namespace
{
constexpr uint8 PET_BATTLE_WILD_PBOID = 3;

// team input flags (BfaCore PetBattleTeamInputFlags)
constexpr uint8 PET_BATTLE_INPUT_FLAG_SELECT_NEW_PET = 0x08;

// effect / target types (BfaCore PetBattleEffectTargetType / ePetBattleEvents)
// Midnight PB-W1 FIRST_ROUND hex uses EffectType 0 with FRONT_PET targets for the
// initial lock-in (BfaCore names that PET_SWAP=4 on older builds — do not invent 4 here).
constexpr uint8 PET_BATTLE_EFFECT_TYPE_FRONT_PET_LOCK = 0;
constexpr uint8 PET_BATTLE_EFFECT_TARGET_FRONT_PET = 0;
constexpr uint8 PET_BATTLE_EFFECT_TARGET_PET = 3;

// abilityId -> { base damage points, damage BattlePetAbilityEffect.ID }
std::unordered_map<uint32, std::pair<int32, uint32>> const& GetAbilityDamageMap()
{
    static std::unordered_map<uint32, std::pair<int32, uint32>> damageByAbility;
    static bool built = false;
    if (built)
        return damageByAbility;

    built = true;

    std::unordered_set<uint32> damageProperties;
    for (BattlePetEffectPropertiesEntry const* properties : sBattlePetEffectPropertiesStore)
        if (properties->ParamLabel[0] && std::strncmp(properties->ParamLabel[0], "Points", 6) == 0)
            damageProperties.insert(properties->ID);

    std::unordered_map<uint32, std::vector<BattlePetAbilityTurnEntry const*>> turnsByAbility;
    for (BattlePetAbilityTurnEntry const* turn : sBattlePetAbilityTurnStore)
        turnsByAbility[turn->BattlePetAbilityID].push_back(turn);

    std::unordered_map<uint32, std::vector<BattlePetAbilityEffectEntry const*>> effectsByTurn;
    for (BattlePetAbilityEffectEntry const* effect : sBattlePetAbilityEffectStore)
        effectsByTurn[effect->BattlePetAbilityTurnID].push_back(effect);

    for (auto& [abilityId, turns] : turnsByAbility)
    {
        std::ranges::sort(turns, {}, &BattlePetAbilityTurnEntry::OrderIndex);

        bool found = false;
        for (BattlePetAbilityTurnEntry const* turn : turns)
        {
            auto itr = effectsByTurn.find(turn->ID);
            if (itr == effectsByTurn.end())
                continue;

            std::vector<BattlePetAbilityEffectEntry const*> effects = itr->second;
            std::ranges::sort(effects, {}, &BattlePetAbilityEffectEntry::OrderIndex);

            for (BattlePetAbilityEffectEntry const* effect : effects)
            {
                if (damageProperties.contains(effect->BattlePetEffectPropertiesID))
                {
                    damageByAbility[abilityId] = { effect->Param[0], effect->ID };
                    found = true;
                    break;
                }
            }

            if (found)
                break;
        }
    }

    return damageByAbility;
}

std::vector<PetBattleCombatantAbility> PickAbilities(uint32 speciesId, uint16 level)
{
    struct SlotChoice
    {
        uint32 AbilityID = 0;
        uint8 RequiredLevel = 0;
        bool Set = false;
    };

    std::array<SlotChoice, 3> slots{};
    for (BattlePetSpeciesXAbilityEntry const* xAbility : sBattlePetSpeciesXAbilityStore)
    {
        if (uint32(xAbility->BattlePetSpeciesID) != speciesId)
            continue;

        if (xAbility->RequiredLevel > level)
            continue;

        if (xAbility->SlotEnum < 0 || xAbility->SlotEnum > 2)
            continue;

        SlotChoice& choice = slots[xAbility->SlotEnum];
        if (!choice.Set || xAbility->RequiredLevel >= choice.RequiredLevel)
        {
            choice.AbilityID = xAbility->BattlePetAbilityID;
            choice.RequiredLevel = xAbility->RequiredLevel;
            choice.Set = true;
        }
    }

    // NYI: Ability.Cooldown / CooldownRemaining tracking across rounds.
    std::vector<PetBattleCombatantAbility> abilities;
    for (uint8 slot = 0; slot < 3; ++slot)
        if (slots[slot].Set)
            abilities.push_back({ slots[slot].AbilityID, slot, int16(0) });

    return abilities;
}

// owner-approved stub: XP amount from BfaCore BattlePetInstance::GetXPEarn (lead only).
// Wire flag AwardedXP follows PB-W2/W4 (SeenAction && !abandoned); amount not re-verified
// against Midnight sniff XP deltas yet.
uint32 GetXPEarn(uint16 killerLevel, uint16 targetLevel)
{
    int32 levelDiff = int32(targetLevel) - int32(killerLevel);
    if (levelDiff < -4 || levelDiff > 2)
        return 0;

    return uint32((killerLevel + 9) * (levelDiff + 5));
}
}

PetBattle::PetBattle(WorldSession* owner) : _owner(owner) { }

bool PetBattle::SetupWild(ObjectGuid wildGuid, WorldPackets::BattlePet::PetBattleLocation const& location)
{
    Player* player = _owner->GetPlayer();
    if (!player)
        return false;

    BattlePetMgr* mgr = _owner->GetBattlePetMgr();
    if (!mgr || !mgr->IsBattlePetSystemEnabled())
        return false;

    Creature* wild = ObjectAccessor::GetCreature(*player, wildGuid);
    if (!wild || !wild->IsWildBattlePet())
        return false;

    BattlePetSpeciesEntry const* wildSpecies = BattlePetMgr::GetBattlePetSpeciesByCreature(wild->GetEntry());
    if (!wildSpecies)
        return false;

    // Build player team from battle slots 0-2 (skip empty / locked / dead).
    for (uint8 slot = 0; slot < AsUnderlyingType(BattlePetSlot::Count); ++slot)
    {
        WorldPackets::BattlePet::BattlePetSlot* battleSlot = mgr->GetSlot(BattlePetSlot(slot));
        if (!battleSlot || battleSlot->Locked || battleSlot->Pet.Guid.IsEmpty())
            continue;

        BattlePet* journalPet = mgr->GetPet(battleSlot->Pet.Guid);
        if (!journalPet)
            continue;

        if (journalPet->PacketInfo.Health == 0)
            continue;

        BattlePetSpeciesEntry const* species = sBattlePetSpeciesStore.LookupEntry(journalPet->PacketInfo.Species);
        if (!species)
            continue;

        PetBattleCombatant combatant;
        combatant.JournalGuid = journalPet->PacketInfo.Guid;
        combatant.SpeciesID = journalPet->PacketInfo.Species;
        combatant.DisplayID = journalPet->PacketInfo.DisplayID;
        combatant.Level = journalPet->PacketInfo.Level;
        combatant.InitialLevel = journalPet->PacketInfo.Level;
        combatant.Xp = journalPet->PacketInfo.Exp;
        combatant.BreedQuality = journalPet->PacketInfo.Quality;
        combatant.Health = int32(journalPet->PacketInfo.Health);
        combatant.MaxHealth = int32(journalPet->PacketInfo.MaxHealth);
        combatant.Power = int32(journalPet->PacketInfo.Power);
        combatant.Speed = int32(journalPet->PacketInfo.Speed);
        combatant.PetType = species->PetTypeEnum;
        combatant.TeamIndex = 0;
        combatant.SlotIndex = slot;
        combatant.Pboid = uint8(_playerTeam.size());
        combatant.Abilities = PickAbilities(combatant.SpeciesID, combatant.Level);
        _playerTeam.push_back(std::move(combatant));
    }

    if (_playerTeam.empty())
        return false;

    // Build the single wild pet from the creature species.
    uint16 wildLevel = uint16(wild->GetWildBattlePetLevel());
    if (!wildLevel)
        wildLevel = 1;

    BattlePet wildPet;
    wildPet.PacketInfo.Species = wildSpecies->ID;
    wildPet.PacketInfo.Breed = BattlePetMgr::RollPetBreed(wildSpecies->ID);
    wildPet.PacketInfo.Quality = AsUnderlyingType(BattlePetMgr::GetDefaultPetQuality(wildSpecies->ID));
    wildPet.PacketInfo.Level = wildLevel;
    wildPet.CalculateStats();

    PetBattleCombatant wildCombatant;
    wildCombatant.SpeciesID = wildSpecies->ID;
    wildCombatant.DisplayID = BattlePetMgr::SelectPetDisplay(wildSpecies);
    wildCombatant.Level = wildLevel;
    wildCombatant.InitialLevel = wildLevel;
    wildCombatant.BreedQuality = wildPet.PacketInfo.Quality;
    wildCombatant.Health = int32(wildPet.PacketInfo.MaxHealth);
    wildCombatant.MaxHealth = int32(wildPet.PacketInfo.MaxHealth);
    wildCombatant.Power = int32(wildPet.PacketInfo.Power);
    wildCombatant.Speed = int32(wildPet.PacketInfo.Speed);
    wildCombatant.PetType = wildSpecies->PetTypeEnum;
    wildCombatant.TeamIndex = 1;
    wildCombatant.SlotIndex = 0;
    wildCombatant.Pboid = PET_BATTLE_WILD_PBOID;
    wildCombatant.IsWild = true;
    wildCombatant.Abilities = PickAbilities(wildCombatant.SpeciesID, wildCombatant.Level);
    _wildTeam.push_back(std::move(wildCombatant));

    _wildCreatureGuid = wildGuid;
    _wildCreatureId = wild->GetEntry();
    _location = location;
    return true;
}

void PetBattle::Start()
{
    // Echo the finalize-location, then push the initial full update. The client answers
    // with CMSG_PET_BATTLE_REPLACE_FRONT_PET, which drives us into the first round.
    WorldPackets::BattlePet::PetBattleFinalizeLocation finalize;
    finalize.Location = _location;
    _owner->SendPacket(finalize.Write());

    SendInitialUpdate();
    _status = PetBattleStatus::Creation;
}

PetBattleCombatant* PetBattle::GetActivePlayerPet()
{
    if (_playerActive < _playerTeam.size())
        return &_playerTeam[_playerActive];

    return nullptr;
}

PetBattleCombatant* PetBattle::GetActiveWildPet()
{
    if (!_wildTeam.empty())
        return &_wildTeam[0];

    return nullptr;
}

void PetBattle::BuildPetUpdate(PetBattleCombatant const& combatant, WorldPackets::BattlePet::PetBattlePetUpdate& out) const
{
    out.BattlePetGUID = combatant.JournalGuid;
    out.SpeciesID = combatant.SpeciesID;
    out.DisplayID = combatant.DisplayID;
    out.CollarID = 0;
    out.Level = combatant.Level;
    out.Xp = combatant.Xp;
    out.CurHealth = std::max<int32>(0, combatant.Health);
    out.MaxHealth = combatant.MaxHealth;
    out.Power = combatant.Power;
    out.Speed = combatant.Speed;
    out.NpcTeamMemberID = 0;
    out.BreedQuality = combatant.BreedQuality;
    out.StatusFlags = 0;
    out.Slot = combatant.SlotIndex;

    for (PetBattleCombatantAbility const& ability : combatant.Abilities)
    {
        WorldPackets::BattlePet::PetBattleActiveAbility active;
        active.AbilityID = int32(ability.AbilityID);
        active.CooldownRemaining = ability.CooldownRemaining;
        active.LockdownRemaining = 0;
        active.AbilityIndex = ability.SlotIndex;
        active.Pboid = combatant.Pboid;
        out.Abilities.push_back(active);
    }

    // NYI: full species/breed BattlePetState table. V1 emits only Power / Stamina / Speed
    // plus state 40 (slice marker observed in INITIAL_UPDATE).
    out.States.push_back({ STATE_STAT_POWER, combatant.Power });
    out.States.push_back({ STATE_STAT_STAMINA, combatant.MaxHealth });
    out.States.push_back({ STATE_STAT_SPEED, combatant.Speed });
    out.States.push_back({ 40u, 5 });
}

void PetBattle::SendInitialUpdate()
{
    Player* player = _owner->GetPlayer();

    WorldPackets::BattlePet::PetBattleInitialUpdate packet;
    WorldPackets::BattlePet::PetBattleFullUpdate& msg = packet.MsgData;

    // Player team (participant 0)
    WorldPackets::BattlePet::PetBattlePlayerUpdate& playerUpdate = msg.Players[0];
    playerUpdate.CharacterID = player->GetGUID();
    playerUpdate.TrapAbilityID = 427; // wild-battle trap ability (retail sniff)
    playerUpdate.TrapStatus = 4;
    playerUpdate.RoundTimeSecs = 0;
    playerUpdate.FrontPet = int8(_playerActive);
    playerUpdate.InputFlags = 6;
    for (PetBattleCombatant const& combatant : _playerTeam)
    {
        WorldPackets::BattlePet::PetBattlePetUpdate petUpdate;
        BuildPetUpdate(combatant, petUpdate);
        playerUpdate.Pets.push_back(std::move(petUpdate));
    }

    // Wild team (participant 1)
    WorldPackets::BattlePet::PetBattlePlayerUpdate& wildUpdate = msg.Players[1];
    wildUpdate.CharacterID = ObjectGuid::Empty;
    wildUpdate.TrapAbilityID = 0;
    wildUpdate.TrapStatus = 2;
    wildUpdate.RoundTimeSecs = 0;
    wildUpdate.FrontPet = 0;
    wildUpdate.InputFlags = 6;
    for (PetBattleCombatant const& combatant : _wildTeam)
    {
        WorldPackets::BattlePet::PetBattlePetUpdate petUpdate;
        BuildPetUpdate(combatant, petUpdate);
        wildUpdate.Pets.push_back(std::move(petUpdate));
    }

    msg.InitialWildPetGUID = _wildCreatureGuid;
    msg.NpcCreatureID = 0;
    msg.NpcDisplayID = 0;
    msg.CurRound = 0;
    msg.WaitingForFrontPetsMaxSecs = 30;
    msg.PvpMaxRoundTime = 30;
    msg.ForfeitPenalty = 10;
    msg.CurPetBattleState = 1;
    msg.IsPVP = false;
    msg.CanAwardXP = false; // PB-W2 INITIAL_UPDATE: CanAwardXP False (XP still via FINAL_ROUND AwardedXP)

    _owner->SendPacket(packet.Write());
}

void PetBattle::SendFirstRound()
{
    WorldPackets::BattlePet::PetBattleRound packet(SMSG_PET_BATTLE_FIRST_ROUND);
    packet.MsgData.CurRound = 0;
    packet.MsgData.NextPetBattleState = 2;
    packet.MsgData.NextInputFlags[0] = 0;
    packet.MsgData.NextTrapStatus[0] = 4;
    packet.MsgData.NextInputFlags[1] = 0;
    packet.MsgData.NextTrapStatus[1] = 2;

    // PB-W1..W6 FIRST_ROUND is always 82 B. Empty (22 B) passed REPLACE but crashed on
    // ROUND_RESULT (pbu3). Two single-target locks (60 B) crashed on FIRST_ROUND (pbu4).
    // Retail hex (constant across wild starts): effect0 with 4 FRONT_PET targets
    // petx {0,0,1,0}, empty effect1, then 18 B after PetXDied that WPP leaves unread.
    WorldPackets::BattlePet::PetBattleEffect frontLock;
    frontLock.EffectType = PET_BATTLE_EFFECT_TYPE_FRONT_PET_LOCK;
    frontLock.CasterPBOID = 0;
    for (int8 petx : { int8(0), int8(0), int8(1), int8(0) })
    {
        WorldPackets::BattlePet::PetBattleEffectTarget target;
        target.Type = PET_BATTLE_EFFECT_TARGET_FRONT_PET;
        target.Petx = petx;
        frontLock.Targets.push_back(target);
    }
    packet.MsgData.Effects.push_back(std::move(frontLock));
    packet.MsgData.Effects.emplace_back(); // empty effect1 (caster 0, no targets)

    packet.MsgData.TrailingBytes = {
        0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00,
        0x00, 0x00
    };

    _owner->SendPacket(packet.Write());
}

void PetBattle::SendRoundResult(std::vector<WorldPackets::BattlePet::PetBattleEffect> effects, std::vector<uint8> petsDied, bool awaitingReplacement)
{
    WorldPackets::BattlePet::PetBattleRound packet(SMSG_PET_BATTLE_ROUND_RESULT);
    packet.MsgData.CurRound = _round;
    packet.MsgData.NextPetBattleState = 2;
    packet.MsgData.NextInputFlags[0] = awaitingReplacement ? PET_BATTLE_INPUT_FLAG_SELECT_NEW_PET : uint8(0);
    packet.MsgData.NextTrapStatus[0] = 4;
    packet.MsgData.NextInputFlags[1] = 0;
    packet.MsgData.NextTrapStatus[1] = 2;
    packet.MsgData.Effects = std::move(effects);
    packet.MsgData.PetXDied = std::move(petsDied);
    _owner->SendPacket(packet.Write());
}

void PetBattle::SendReplacementsMade()
{
    // Announce the swap; no damage exchanged on the replacement/voluntary-swap packet
    // (PB-W4: REPLACE_FRONT_PET → REPLACEMENTS_MADE).
    WorldPackets::BattlePet::PetBattleRound packet(SMSG_PET_BATTLE_REPLACEMENTS_MADE);
    packet.MsgData.CurRound = _round;
    packet.MsgData.NextPetBattleState = 2;
    packet.MsgData.NextInputFlags[0] = 0;
    packet.MsgData.NextTrapStatus[0] = 4;
    packet.MsgData.NextInputFlags[1] = 0;
    packet.MsgData.NextTrapStatus[1] = 2;
    _owner->SendPacket(packet.Write());
}

void PetBattle::HandleReplaceFrontPet(uint8 frontPet)
{
    if (_status == PetBattleStatus::Creation)
    {
        if (frontPet < _playerTeam.size() && _playerTeam[frontPet].IsAlive())
            _playerActive = frontPet;

        _status = PetBattleStatus::Running;
        SendFirstRound();
        return;
    }

    if (_status == PetBattleStatus::WaitingReplacement)
    {
        if (frontPet < _playerTeam.size() && _playerTeam[frontPet].IsAlive())
            _playerActive = frontPet;

        _status = PetBattleStatus::Running;
        SendReplacementsMade();
        return;
    }

    // PB-W4: voluntary mid-battle REPLACE_FRONT_PET while Running → REPLACEMENTS_MADE.
    if (_status == PetBattleStatus::Running)
    {
        if (frontPet >= _playerTeam.size() || !_playerTeam[frontPet].IsAlive() || frontPet == _playerActive)
            return;

        _playerActive = frontPet;
        SendReplacementsMade();
    }
}

uint32 PetBattle::PickWildAbility(PetBattleCombatant const& wild) const
{
    std::unordered_map<uint32, std::pair<int32, uint32>> const& damageMap = GetAbilityDamageMap();

    for (PetBattleCombatantAbility const& ability : wild.Abilities)
    {
        auto itr = damageMap.find(ability.AbilityID);
        if (itr != damageMap.end() && itr->second.first > 0)
            return ability.AbilityID;
    }

    if (!wild.Abilities.empty())
        return wild.Abilities.front().AbilityID;

    return 0;
}

int32 PetBattle::ApplyAbilityDamage(PetBattleCombatant const& caster, PetBattleCombatant& target, uint32 abilityId, std::vector<WorldPackets::BattlePet::PetBattleEffect>& effects)
{
    std::unordered_map<uint32, std::pair<int32, uint32>> const& damageMap = GetAbilityDamageMap();
    auto itr = damageMap.find(abilityId);
    if (itr == damageMap.end() || itr->second.first <= 0)
        return 0; // NYI: non-damage / utility BattlePetAbilityEffect catalog (buffs, auras, weather)

    int32 points = itr->second.first;
    uint32 effectId = itr->second.second;

    // owner-approved stub: provisional V1 damage for playtest.
    // Lead: BfaCore PetBattleAbilityEffect::CalculateDamage (Power% + TypeDamageMod).
    // DB2 inputs: AbilityEffect.Param[0] (Points), Ability.PetTypeEnum, gtBattlePetTypeDamageMod.
    // NYI / not yet re-verified vs Midnight sniff HP deltas: crit, passives, flat state mods,
    // multi-effect turns. Do not treat as retail-like until Phase 2A HP-delta pass.
    int32 damage = points;
    int32 modPct = CalculatePct(caster.Power, 5);
    damage += CalculatePct(damage, modPct);

    int32 abilityPetType = caster.PetType;
    if (BattlePetAbilityEntry const* abilityEntry = sBattlePetAbilityStore.LookupEntry(abilityId))
        abilityPetType = abilityEntry->PetTypeEnum;

    float typeMod = 1.0f;
    if (GtBattlePetTypeDamageModEntry const* row = sBattlePetTypeDamageModGameTable.GetRow(uint32(abilityPetType) + 1))
        typeMod = GetBattlePetTypeDamageModForType(row, target.PetType);

    int32 typePct = int32(typeMod * 100.0f) - 100;
    damage += CalculatePct(damage, typePct);

    if (damage < 1)
        damage = 1;

    target.Health -= damage;

    WorldPackets::BattlePet::PetBattleEffect effect;
    effect.AbilityEffectID = effectId;
    effect.EffectType = 0; // set health
    effect.CasterPBOID = int8(caster.Pboid);

    WorldPackets::BattlePet::PetBattleEffectTarget effectTarget;
    effectTarget.Type = PET_BATTLE_EFFECT_TARGET_PET;
    effectTarget.Petx = int8(target.Pboid);
    effectTarget.Health = std::max<int32>(0, target.Health);
    effect.Targets.push_back(effectTarget);

    effects.push_back(std::move(effect));
    return damage;
}

void PetBattle::ResolveRound(uint32 playerAbilityId, bool playerPasses, std::vector<WorldPackets::BattlePet::PetBattleEffect>& effects, std::vector<uint8>& petsDied)
{
    PetBattleCombatant* playerPet = GetActivePlayerPet();
    PetBattleCombatant* wildPet = GetActiveWildPet();
    if (!playerPet || !wildPet)
        return;

    playerPet->SeenAction = true;
    wildPet->SeenAction = true;

    uint32 wildAbilityId = PickWildAbility(*wildPet);

    auto castPlayer = [&]()
    {
        if (!playerPasses && playerAbilityId && playerPet->IsAlive() && wildPet->IsAlive())
            ApplyAbilityDamage(*playerPet, *wildPet, playerAbilityId, effects);
    };

    auto castWild = [&]()
    {
        if (wildAbilityId && wildPet->IsAlive() && playerPet->IsAlive())
            ApplyAbilityDamage(*wildPet, *playerPet, wildAbilityId, effects);
    };

    // Higher Speed acts first. NYI: retail speed-tie ordering (V1 always prefers the player).
    if (playerPet->Speed >= wildPet->Speed)
    {
        castPlayer();
        castWild();
    }
    else
    {
        castWild();
        castPlayer();
    }

    if (!wildPet->IsAlive())
        petsDied.push_back(wildPet->Pboid);

    if (!playerPet->IsAlive())
        petsDied.push_back(playerPet->Pboid);

    ++_round;
}

void PetBattle::HandleInput(WorldPackets::BattlePet::PetBattleInput const& input)
{
    if (_status != PetBattleStatus::Running)
        return;

    if (input.MoveType == PET_BATTLE_INPUT_MOVE_FORFEIT)
    {
        FinishBattle(false, true);
        return;
    }

    // NYI: trap success/fail outcomes + capture chance math (PB-W5/W6 wire known; % not).
    // V1 treats MoveType 3 as a pass so the round loop stays playable without inventing %.
    bool playerPasses = input.MoveType == PET_BATTLE_INPUT_MOVE_PASS || input.MoveType == PET_BATTLE_INPUT_MOVE_TRAP;
    uint32 abilityId = input.MoveType == PET_BATTLE_INPUT_MOVE_ABILITY ? uint32(input.AbilityID) : 0u;

    std::vector<WorldPackets::BattlePet::PetBattleEffect> effects;
    std::vector<uint8> petsDied;
    ResolveRound(abilityId, playerPasses, effects, petsDied);

    PetBattleCombatant* wildPet = GetActiveWildPet();
    if (wildPet && !wildPet->IsAlive())
    {
        SendRoundResult(std::move(effects), std::move(petsDied), false);
        FinishBattle(true, false);
        return;
    }

    PetBattleCombatant* playerPet = GetActivePlayerPet();
    if (playerPet && !playerPet->IsAlive())
    {
        bool anyAlive = std::ranges::any_of(_playerTeam, [](PetBattleCombatant const& c) { return c.IsAlive(); });
        if (anyAlive)
        {
            SendRoundResult(std::move(effects), std::move(petsDied), true);
            _status = PetBattleStatus::WaitingReplacement;
        }
        else
        {
            SendRoundResult(std::move(effects), std::move(petsDied), false);
            FinishBattle(false, false);
        }
        return;
    }

    SendRoundResult(std::move(effects), std::move(petsDied), false);
}

void PetBattle::HandleQuit()
{
    if (_status == PetBattleStatus::Finished || _status == PetBattleStatus::PendingFinish)
        return;

    FinishBattle(false, true);
}

void PetBattle::FinishBattle(bool playerWon, bool abandoned)
{
    _finalPlayerWon = playerWon;
    _finalAbandoned = abandoned;

    WorldPackets::BattlePet::PetBattleFinalRound packet;
    WorldPackets::BattlePet::PetBattleFinalRoundData& msg = packet.MsgData;
    msg.Abandoned = abandoned;
    msg.PvpBattle = false;
    msg.Winner[0] = playerWon;
    msg.Winner[1] = !playerWon;
    msg.NpcCreatureID[0] = 0;
    msg.NpcCreatureID[1] = _wildCreatureId;

    for (PetBattleCombatant& combatant : _playerTeam)
    {
        // PB-W2/W4: AwardedXP tracks SeenAction, not Winner. PB-W1 forfeit: AwardedXP false.
        combatant.AwardedXP = combatant.SeenAction && !abandoned;

        WorldPackets::BattlePet::PetBattleFinalPet finalPet;
        finalPet.Guid = combatant.JournalGuid;
        finalPet.Level = combatant.Level;
        finalPet.Xp = combatant.Xp;
        finalPet.Health = combatant.Health;
        finalPet.MaxHealth = combatant.MaxHealth;
        finalPet.InitialLevel = combatant.InitialLevel;
        finalPet.Pboid = combatant.Pboid;
        finalPet.SeenAction = combatant.SeenAction;
        finalPet.AwardedXP = combatant.AwardedXP;
        msg.Pets.push_back(finalPet);
    }

    for (PetBattleCombatant const& combatant : _wildTeam)
    {
        WorldPackets::BattlePet::PetBattleFinalPet finalPet;
        finalPet.Guid = combatant.JournalGuid;
        finalPet.Level = combatant.Level;
        finalPet.Xp = combatant.Xp;
        finalPet.Health = combatant.Health;
        finalPet.MaxHealth = combatant.MaxHealth;
        finalPet.InitialLevel = combatant.InitialLevel;
        finalPet.Pboid = combatant.Pboid;
        finalPet.SeenAction = combatant.SeenAction;
        finalPet.AwardedXP = false;
        msg.Pets.push_back(finalPet);
    }

    _owner->SendPacket(packet.Write());
    _status = PetBattleStatus::PendingFinish;
}

void PetBattle::HandleFinalNotify()
{
    if (_status != PetBattleStatus::PendingFinish)
        return;

    WorldPackets::BattlePet::PetBattleFinished finished;
    _owner->SendPacket(finished.Write());

    WriteJournalResults();
    _status = PetBattleStatus::Finished;
}

void PetBattle::WriteJournalResults()
{
    BattlePetMgr* mgr = _owner->GetBattlePetMgr();
    if (!mgr)
        return;

    Player* player = _owner->GetPlayer();
    if (!player)
        return;

    uint16 wildLevel = _wildTeam.empty() ? 0 : _wildTeam.front().Level;

    std::vector<std::reference_wrapper<BattlePet const>> updates;
    for (PetBattleCombatant const& combatant : _playerTeam)
    {
        BattlePet* journalPet = mgr->GetPet(combatant.JournalGuid);
        if (!journalPet)
            continue;

        // GrantBattlePetExperience recalculates stats and resets Health to MaxHealth, so award
        // XP first, then re-apply the injured Health persisted from the battle.
        // PB-W4: grant when AwardedXP (lose still awards); forfeit leaves AwardedXP false.
        if (combatant.AwardedXP)
        {
            if (uint32 xp = GetXPEarn(combatant.Level, wildLevel))
                mgr->GrantBattlePetExperience(combatant.JournalGuid, uint16(std::min<uint32>(xp, 0xFFFF)), BattlePetXpSource::PetBattle);
        }

        journalPet->PacketInfo.Health = uint32(std::clamp<int32>(combatant.Health, 0, int32(journalPet->PacketInfo.MaxHealth)));
        if (journalPet->SaveInfo != BATTLE_PET_NEW)
            journalPet->SaveInfo = BATTLE_PET_CHANGED;

        updates.push_back(*journalPet);
    }

    // NYI: post-battle auto HealBattlePetsPct / spell 125439 (PB-W4: no auto revive in sniff).
    if (!updates.empty())
        mgr->SendUpdates(updates, false);

    // Real decisive outcome only — forfeit sets _finalAbandoned and must not grant quest credit.
    // Newbie (7433) / Just a Pup (6566) unlock slots via BattlePetReachLevel from XP above;
    // WinPetBattle feeds win-count achievements (e.g. Experienced Pet Battler).
    if (_finalAbandoned)
        return;

    if (_finalPlayerWon)
    {
        player->UpdateCriteria(CriteriaType::WinPetBattle, 1);
        player->KilledMonsterCredit(NPC_KILL_CREDIT_WIN_A_PET_BATTLE);
    }
    else
        player->UpdateCriteria(CriteriaType::LosePetBattle, 1);
}
}

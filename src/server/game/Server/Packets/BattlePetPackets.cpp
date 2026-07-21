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

#include "BattlePetPackets.h"
#include "PacketOperators.h"

namespace WorldPackets::BattlePet
{
ByteBuffer& operator<<(ByteBuffer& data, BattlePetSlot const& slot)
{
    data << (slot.Pet.Guid.IsEmpty() ? ObjectGuid::Create<HighGuid::BattlePet>(0) : slot.Pet.Guid);
    data << uint32(slot.CollarID);
    data << uint8(slot.Index);
    data << Bits<1>(slot.Locked);
    data.FlushBits();

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, BattlePetOwnerInfo const& owner)
{
    data << owner.Guid;
    data << uint32(owner.PlayerVirtualRealm);
    data << uint32(owner.PlayerNativeRealm);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, BattlePet const& pet)
{
    data << pet.Guid;
    data << uint32(pet.Species);
    data << uint32(pet.CreatureID);
    data << uint32(pet.DisplayID);
    data << uint16(pet.Breed);
    data << uint16(pet.Level);
    data << uint16(pet.Exp);
    data << uint16(pet.Flags);
    data << uint32(pet.Power);
    data << uint32(pet.Health);
    data << uint32(pet.MaxHealth);
    data << uint32(pet.Speed);
    data << uint8(pet.Quality);
    data << SizedString::BitsSize<7>(pet.Name);
    data << OptionalInit(pet.OwnerInfo);
    data << Bits<1>(pet.NoRename);
    data.FlushBits();

    data << SizedString::Data(pet.Name);

    if (pet.OwnerInfo)
        data << *pet.OwnerInfo;

    return data;
}

WorldPacket const* BattlePetJournal::Write()
{
    _worldPacket << uint16(Trap);
    _worldPacket << Size<uint32>(Slots);
    _worldPacket << Size<uint32>(Pets);
    _worldPacket << Bits<1>(HasJournalLock);
    _worldPacket.FlushBits();

    for (BattlePetSlot const& slot : Slots)
        _worldPacket << slot;

    for (BattlePet const& pet : Pets)
        _worldPacket << pet;

    return &_worldPacket;
}

WorldPacket const* BattlePetUpdates::Write()
{
    _worldPacket << Size<uint32>(Pets);
    _worldPacket << Bits<1>(PetAdded);
    _worldPacket.FlushBits();

    for (BattlePet const& pet : Pets)
        _worldPacket << pet;

    return &_worldPacket;
}

WorldPacket const* PetBattleSlotUpdates::Write()
{
    _worldPacket << Size<uint32>(Slots);
    _worldPacket << Bits<1>(NewSlot);
    _worldPacket << Bits<1>(AutoSlotted);
    _worldPacket.FlushBits();

    for (BattlePetSlot const& slot : Slots)
        _worldPacket << slot;

    return &_worldPacket;
}

void BattlePetSetBattleSlot::Read()
{
    _worldPacket >> PetGuid;
    _worldPacket >> Slot;
}

void BattlePetModifyName::Read()
{
    _worldPacket >> PetGuid;
    _worldPacket >> SizedString::BitsSize<7>(Name);
    _worldPacket >> OptionalInit(DeclinedNames);

    if (DeclinedNames)
    {
        for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            _worldPacket >> SizedString::BitsSize<7>(DeclinedNames->name[i]);

        for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            _worldPacket >> SizedString::Data(DeclinedNames->name[i]);
    }

    _worldPacket >> SizedString::Data(Name);
}

void QueryBattlePetName::Read()
{
    _worldPacket >> BattlePetID;
    _worldPacket >> UnitGUID;
}

WorldPacket const* QueryBattlePetNameResponse::Write()
{
    _worldPacket << BattlePetID;
    _worldPacket << int32(CreatureID);
    _worldPacket << Timestamp;

    _worldPacket << Bits<1>(Allow);

    if (Allow)
    {
        _worldPacket << SizedString::BitsSize<8>(Name);
        _worldPacket << Bits<1>(HasDeclined);

        for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            _worldPacket << SizedString::BitsSize<7>(DeclinedNames.name[i]);

        _worldPacket.FlushBits();

        for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            _worldPacket << SizedString::Data(DeclinedNames.name[i]);

        _worldPacket << SizedString::Data(Name);
    }
    else
        _worldPacket.FlushBits();

    return &_worldPacket;
}

void BattlePetDeletePet::Read()
{
    _worldPacket >> PetGuid;
}

void BattlePetSetFlags::Read()
{
    _worldPacket >> PetGuid;
    _worldPacket >> Flags;
    _worldPacket >> Bits<2>(ControlType);
}

void BattlePetClearFanfare::Read()
{
    _worldPacket >> PetGuid;
}

void CageBattlePet::Read()
{
    _worldPacket >> PetGuid;
}

WorldPacket const* BattlePetDeleted::Write()
{
    _worldPacket << PetGuid;

    return &_worldPacket;
}

WorldPacket const* BattlePetError::Write()
{
    _worldPacket << Bits<4>(Result);
    _worldPacket << int32(CreatureID);

    return &_worldPacket;
}

void BattlePetSummon::Read()
{
    _worldPacket >> PetGuid;
}

void BattlePetUpdateNotify::Read()
{
    _worldPacket >> PetGuid;
}

// ---- Pet Battle (combat) ---------------------------------------------------------------

ByteBuffer& operator<<(ByteBuffer& data, PetBattleLocation const& location)
{
    data << int32(location.LocationResult);
    data << location.BattleOrigin;
    data << float(location.BattleFacing);
    for (TaggedPosition<Position::XYZ> const& playerPosition : location.PlayerPositions)
        data << playerPosition;

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattleEffectTarget const& target)
{
    data << Bits<4>(target.Type);
    data << int8(target.Petx);

    switch (target.Type)
    {
        case 3: // PET (set/change health)
            data << int32(target.Health);
            break;
        case 2: // STATE
            data << int32(target.StateID);
            data << int32(target.StateValue);
            break;
        default: // FRONT_PET and others carry no extra payload in V1
            break;
    }

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattleEffect const& effect)
{
    data << uint32(effect.AbilityEffectID);
    data << uint16(effect.Flags);
    data << uint16(effect.SourceAuraInstanceID);
    data << uint16(effect.TurnInstanceID);
    data << uint8(effect.EffectType);
    data << int8(effect.CasterPBOID);
    data << uint8(effect.StackDepth);
    data << Size<uint32>(effect.Targets);
    for (PetBattleEffectTarget const& target : effect.Targets)
        data << target;

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattleActiveAbility const& ability)
{
    data << int32(ability.AbilityID);
    data << int16(ability.CooldownRemaining);
    data << int16(ability.LockdownRemaining);
    data << uint8(ability.AbilityIndex);
    data << uint8(ability.Pboid);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattleActiveAura const& aura)
{
    data << int32(aura.AbilityID);
    data << uint32(aura.InstanceID);
    data << int32(aura.RoundsRemaining);
    data << int32(aura.CurrentRound);
    data << uint8(aura.CasterPBOID);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattleState const& state)
{
    data << uint32(state.StateID);
    data << int32(state.StateValue);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattlePetUpdate const& pet)
{
    data << pet.BattlePetGUID;
    data << uint32(pet.SpeciesID);
    data << uint32(pet.DisplayID);
    data << uint32(pet.CollarID);
    data << uint16(pet.Level);
    data << uint16(pet.Xp);
    data << int32(pet.CurHealth);
    data << int32(pet.MaxHealth);
    data << int32(pet.Power);
    data << int32(pet.Speed);
    data << uint32(pet.NpcTeamMemberID);
    data << uint8(pet.BreedQuality);
    data << uint16(pet.StatusFlags);
    data << uint8(pet.Slot);
    data << Size<uint32>(pet.Abilities);
    data << Size<uint32>(pet.Auras);
    data << Size<uint32>(pet.States);
    for (PetBattleActiveAbility const& ability : pet.Abilities)
        data << ability;
    for (PetBattleActiveAura const& aura : pet.Auras)
        data << aura;
    for (PetBattleState const& state : pet.States)
        data << state;

    data << SizedString::BitsSize<7>(pet.CustomName);
    data.FlushBits();
    data << SizedString::Data(pet.CustomName);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattlePlayerUpdate const& player)
{
    data << player.CharacterID;
    data << int32(player.TrapAbilityID);
    data << int32(player.TrapStatus);
    data << uint16(player.RoundTimeSecs);
    data << int8(player.FrontPet);
    data << uint8(player.InputFlags);
    data << Bits<2>(uint32(player.Pets.size()));
    data.FlushBits();
    for (PetBattlePetUpdate const& pet : player.Pets)
        data << pet;

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattleEnviroUpdate const& enviro)
{
    data << Size<uint32>(enviro.Auras);
    data << Size<uint32>(enviro.States);
    for (PetBattleActiveAura const& aura : enviro.Auras)
        data << aura;
    for (PetBattleState const& state : enviro.States)
        data << state;

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattleFullUpdate const& update)
{
    for (PetBattlePlayerUpdate const& player : update.Players)
        data << player;
    for (PetBattleEnviroUpdate const& enviro : update.Enviros)
        data << enviro;

    data << uint16(update.WaitingForFrontPetsMaxSecs);
    data << uint16(update.PvpMaxRoundTime);
    data << uint32(update.CurRound);
    data << uint32(update.NpcCreatureID);
    data << uint32(update.NpcDisplayID);
    data << int8(update.CurPetBattleState);
    data << uint8(update.ForfeitPenalty);
    data << update.InitialWildPetGUID;
    data << Bits<1>(update.IsPVP);
    data << Bits<1>(update.CanAwardXP);
    data.FlushBits();

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, PetBattleFinalPet const& pet)
{
    data << pet.Guid;
    data << uint16(pet.Level);
    data << uint16(pet.Xp);
    data << int32(pet.Health);
    data << int32(pet.MaxHealth);
    data << uint16(pet.InitialLevel);
    data << uint8(pet.Pboid);
    data << Bits<1>(pet.Captured);
    data << Bits<1>(pet.SeenAction);
    data << Bits<1>(pet.Caged);
    data << Bits<1>(pet.AwardedXP);
    data.FlushBits();

    return data;
}

void PetBattleRequestWild::Read()
{
    _worldPacket >> TargetGUID;
    _worldPacket >> Location.LocationResult;
    _worldPacket >> Location.BattleOrigin;
    _worldPacket >> Location.BattleFacing;
    for (TaggedPosition<Position::XYZ>& playerPosition : Location.PlayerPositions)
        _worldPacket >> playerPosition;
}

void PetBattleRequestUpdate::Read()
{
    _worldPacket >> TargetGUID;
    Canceled = _worldPacket.ReadBit();
}

void PetBattleInput::Read()
{
    _worldPacket >> MoveType;
    _worldPacket >> NewFrontPet;
    _worldPacket >> DebugFlags;
    _worldPacket >> BattleInterrupted;
    _worldPacket >> AbilityID;
    _worldPacket >> Round;
    IgnoreAbandonPenalty = _worldPacket.ReadBit();
}

void PetBattleReplaceFrontPet::Read()
{
    _worldPacket >> FrontPet;
}

WorldPacket const* PetBattleRequestFailed::Write()
{
    _worldPacket << uint8(Reason);

    return &_worldPacket;
}

WorldPacket const* PetBattleFinalizeLocation::Write()
{
    _worldPacket << Location;

    return &_worldPacket;
}

WorldPacket const* PetBattleInitialUpdate::Write()
{
    _worldPacket << MsgData;

    return &_worldPacket;
}

WorldPacket const* PetBattleRound::Write()
{
    _worldPacket << uint32(MsgData.CurRound);
    _worldPacket << uint8(MsgData.NextPetBattleState);
    _worldPacket << uint32(MsgData.Effects.size());

    for (uint8 i = 0; i < PET_BATTLE_PARTICIPANTS_COUNT; ++i)
    {
        _worldPacket << uint8(MsgData.NextInputFlags[i]);
        _worldPacket << uint8(MsgData.NextTrapStatus[i]);
        _worldPacket << uint16(MsgData.RoundTimeSecs[i]);
    }

    _worldPacket << uint32(MsgData.Cooldowns.size());
    for (PetBattleActiveAbility const& cooldown : MsgData.Cooldowns)
        _worldPacket << cooldown;

    _worldPacket << Bits<3>(uint32(MsgData.PetXDied.size()));
    _worldPacket.FlushBits();

    for (PetBattleEffect const& effect : MsgData.Effects)
        _worldPacket << effect;

    for (uint8 petX : MsgData.PetXDied)
        _worldPacket << uint8(petX);

    return &_worldPacket;
}

WorldPacket const* PetBattleFinalRound::Write()
{
    _worldPacket << Bits<1>(MsgData.Abandoned);
    _worldPacket << Bits<1>(MsgData.PvpBattle);
    _worldPacket << Bits<1>(MsgData.Winner[0]);
    _worldPacket << Bits<1>(MsgData.Winner[1]);
    _worldPacket.FlushBits();

    for (uint8 i = 0; i < PET_BATTLE_PARTICIPANTS_COUNT; ++i)
        _worldPacket << uint32(MsgData.NpcCreatureID[i]);

    _worldPacket << uint32(MsgData.Pets.size());
    for (PetBattleFinalPet const& pet : MsgData.Pets)
        _worldPacket << pet;

    return &_worldPacket;
}
}

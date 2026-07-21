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

#ifndef TRINITYCORE_BATTLE_PET_PACKETS_H
#define TRINITYCORE_BATTLE_PET_PACKETS_H

#include "Packet.h"
#include "PacketUtilities.h"
#include "ObjectGuid.h"
#include "Optional.h"
#include "Position.h"
#include "UnitDefines.h"
#include <array>
#include <memory>
#include <vector>

namespace WorldPackets
{
    namespace BattlePet
    {
        struct BattlePetOwnerInfo
        {
            ObjectGuid Guid;
            uint32 PlayerVirtualRealm = 0;
            uint32 PlayerNativeRealm = 0;
        };

        struct BattlePet
        {
            ObjectGuid Guid;
            uint32 Species = 0;
            uint32 CreatureID = 0;
            uint32 DisplayID = 0;
            uint16 Breed = 0;
            uint16 Level = 0;
            uint16 Exp = 0;
            uint16 Flags = 0;
            uint32 Power = 0;
            uint32 Health = 0;
            uint32 MaxHealth = 0;
            uint32 Speed = 0;
            uint8 Quality = 0;
            Optional<BattlePetOwnerInfo> OwnerInfo;
            std::string Name;
            bool NoRename = false;
        };

        struct BattlePetSlot
        {
            BattlePet Pet;
            uint32 CollarID = 0;
            uint8 Index = 0;
            bool Locked = true;
        };

        class BattlePetJournal final : public ServerPacket
        {
        public:
            BattlePetJournal() : ServerPacket(SMSG_BATTLE_PET_JOURNAL) { }

            WorldPacket const* Write() override;

            uint16 Trap = 0;
            bool HasJournalLock = false;
            std::vector<std::reference_wrapper<BattlePetSlot>> Slots;
            std::vector<std::reference_wrapper<BattlePet>> Pets;
        };

        class BattlePetJournalLockAcquired final : public ServerPacket
        {
        public:
            BattlePetJournalLockAcquired() : ServerPacket(SMSG_BATTLE_PET_JOURNAL_LOCK_ACQUIRED, 0) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };

        class BattlePetJournalLockDenied final : public ServerPacket
        {
        public:
            BattlePetJournalLockDenied() : ServerPacket(SMSG_BATTLE_PET_JOURNAL_LOCK_DENIED, 0) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };

        class BattlePetRequestJournal final : public ClientPacket
        {
        public:
            explicit BattlePetRequestJournal(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_REQUEST_JOURNAL, std::move(packet)) { }

            void Read() override { }
        };

        class BattlePetRequestJournalLock final : public ClientPacket
        {
        public:
            explicit BattlePetRequestJournalLock(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_REQUEST_JOURNAL_LOCK, std::move(packet)) { }

            void Read() override { }
        };

        class BattlePetUpdates final : public ServerPacket
        {
        public:
            BattlePetUpdates() : ServerPacket(SMSG_BATTLE_PET_UPDATES) { }

            WorldPacket const* Write() override;

            std::vector<std::reference_wrapper<BattlePet const>> Pets;
            bool PetAdded = false;
        };

        class PetBattleSlotUpdates final : public ServerPacket
        {
        public:
            PetBattleSlotUpdates() : ServerPacket(SMSG_PET_BATTLE_SLOT_UPDATES) { }

            WorldPacket const* Write() override;

            std::vector<BattlePetSlot> Slots;
            bool AutoSlotted = false;
            bool NewSlot = false;
        };

        class BattlePetSetBattleSlot final : public ClientPacket
        {
        public:
            explicit BattlePetSetBattleSlot(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_SET_BATTLE_SLOT, std::move(packet)) { }

            void Read() override;

            ObjectGuid PetGuid;
            uint8 Slot = 0;
        };

        class BattlePetModifyName final : public ClientPacket
        {
        public:
            explicit BattlePetModifyName(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_MODIFY_NAME, std::move(packet)) { }

            void Read() override;

            ObjectGuid PetGuid;
            std::string Name;
            std::unique_ptr<DeclinedName> DeclinedNames;
        };

        class QueryBattlePetName final : public ClientPacket
        {
        public:
            explicit QueryBattlePetName(WorldPacket&& packet) : ClientPacket(CMSG_QUERY_BATTLE_PET_NAME, std::move(packet)) { }

            void Read() override;

            ObjectGuid BattlePetID;
            ObjectGuid UnitGUID;
        };

        class QueryBattlePetNameResponse final : public ServerPacket
        {
        public:
            QueryBattlePetNameResponse() : ServerPacket(SMSG_QUERY_BATTLE_PET_NAME_RESPONSE) { }

            WorldPacket const* Write() override;

            ObjectGuid BattlePetID;
            int32 CreatureID = 0;
            WorldPackets::Timestamp<> Timestamp;
            bool Allow = false;

            bool HasDeclined = false;
            DeclinedName DeclinedNames;
            std::string Name;
        };

        class BattlePetDeletePet final : public ClientPacket
        {
        public:
            explicit BattlePetDeletePet(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_DELETE_PET, std::move(packet)) { }

            void Read() override;

            ObjectGuid PetGuid;
        };

        class BattlePetSetFlags final : public ClientPacket
        {
        public:
            explicit BattlePetSetFlags(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_SET_FLAGS, std::move(packet)) { }

            void Read() override;

            ObjectGuid PetGuid;
            uint16 Flags = 0;
            uint8 ControlType = 0;
        };

        class BattlePetClearFanfare final : public ClientPacket
        {
        public:
            explicit BattlePetClearFanfare(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_CLEAR_FANFARE, std::move(packet)) { }

            void Read() override;

            ObjectGuid PetGuid;
        };

        class CageBattlePet final : public ClientPacket
        {
        public:
            explicit CageBattlePet(WorldPacket&& packet) : ClientPacket(CMSG_CAGE_BATTLE_PET, std::move(packet)) { }

            void Read() override;

            ObjectGuid PetGuid;
        };

        class BattlePetDeleted final : public ServerPacket
        {
        public:
            BattlePetDeleted() : ServerPacket(SMSG_BATTLE_PET_DELETED, 16) { }

            WorldPacket const* Write() override;

            ObjectGuid PetGuid;
        };

        class BattlePetError final : public ServerPacket
        {
        public:
            BattlePetError() : ServerPacket(SMSG_BATTLE_PET_ERROR, 5) { }

            WorldPacket const* Write() override;

            uint8 Result = 0;
            int32 CreatureID = 0;
        };

        class BattlePetSummon final : public ClientPacket
        {
        public:
            explicit BattlePetSummon(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_SUMMON, std::move(packet)) { }

            void Read() override;

            ObjectGuid PetGuid;
        };

        class BattlePetUpdateNotify final : public ClientPacket
        {
        public:
            explicit BattlePetUpdateNotify(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PET_UPDATE_NOTIFY, std::move(packet)) { }

            void Read() override;

            ObjectGuid PetGuid;
        };

        // ---- Pet Battle (combat) ---------------------------------------------------------------
        // Midnight (12.0.7) wire from retail sniffs; structural lead from BfaCore Reforged.

        static constexpr uint8 PET_BATTLE_PARTICIPANTS_COUNT = 2;
        static constexpr uint8 PET_BATTLE_ENVIRO_COUNT = 3;

        struct PetBattleLocation
        {
            int32 LocationResult = 0;
            TaggedPosition<Position::XYZ> BattleOrigin;
            float BattleFacing = 0.0f;
            TaggedPosition<Position::XYZ> PlayerPositions[PET_BATTLE_PARTICIPANTS_COUNT];
        };

        // effect target params kept minimal for V1 (front pet / pet health / state)
        struct PetBattleEffectTarget
        {
            uint8 Type = 0;
            int8 Petx = 0;
            int32 Health = 0;      // PET_BATTLE_EFFECT_TARGET_PET
            uint32 StateID = 0;    // PET_BATTLE_EFFECT_TARGET_STATE
            int32 StateValue = 0;  // PET_BATTLE_EFFECT_TARGET_STATE
        };

        struct PetBattleEffect
        {
            uint32 AbilityEffectID = 0;
            uint16 Flags = 0;
            uint16 SourceAuraInstanceID = 0;
            uint16 TurnInstanceID = 0;
            uint8 EffectType = 0;
            int8 CasterPBOID = 0;
            uint8 StackDepth = 0;
            std::vector<PetBattleEffectTarget> Targets;
        };

        struct PetBattleActiveAbility
        {
            int32 AbilityID = 0;
            int16 CooldownRemaining = 0;
            int16 LockdownRemaining = 0;
            uint8 AbilityIndex = 0;
            uint8 Pboid = 0;
        };

        struct PetBattleActiveAura
        {
            int32 AbilityID = 0;
            uint32 InstanceID = 0;
            int32 RoundsRemaining = 0;
            int32 CurrentRound = 0;
            uint8 CasterPBOID = 0;
        };

        struct PetBattleState
        {
            uint32 StateID = 0;
            int32 StateValue = 0;
        };

        struct PetBattleRoundResult
        {
            uint32 CurRound = 0;
            uint8 NextPetBattleState = 0;
            uint8 NextInputFlags[PET_BATTLE_PARTICIPANTS_COUNT] = { };
            uint8 NextTrapStatus[PET_BATTLE_PARTICIPANTS_COUNT] = { };
            uint16 RoundTimeSecs[PET_BATTLE_PARTICIPANTS_COUNT] = { };
            std::vector<PetBattleEffect> Effects;
            std::vector<PetBattleActiveAbility> Cooldowns;
            std::vector<uint8> PetXDied;
            // Bytes after PetXDied that WPP still leaves unread (PB-W FIRST_ROUND trailer).
            std::vector<uint8> TrailingBytes;
        };

        struct PetBattlePetUpdate
        {
            ObjectGuid BattlePetGUID;
            uint32 SpeciesID = 0;
            uint32 DisplayID = 0;
            uint32 CollarID = 0;
            uint16 Level = 0;
            uint16 Xp = 0;
            int32 CurHealth = 0;
            int32 MaxHealth = 0;
            int32 Power = 0;
            int32 Speed = 0;
            uint32 NpcTeamMemberID = 0;
            uint8 BreedQuality = 0; // TWW+: 1 byte on wire (journal Quality already uint8)
            uint16 StatusFlags = 0;
            uint8 Slot = 0;
            std::vector<PetBattleActiveAbility> Abilities;
            std::vector<PetBattleActiveAura> Auras;
            std::vector<PetBattleState> States;
            std::string CustomName;
        };

        struct PetBattlePlayerUpdate
        {
            ObjectGuid CharacterID;
            int32 TrapAbilityID = 0;
            int32 TrapStatus = 0;
            uint16 RoundTimeSecs = 0;
            int8 FrontPet = 0;
            uint8 InputFlags = 0;
            std::vector<PetBattlePetUpdate> Pets;
        };

        struct PetBattleEnviroUpdate
        {
            std::vector<PetBattleActiveAura> Auras;
            std::vector<PetBattleState> States;
        };

        struct PetBattleFullUpdate
        {
            PetBattlePlayerUpdate Players[PET_BATTLE_PARTICIPANTS_COUNT];
            PetBattleEnviroUpdate Enviros[PET_BATTLE_ENVIRO_COUNT];
            ObjectGuid InitialWildPetGUID;
            uint32 NpcCreatureID = 0;
            uint32 NpcDisplayID = 0;
            uint32 CurRound = 0;
            uint16 WaitingForFrontPetsMaxSecs = 30;
            uint16 PvpMaxRoundTime = 30;
            uint8 ForfeitPenalty = 0;
            int8 CurPetBattleState = 0;
            bool IsPVP = false;
            bool CanAwardXP = false;
        };

        struct PetBattleFinalPet
        {
            ObjectGuid Guid;
            uint16 Level = 0;
            uint16 Xp = 0;
            int32 Health = 0;
            int32 MaxHealth = 0;
            uint16 InitialLevel = 0;
            uint8 Pboid = 0;
            bool Captured = false;
            bool Caged = false;
            bool SeenAction = false;
            bool AwardedXP = false;
        };

        struct PetBattleFinalRoundData
        {
            bool Abandoned = false;
            bool PvpBattle = false;
            bool Winner[PET_BATTLE_PARTICIPANTS_COUNT] = { };
            uint32 NpcCreatureID[PET_BATTLE_PARTICIPANTS_COUNT] = { };
            std::vector<PetBattleFinalPet> Pets;
        };

        class PetBattleRequestWild final : public ClientPacket
        {
        public:
            explicit PetBattleRequestWild(WorldPacket&& packet) : ClientPacket(CMSG_PET_BATTLE_REQUEST_WILD, std::move(packet)) { }

            void Read() override;

            ObjectGuid TargetGUID;
            PetBattleLocation Location;
        };

        class PetBattleRequestUpdate final : public ClientPacket
        {
        public:
            explicit PetBattleRequestUpdate(WorldPacket&& packet) : ClientPacket(CMSG_PET_BATTLE_REQUEST_UPDATE, std::move(packet)) { }

            void Read() override;

            ObjectGuid TargetGUID;
            bool Canceled = false;
        };

        class PetBattleInput final : public ClientPacket
        {
        public:
            explicit PetBattleInput(WorldPacket&& packet) : ClientPacket(CMSG_PET_BATTLE_INPUT, std::move(packet)) { }

            void Read() override;

            uint32 MoveType = 0;
            int32 NewFrontPet = 0;
            uint8 DebugFlags = 0;
            uint8 BattleInterrupted = 0;
            int32 AbilityID = 0;
            uint32 Round = 0;
            bool IgnoreAbandonPenalty = false;
        };

        class PetBattleReplaceFrontPet final : public ClientPacket
        {
        public:
            explicit PetBattleReplaceFrontPet(WorldPacket&& packet) : ClientPacket(CMSG_PET_BATTLE_REPLACE_FRONT_PET, std::move(packet)) { }

            void Read() override;

            uint8 FrontPet = 0;
        };

        class PetBattleQuitNotify final : public ClientPacket
        {
        public:
            explicit PetBattleQuitNotify(WorldPacket&& packet) : ClientPacket(CMSG_PET_BATTLE_QUIT_NOTIFY, std::move(packet)) { }

            void Read() override { }
        };

        class PetBattleFinalNotify final : public ClientPacket
        {
        public:
            explicit PetBattleFinalNotify(WorldPacket&& packet) : ClientPacket(CMSG_PET_BATTLE_FINAL_NOTIFY, std::move(packet)) { }

            void Read() override { }
        };

        class PetBattleRequestFailed final : public ServerPacket
        {
        public:
            PetBattleRequestFailed() : ServerPacket(SMSG_PET_BATTLE_REQUEST_FAILED, 1) { }

            WorldPacket const* Write() override;

            uint8 Reason = 0;
        };

        class PetBattleFinalizeLocation final : public ServerPacket
        {
        public:
            PetBattleFinalizeLocation() : ServerPacket(SMSG_PET_BATTLE_FINALIZE_LOCATION, 44) { }

            WorldPacket const* Write() override;

            PetBattleLocation Location;
        };

        class PetBattleInitialUpdate final : public ServerPacket
        {
        public:
            PetBattleInitialUpdate() : ServerPacket(SMSG_PET_BATTLE_INITIAL_UPDATE) { }

            WorldPacket const* Write() override;

            PetBattleFullUpdate MsgData;
        };

        class PetBattleRound final : public ServerPacket
        {
        public:
            explicit PetBattleRound(OpcodeServer opcode) : ServerPacket(opcode) { }

            WorldPacket const* Write() override;

            PetBattleRoundResult MsgData;
        };

        class PetBattleFinalRound final : public ServerPacket
        {
        public:
            PetBattleFinalRound() : ServerPacket(SMSG_PET_BATTLE_FINAL_ROUND) { }

            WorldPacket const* Write() override;

            PetBattleFinalRoundData MsgData;
        };

        class PetBattleFinished final : public ServerPacket
        {
        public:
            PetBattleFinished() : ServerPacket(SMSG_PET_BATTLE_FINISHED, 0) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };
    }
}

#endif // TRINITYCORE_BATTLE_PET_PACKETS_H

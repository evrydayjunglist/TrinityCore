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

#include "WorldSession.h"
#include "BattlePetMgr.h"
#include "BattlePetPackets.h"
#include "ObjectAccessor.h"
#include "PetBattle.h"
#include "Player.h"
#include "TemporarySummon.h"

void WorldSession::HandleBattlePetRequestJournal(WorldPackets::BattlePet::BattlePetRequestJournal& /*battlePetRequestJournal*/)
{
    GetBattlePetMgr()->SendJournal();
}

void WorldSession::HandleBattlePetRequestJournalLock(WorldPackets::BattlePet::BattlePetRequestJournalLock& /*battlePetRequestJournalLock*/)
{
    GetBattlePetMgr()->SendJournalLockStatus();

    if (GetBattlePetMgr()->HasJournalLock())
        GetBattlePetMgr()->SendJournal();
}

void WorldSession::HandleBattlePetSetBattleSlot(WorldPackets::BattlePet::BattlePetSetBattleSlot& battlePetSetBattleSlot)
{
    if (BattlePets::BattlePet* pet = GetBattlePetMgr()->GetPet(battlePetSetBattleSlot.PetGuid))
        if (WorldPackets::BattlePet::BattlePetSlot* slot = GetBattlePetMgr()->GetSlot(BattlePets::BattlePetSlot(battlePetSetBattleSlot.Slot)))
            slot->Pet = pet->PacketInfo;
}

void WorldSession::HandleBattlePetModifyName(WorldPackets::BattlePet::BattlePetModifyName& battlePetModifyName)
{
    GetBattlePetMgr()->ModifyName(battlePetModifyName.PetGuid, battlePetModifyName.Name, std::move(battlePetModifyName.DeclinedNames));
}

void WorldSession::HandleQueryBattlePetName(WorldPackets::BattlePet::QueryBattlePetName& queryBattlePetName)
{
    WorldPackets::BattlePet::QueryBattlePetNameResponse response;
    response.BattlePetID = queryBattlePetName.BattlePetID;

    Creature* summonedBattlePet = ObjectAccessor::GetCreatureOrPetOrVehicle(*_player, queryBattlePetName.UnitGUID);
    if (!summonedBattlePet || !summonedBattlePet->IsSummon())
    {
        SendPacket(response.Write());
        return;
    }

    response.CreatureID = summonedBattlePet->GetEntry();
    response.Timestamp = summonedBattlePet->GetBattlePetCompanionNameTimestamp();

    Unit* petOwner = summonedBattlePet->ToTempSummon()->GetSummonerUnit();
    if (!petOwner->IsPlayer())
    {
        SendPacket(response.Write());
        return;
    }

    BattlePets::BattlePet const* battlePet = petOwner->ToPlayer()->GetSession()->GetBattlePetMgr()->GetPet(queryBattlePetName.BattlePetID);
    if (!battlePet)
    {
        SendPacket(response.Write());
        return;
    }

    response.Name = battlePet->PacketInfo.Name;
    if (battlePet->DeclinedName)
    {
        response.HasDeclined = true;
        response.DeclinedNames = *battlePet->DeclinedName;
    }

    response.Allow = !response.Name.empty();

    SendPacket(response.Write());
}

void WorldSession::HandleBattlePetDeletePet(WorldPackets::BattlePet::BattlePetDeletePet& battlePetDeletePet)
{
    GetBattlePetMgr()->RemovePet(battlePetDeletePet.PetGuid);
}

void WorldSession::HandleBattlePetSetFlags(WorldPackets::BattlePet::BattlePetSetFlags& battlePetSetFlags)
{
    if (!GetBattlePetMgr()->HasJournalLock())
        return;

    if (BattlePets::BattlePet* pet = GetBattlePetMgr()->GetPet(battlePetSetFlags.PetGuid))
    {
        if (battlePetSetFlags.ControlType == BattlePets::FLAGS_CONTROL_TYPE_APPLY)
            pet->PacketInfo.Flags |= battlePetSetFlags.Flags;
        else // FLAGS_CONTROL_TYPE_REMOVE
            pet->PacketInfo.Flags &= ~battlePetSetFlags.Flags;

        if (pet->SaveInfo != BattlePets::BATTLE_PET_NEW)
            pet->SaveInfo = BattlePets::BATTLE_PET_CHANGED;
    }
}

void WorldSession::HandleBattlePetClearFanfare(WorldPackets::BattlePet::BattlePetClearFanfare& battlePetClearFanfare)
{
    GetBattlePetMgr()->ClearFanfare(battlePetClearFanfare.PetGuid);
}

void WorldSession::HandleCageBattlePet(WorldPackets::BattlePet::CageBattlePet& cageBattlePet)
{
    GetBattlePetMgr()->CageBattlePet(cageBattlePet.PetGuid);
}

void WorldSession::HandleBattlePetSummon(WorldPackets::BattlePet::BattlePetSummon& battlePetSummon)
{
    if (*_player->m_activePlayerData->SummonedBattlePetGUID != battlePetSummon.PetGuid)
        GetBattlePetMgr()->SummonPet(battlePetSummon.PetGuid);
    else
        GetBattlePetMgr()->DismissPet();
}

void WorldSession::HandleBattlePetUpdateNotify(WorldPackets::BattlePet::BattlePetUpdateNotify& battlePetUpdateNotify)
{
    GetBattlePetMgr()->UpdateBattlePetData(battlePetUpdateNotify.PetGuid);
}

void WorldSession::HandlePetBattleRequestWild(WorldPackets::BattlePet::PetBattleRequestWild& petBattleRequestWild)
{
    // Only one wild PvE battle per session at a time.
    if (_petBattle)
        return;

    auto sendFailed = [this](uint8 reason)
    {
        WorldPackets::BattlePet::PetBattleRequestFailed failed;
        failed.Reason = reason;
        SendPacket(failed.Write());
    };

    if (_player->IsInCombat())
    {
        sendFailed(5); // ERR_PETBATTLE_NOT_HERE-class reason observed in retail while in world combat
        return;
    }

    auto battle = std::make_unique<BattlePets::PetBattle>(this);
    if (!battle->SetupWild(petBattleRequestWild.TargetGUID, petBattleRequestWild.Location))
    {
        sendFailed(5);
        return;
    }

    _petBattle = std::move(battle);
    _petBattle->Start();
}

void WorldSession::HandlePetBattleRequestUpdate(WorldPackets::BattlePet::PetBattleRequestUpdate& /*petBattleRequestUpdate*/)
{
    // NYI: reconcile pending/queued battle state (V1 wild path has nothing to answer yet).
}

void WorldSession::HandlePetBattleInput(WorldPackets::BattlePet::PetBattleInput& petBattleInput)
{
    if (_petBattle)
        _petBattle->HandleInput(petBattleInput);
}

void WorldSession::HandlePetBattleReplaceFrontPet(WorldPackets::BattlePet::PetBattleReplaceFrontPet& petBattleReplaceFrontPet)
{
    if (_petBattle)
        _petBattle->HandleReplaceFrontPet(petBattleReplaceFrontPet.FrontPet);
}

void WorldSession::HandlePetBattleQuitNotify(WorldPackets::BattlePet::PetBattleQuitNotify& /*petBattleQuitNotify*/)
{
    if (_petBattle)
        _petBattle->HandleQuit();
}

void WorldSession::HandlePetBattleFinalNotify(WorldPackets::BattlePet::PetBattleFinalNotify& /*petBattleFinalNotify*/)
{
    if (!_petBattle)
        return;

    _petBattle->HandleFinalNotify();
    if (_petBattle->IsFinished())
        _petBattle.reset();
}

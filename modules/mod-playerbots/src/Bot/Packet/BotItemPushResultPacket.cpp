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

#include "BotItemPushResultPacket.h"
#include "BotPacketParse.h"
#include "PacketOperators.h"

namespace Playerbots::PacketParse
{
namespace
{
void ReadUiEventToast(ByteBuffer& data, WorldPackets::Item::UiEventToast& out)
{
    // Mirror WorldPackets::Item::operator<<(UiEventToast) field order exactly.
    data >> out.UiEventToastID;
    data >> out.Asset;
}

void ReadSpellReducedReagent(ByteBuffer& data, WorldPackets::Crafting::SpellReducedReagent& out)
{
    // Mirror WorldPackets::Crafting::operator<<(SpellReducedReagent) field order exactly.
    data >> out.Quantity;
    data >> out.Reagent;
}

void ReadCraftingData(ByteBuffer& data, WorldPackets::Crafting::CraftingData& out)
{
    // Mirror WorldPackets::Crafting::operator<<(CraftingData) field order exactly.
    data >> out.CraftingQualityID;
    data >> out.QualityProgress;
    data >> out.SkillLineAbilityID;
    data >> out.CraftingDataID;
    data >> out.Multicraft;
    data >> out.SkillFromReagents;
    data >> out.Skill;
    data >> out.CritBonusSkill;
    data >> out.ModSkillGain;
    data >> out.OrderID;
    data >> WorldPackets::Size<uint32>(out.ResourcesReturned);
    data >> out.OperationID;
    data >> out.ItemGUID;
    data >> out.Quantity;
    data >> out.EnchantID;
    data >> out.ConcentrationCurrencyID;
    data >> out.ConcentrationSpent;
    data >> out.IngenuityRefund;

    data >> WorldPackets::Bits<1>(out.IsCrit);
    data >> WorldPackets::Bits<1>(out.IsRecraft);
    data >> WorldPackets::Bits<1>(out.IsInitialRecraft);
    data >> WorldPackets::Bits<1>(out.IsFirstCraft);
    data >> WorldPackets::Bits<1>(out.HasIngenuityProc);
    data >> WorldPackets::Bits<1>(out.ApplyConcentration);
    data.ResetBitPos();

    data >> out.OldItem;
    data >> out.NewItem;

    for (WorldPackets::Crafting::SpellReducedReagent& reagent : out.ResourcesReturned)
        ReadSpellReducedReagent(data, reagent);
}

void ReadItemPushResultBody(ByteBuffer& data, ItemPushResultPayload& out)
{
    // Mirror WorldPackets::Item::ItemPushResult::Write() field order exactly.
    data >> out.PlayerGUID;
    data >> out.Slot;
    data >> out.SlotInBag;
    data >> out.ProxyItemID;
    data >> out.Quantity;
    data >> out.QuantityInInventory;
    data >> out.QuantityInQuestLog;
    data >> out.EncounterID;
    data >> out.BattlePetSpeciesID;
    data >> out.BattlePetBreedID;
    data >> out.BattlePetBreedQuality;
    data >> out.BattlePetLevel;
    data >> out.ItemGUID;
    data >> WorldPackets::Size<uint32>(out.Toasts);
    for (WorldPackets::Item::UiEventToast& toast : out.Toasts)
        ReadUiEventToast(data, toast);

    data >> WorldPackets::Bits<1>(out.Pushed);
    data >> WorldPackets::Bits<1>(out.Created);
    data >> WorldPackets::Bits<1>(out.FakeQuestItem);
    data >> WorldPackets::Bits<3>(out.ChatNotifyType);
    data >> WorldPackets::Bits<1>(out.IsBonusRoll);
    data >> WorldPackets::Bits<1>(out.IsPersonalLoot);
    data >> WorldPackets::OptionalInit(out.CraftingData);
    data >> WorldPackets::OptionalInit(out.FirstCraftOperationID);
    data.ResetBitPos();

    data >> out.Item;

    if (out.FirstCraftOperationID)
        data >> *out.FirstCraftOperationID;

    if (out.CraftingData)
        ReadCraftingData(data, *out.CraftingData);
}
}

bool TryReadItemPushResult(WorldPacket const& packet, ItemPushResultPayload& out)
{
    ItemPushResultPayload parsed;
    Result const result = TryReadFully(packet, "SMSG_ITEM_PUSH_RESULT", [&parsed](WorldPacket& copy)
    {
        ReadItemPushResultBody(copy, parsed);
    });

    if (result != Result::Ok)
        return false;

    out = std::move(parsed);
    return true;
}
}

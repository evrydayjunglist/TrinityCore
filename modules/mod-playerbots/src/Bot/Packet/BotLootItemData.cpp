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

#include "BotLootItemData.h"
#include "ItemPacketsCommon.h"
#include "LootItemType.h"
#include "PacketOperators.h"

namespace Playerbots::PacketParse
{
void ReadLootItemData(ByteBuffer& data, WorldPackets::Loot::LootItemData& out)
{
    // Mirror WorldPackets::Loot::operator<<(LootItemData) field order exactly.
    uint8 typeRaw = 0;
    data >> WorldPackets::Bits<2>(typeRaw);
    data >> WorldPackets::Bits<3>(out.UIType);
    data >> WorldPackets::Bits<1>(out.CanTradeToTapList);
    data.ResetBitPos();

    out.Type = static_cast<::LootItemType>(typeRaw);
    data >> out.Loot;
    data >> out.Quantity;
    data >> out.LootItemType;
    data >> out.LootListID;
}
}

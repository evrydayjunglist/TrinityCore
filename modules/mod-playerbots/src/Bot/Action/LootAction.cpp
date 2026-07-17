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

#include "LootAction.h"
#include "BotPlayerbotAI.h"
#include "CellImpl.h"
#include "Creature.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "Loot.h"
#include "LootPackets.h"
#include "LootQuestFilter.h"
#include "MotionMaster.h"
#include "ObjectDefines.h"
#include "Opcodes.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "SafeMovement.h"
#include "WorldPacket.h"
#include "WorldSession.h"

// AC reference: mod-playerbots-master OpenLootAction / StoreLootAction. Open stays SendLoot for
// now; store is Handle*-only via StoreLootAction (see packetless-retirement inventory).

namespace
{
bool LootHasRelevantItem(Player* bot, Loot const* loot, bool questOnly)
{
    if (!loot)
        return false;

    for (LootItem const& item : loot->items)
    {
        if (item.is_looted)
            continue;

        Optional<LootSlotType> const slotType = item.GetUiTypeForPlayer(bot, *loot);
        if (!slotType || (*slotType != LOOT_SLOT_TYPE_ALLOW_LOOT && *slotType != LOOT_SLOT_TYPE_OWNER))
            continue;

        if (questOnly && !Playerbots::IsQuestRelevantLootItem(bot, item.itemid))
            continue;

        // Don't count a corpse as worth approaching if the bot can't actually store the item
        // (e.g. bags full) — otherwise it would path to it and re-attempt every tick, starving the
        // status machine. Currency/tracking-quest items aren't bag-stored, so only gate real items.
        if (item.type == LootItemType::Item)
        {
            ItemPosCountVec dest;
            if (bot->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, item.itemid, item.count) != EQUIP_ERR_OK)
                continue;
        }

        return true;
    }

    return false;
}

// Nearest dead creature this bot is allowed to loot (core ownership/anti-ninja check) that holds an
// item worth taking. AC's LootObjectStack keeps a rolling set fed by combat kills; a live grid scan
// (same shape as AttackAnythingAction / QuestGiverAction) is the fork-native equivalent and needs
// no per-bot state.
class LootableCorpseCheck
{
public:
    LootableCorpseCheck(Player* bot, float range, bool questOnly) : _bot(bot), _range(range), _questOnly(questOnly) { }

    bool operator()(Creature* creature) const
    {
        if (creature->IsAlive())
            return false;

        if (!_bot->IsWithinDist(creature, _range))
            return false;

        // Delegates every ownership rule to the core: GetLootForPlayer non-null, not fully looted,
        // HasAllowedLooter, and hasItemFor — i.e. exactly what a real looting player is allowed.
        if (!_bot->isAllowedToLoot(creature))
            return false;

        return LootHasRelevantItem(_bot, creature->GetLootForPlayer(_bot), _questOnly);
    }

private:
    Player* _bot;
    float _range;
    bool _questOnly;
};

Creature* FindNearbyLootableCorpse(Player* bot, float range, bool questOnly)
{
    Creature* result = nullptr;
    float bestDistSq = (range * range) + 1.0f;

    std::list<Creature*> corpses;
    LootableCorpseCheck check(bot, range, questOnly);
    Trinity::CreatureListSearcher<LootableCorpseCheck> searcher(bot, corpses, check);
    Cell::VisitAllObjects(bot, searcher, range);

    for (Creature* creature : corpses)
    {
        float const distSq = bot->GetExactDistSq(creature);
        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            result = creature;
        }
    }

    return result;
}
} // namespace

bool LootAction::IsUseful()
{
    Player* bot = GetBot();
    if (!bot || !bot->IsInWorld() || !bot->IsAlive())
        return false;

    // Loot after the fight, never during it (AC runs looting in its non-combat strategy).
    if (bot->IsInCombat())
        return false;

    // Open windows are drained by StoreLootAction after SMSG_LOOT_RESPONSE — do not poll AE view
    // here (that was the old packetless drain wake-up).
    return FindNearbyLootableCorpse(bot, Playerbots::GetLootDistance(), Playerbots::GetLootQuestItemsOnly()) != nullptr;
}

bool LootAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    bool const questOnly = Playerbots::GetLootQuestItemsOnly();

    Creature* corpse = FindNearbyLootableCorpse(bot, Playerbots::GetLootDistance(), questOnly);
    if (!corpse)
        return false;

    // Walk into interaction range if needed (same SafeMovement contract Wander/Grind/QuestGiver
    // use — never hand a raw point to MotionMaster without a validated navmesh route).
    if (bot->GetDistance(corpse) > INTERACTION_DISTANCE - 1.0f)
    {
        if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE)
            return true; // already walking somewhere this tick

        return TryMoveToValidatedPoint(bot, corpse->GetPositionX(), corpse->GetPositionY(),
            corpse->GetPositionZ());
    }

    // In range: open only. Store/money/release run via SMSG_LOOT_RESPONSE → StoreLootAction.
    Loot* loot = corpse->GetLootForPlayer(bot);
    if (!loot)
        return false;

    bot->SendLoot(*loot);

    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots", "LootAction bot={} opened loot on {}",
            bot->GetName(), corpse->GetName());

    return true;
}

bool StoreLootAction::IsUseful()
{
    return _botAI && _botAI->GetPendingLootStore().has_value();
}

bool StoreLootAction::Execute(Event /*event*/)
{
    if (!_botAI)
        return false;

    std::optional<BotPlayerbotAI::PendingLootStore> const pending = _botAI->GetPendingLootStore();
    _botAI->ClearPendingLootStore();
    if (!pending)
        return false;

    Player* bot = _botAI->GetBot();
    if (!bot || !bot->GetSession())
        return false;

    bool const questOnly = Playerbots::GetLootQuestItemsOnly();
    bool const lootMoney = Playerbots::GetLootMoney();
    uint32 stored = 0;

    if (lootMoney && pending->Coins > 0)
    {
        WorldPacket moneyPacket(CMSG_LOOT_MONEY);
        WorldPackets::Loot::LootMoney money(std::move(moneyPacket));
        bot->GetSession()->HandleLootMoneyOpcode(money);
    }

    for (BotPlayerbotAI::PendingLootStore::ItemSlot const& slot : pending->Items)
    {
        if (slot.UIType != LOOT_SLOT_TYPE_ALLOW_LOOT && slot.UIType != LOOT_SLOT_TYPE_OWNER)
            continue;

        if (questOnly && !Playerbots::IsQuestRelevantLootItem(bot, slot.ItemID))
            continue;

        WorldPacket itemPacket(CMSG_LOOT_ITEM);
        WorldPackets::Loot::LootItem lootItem(std::move(itemPacket));
        WorldPackets::Loot::LootRequest req;
        req.Object = pending->LootObj;
        req.LootListID = slot.LootListID;
        lootItem.Loot.push_back(req);
        bot->GetSession()->HandleAutostoreLootItemOpcode(lootItem);
        ++stored;
    }

    WorldPacket releasePacket(CMSG_LOOT_RELEASE);
    WorldPackets::Loot::LootRelease release(std::move(releasePacket));
    release.Unit = pending->Owner;
    bot->GetSession()->HandleLootReleaseOpcode(release);

    if (stored)
        _botAI->GetRpgStatistics().itemsLooted += stored;

    if (Playerbots::GetLogLevel() >= 1)
        TC_LOG_DEBUG("playerbots", "StoreLootAction bot={} owner={} lootObj={} coins={} stored={}",
            bot->GetName(),
            pending->Owner.ToString(),
            pending->LootObj.ToString(),
            pending->Coins,
            stored);

    return true;
}

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
#include "MotionMaster.h"
#include "ObjectDefines.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "QuestDef.h"
#include "SafeMovement.h"
#include "SpellAuraDefines.h"
#include "Util.h"
#include "WorldSession.h"

// AC reference: mod-playerbots-master/src/Ai/Base/Actions/LootAction.cpp (OpenLootAction::DoLoot,
// StoreLootAction::Execute/IsLootAllowed). TC-native/packetless adaptations are documented in
// LootAction.h and inline below.

namespace
{
// AC's IsLootAllowed quest core: an item is worth storing if it starts a quest, or it satisfies an
// incomplete ITEM objective in one of the bot's own quests. Everything else is left on the corpse
// in the default (quest-only) mode so looting never becomes gear/vendor churn (Gate 18 scope).
bool IsQuestRelevantItem(Player* bot, uint32 itemId)
{
    if (!itemId)
        return false;

    if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId))
        if (proto->GetStartQuest() != 0)
            return true;

    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 const questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;

        for (QuestObjective const& objective : quest->Objectives)
        {
            if (objective.Type != QUEST_OBJECTIVE_ITEM)
                continue;

            if (uint32(objective.ObjectID) != itemId)
                continue;

            if (!bot->IsQuestObjectiveComplete(slot, quest, objective))
                return true;
        }
    }

    return false;
}

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

        if (questOnly && !IsQuestRelevantItem(bot, item.itemid))
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

// Stores every allowed, quest-relevant slot from every loot window currently open on the bot, then
// closes those windows — the packetless equivalent of the client's autostore-then-release flow
// (WorldSession::HandleAutostoreLootItemOpcode + HandleLootMoneyOpcode + DoLootRelease). Returns how
// many item slots were taken. The AE view is copied first because StoreLootItem/DoLootRelease mutate
// it as loot is consumed.
uint32 StoreOpenLoot(Player* bot, bool questOnly, bool lootMoney)
{
    uint32 stored = 0;

    std::unordered_map<ObjectGuid, Loot*> const view = bot->GetAELootView();
    std::vector<Loot*> toRelease;
    toRelease.reserve(view.size());

    for (auto const& [lootGuid, loot] : view)
    {
        if (!loot)
            continue;

        // Money (single-player branch of HandleLootMoneyOpcode — bots are solo).
        if (lootMoney && loot->gold > 0)
        {
            uint64 const goldMod = CalculatePct(loot->gold,
                bot->GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_MONEY_GAIN, 1));
            bot->ModifyMoney(loot->gold + goldMod);
            bot->UpdateCriteria(CriteriaType::MoneyLootedFromCreatures, loot->gold);
            loot->NotifyMoneyRemoved(bot->GetMap());
            loot->LootMoney();
        }

        ObjectGuid const owner = loot->GetOwnerGUID();
        for (LootItem const& item : loot->items)
        {
            if (item.is_looted)
                continue;

            Optional<LootSlotType> const slotType = item.GetUiTypeForPlayer(bot, *loot);
            if (!slotType || (*slotType != LOOT_SLOT_TYPE_ALLOW_LOOT && *slotType != LOOT_SLOT_TYPE_OWNER))
                continue;

            if (questOnly && !IsQuestRelevantItem(bot, item.itemid))
                continue;

            // StoreLootItem re-checks HasAllowedLooter / blocked / roll-winner and applies quest
            // credit as the item enters the bag; lootSlot is the LootListId.
            bot->StoreLootItem(owner, item.LootListId, loot);
            ++stored;
        }

        toRelease.push_back(loot);
    }

    for (Loot* loot : toRelease)
        bot->GetSession()->DoLootRelease(loot);

    return stored;
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

    // A window left open (e.g. a gameobject the UseQuestObjectAction just opened) still needs
    // draining, even if no fresh corpse is nearby.
    if (!bot->GetAELootView().empty())
        return true;

    return FindNearbyLootableCorpse(bot, Playerbots::GetLootDistance(), Playerbots::GetLootQuestItemsOnly()) != nullptr;
}

bool LootAction::Execute(Event /*event*/)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    bool const questOnly = Playerbots::GetLootQuestItemsOnly();
    bool const lootMoney = Playerbots::GetLootMoney();

    // 1) Drain anything already open (GO loot opened last tick, or a corpse we opened and haven't
    //    finished). This is a no-op when the view is empty.
    uint32 looted = StoreOpenLoot(bot, questOnly, lootMoney);

    // 2) Find a fresh corpse to open.
    Creature* corpse = FindNearbyLootableCorpse(bot, Playerbots::GetLootDistance(), questOnly);
    if (!corpse)
    {
        _botAI->GetRpgStatistics().itemsLooted += looted;
        return looted > 0;
    }

    // 3) Walk into interaction range if needed (same SafeMovement contract Wander/Grind/QuestGiver
    //    use — never hand a raw point to MotionMaster without a validated navmesh route).
    if (bot->GetDistance(corpse) > INTERACTION_DISTANCE - 1.0f)
    {
        if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE)
            return looted > 0; // already walking somewhere this tick

        if (TryMoveToValidatedPoint(bot, corpse->GetPositionX(), corpse->GetPositionY(), corpse->GetPositionZ()))
            return true;

        _botAI->GetRpgStatistics().itemsLooted += looted;
        return looted > 0;
    }

    // 4) In range: open the corpse (registers it in the AE loot view, packetless — the SMSG the
    //    core emits is harmlessly dropped by the socketless bot session) and store from it.
    if (Loot* loot = corpse->GetLootForPlayer(bot))
    {
        bot->SendLoot(*loot);
        looted += StoreOpenLoot(bot, questOnly, lootMoney);
    }

    if (looted)
    {
        _botAI->GetRpgStatistics().itemsLooted += looted;
        TC_LOG_DEBUG("playerbots", "[New RPG] {} looted {} quest item(s) from {}",
            bot->GetName(), looted, corpse->GetName());
    }

    return true;
}

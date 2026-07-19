/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 */

#include "tc_catch2.h"

#include "AchievementMgr.h"
#include "DB2Structure.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace
{
    class TestCriteriaHandler final : public CriteriaHandler
    {
    public:
        bool IsCompleted(CriteriaType type, uint64 counter, uint64 requiredAmount)
        {
            CriteriaEntry entry{};
            entry.ID = uint32(type) + 1;
            entry.Type = int16(type);

            Criteria criteria{ .ID = entry.ID, .Entry = &entry };
            _criteriaProgress[criteria.ID].Counter = counter;
            return IsCompletedCriteria(&criteria, requiredAmount);
        }

        bool MeetsRequirements(CriteriaType type, int32 asset, uint64 miscValue1)
        {
            CriteriaEntry entry{};
            entry.ID = uint32(type) + 1;
            entry.Type = int16(type);
            entry.Asset.ID = asset;

            Criteria criteria{ .ID = entry.ID, .Entry = &entry };
            return RequirementsSatisfied(&criteria, miscValue1, 0, 0, nullptr, nullptr);
        }

        bool MeetsModifier(ModifierTreeType type, int32 asset, uint64 miscValue1, uint64 miscValue2)
        {
            ModifierTreeEntry modifier{};
            modifier.Type = int32(type);
            modifier.Asset = asset;
            return ModifierSatisfied(&modifier, miscValue1, miscValue2, nullptr, nullptr);
        }

        void SendAllData(Player const*) const override { }

    protected:
        void SendCriteriaUpdate(Criteria const*, CriteriaProgress const*, Seconds, bool) const override { }
        void SendCriteriaProgressRemoved(uint32) override { }
        void CompletedCriteriaTree(CriteriaTree const*, Player*) override { }
        void SendPacket(WorldPacket const*) const override { }
        std::string GetOwnerInfo() const override { return "test"; }

        CriteriaList const& GetCriteriaByType(CriteriaType, uint32) const override
        {
            static CriteriaList const Empty;
            return Empty;
        }
    };
}

TEST_CASE("Achievement criteria preserve DB2-declared owners across shared trees", "[Achievement][Archaeology]")
{
    AchievementEntry playerAchievement{};
    playerAchievement.ID = 4854;

    AchievementEntry accountAchievement{};
    accountAchievement.ID = 4855;
    accountAchievement.Flags = ACHIEVEMENT_FLAG_ACCOUNT;

    CriteriaTree playerTree{ .Achievement = &playerAchievement };
    CriteriaTree accountTree{ .Achievement = &accountAchievement };
    CriteriaTreeList sharedTrees{ &playerTree, &accountTree };

    CHECK(GetCriteriaOwnerFlags(sharedTrees) == (CRITERIA_FLAG_CU_PLAYER | CRITERIA_FLAG_CU_ACCOUNT));
    CHECK(IsCriteriaTreeForOwner(&playerTree, AchievementCriteriaOwner::Player));
    CHECK_FALSE(IsCriteriaTreeForOwner(&playerTree, AchievementCriteriaOwner::Account));
    CHECK(IsCriteriaTreeForOwner(&accountTree, AchievementCriteriaOwner::Account));
    CHECK_FALSE(IsCriteriaTreeForOwner(&accountTree, AchievementCriteriaOwner::Player));
}

TEST_CASE("Archaeology criteria events increment once and filter find assets", "[Achievement][Archaeology]")
{
    TestCriteriaHandler handler;

    CHECK(GetArchaeologyCriteriaProgressDelta(CriteriaType::CompleteAnyResearchProject) == 1);
    CHECK(GetArchaeologyCriteriaProgressDelta(CriteriaType::FindResearchObject) == 1);
    CHECK(GetArchaeologyCriteriaProgressDelta(CriteriaType::ExhaustAnyResearchSite) == 1);
    CHECK_FALSE(GetArchaeologyCriteriaProgressDelta(CriteriaType::CompleteResearchProject).has_value());

    CHECK_FALSE(handler.IsCompleted(CriteriaType::CompleteAnyResearchProject, 1, 2));
    CHECK(handler.IsCompleted(CriteriaType::CompleteAnyResearchProject, 2, 2));
    CHECK(handler.IsCompleted(CriteriaType::FindResearchObject, 2, 2));
    CHECK(handler.IsCompleted(CriteriaType::ExhaustAnyResearchSite, 2, 2));

    constexpr int32 QuestFindGameObject = 246804;
    CHECK(handler.MeetsRequirements(CriteriaType::FindResearchObject, QuestFindGameObject, QuestFindGameObject));
    CHECK_FALSE(handler.MeetsRequirements(CriteriaType::FindResearchObject, QuestFindGameObject, 278476));
    CHECK_FALSE(handler.MeetsRequirements(CriteriaType::FindResearchObject, QuestFindGameObject, 0));
}

TEST_CASE("Archaeology modifiers consume rarity and branch event fields", "[Achievement][Archaeology]")
{
    TestCriteriaHandler handler;
    constexpr uint64 Rare = 1;
    constexpr uint64 Branch = 27;

    CHECK(handler.MeetsModifier(ModifierTreeType::ResearchProjectRarity, Rare, Rare, Branch));
    CHECK_FALSE(handler.MeetsModifier(ModifierTreeType::ResearchProjectRarity, Rare, 0, Rare));

    CHECK(handler.MeetsModifier(ModifierTreeType::ResearchProjectBranch, Branch, Rare, Branch));
    CHECK_FALSE(handler.MeetsModifier(ModifierTreeType::ResearchProjectBranch, Branch, Branch, Rare));
}

TEST_CASE("Legacy account criteria migration does not double-add durable progress", "[Achievement][Archaeology]")
{
    CriteriaEntry entry{};
    entry.ID = 16191;

    Criteria criteria{ .ID = entry.ID, .Entry = &entry, .FlagsCu = CRITERIA_FLAG_CU_ACCOUNT };
    CriteriaProgressMap progressMap;

    CriteriaProgress& durableProgress = progressMap[criteria.ID];
    durableProgress.Counter = 4;
    durableProgress.Date = 10;
    durableProgress.Changed = false;

    CHECK_FALSE(MergeLegacyAccountCriteriaProgress(progressMap, &criteria, 3, 20, ObjectGuid::Empty));
    CHECK(progressMap[criteria.ID].Counter == 4);
    CHECK(progressMap[criteria.ID].Date == 10);
    CHECK_FALSE(progressMap[criteria.ID].Changed);

    progressMap.clear();
    CHECK(MergeLegacyAccountCriteriaProgress(progressMap, &criteria, 3, 20, ObjectGuid::Empty));
    CHECK(progressMap[criteria.ID].Counter == 3);
    CHECK(progressMap[criteria.ID].Date == 20);
    CHECK(progressMap[criteria.ID].Changed);

    CHECK_FALSE(MergeLegacyAccountCriteriaProgress(progressMap, &criteria, 3, 20, ObjectGuid::Empty));
    CHECK(progressMap[criteria.ID].Counter == 3);
}

TEST_CASE("Account achievement saves target only dirty rows", "[Achievement]")
{
    // Multi-session contract: a sibling WorldSession must not rewrite clean rows it never changed.
    // CollectChangedAccountAchievementSaveTargets is what AccountAchievementMgr::SaveToDB persists.
    std::unordered_map<uint32, CompletedAchievementData> completed;
    completed[100].Changed = false;
    completed[100].Date = 1;
    completed[200].Changed = true;
    completed[200].Date = 2;

    CriteriaProgressMap progress;
    progress[11].Changed = false;
    progress[11].Counter = 9;
    progress[22].Changed = true;
    progress[22].Counter = 3;
    progress[33].Changed = true;
    progress[33].Counter = 0; // zeroed progress is still a dirty delete target

    std::vector<uint32> dirtyAchievements;
    std::vector<uint32> dirtyCriteria;
    CollectChangedAccountAchievementSaveTargets(completed, progress, dirtyAchievements, dirtyCriteria);

    REQUIRE(dirtyAchievements.size() == 1);
    CHECK(dirtyAchievements[0] == 200);

    REQUIRE(dirtyCriteria.size() == 2);
    CHECK(std::find(dirtyCriteria.begin(), dirtyCriteria.end(), 22) != dirtyCriteria.end());
    CHECK(std::find(dirtyCriteria.begin(), dirtyCriteria.end(), 33) != dirtyCriteria.end());
    CHECK(std::find(dirtyCriteria.begin(), dirtyCriteria.end(), 11) == dirtyCriteria.end());
}

TEST_CASE("Account achievement reset must clear sibling clean rows for SaveToDB contract", "[Achievement]")
{
    // AccountAchievementMgr::Reset deletes the whole battlenet account's durable rows, then
    // SaveToDB only rewrites Changed=true rows. A sibling session that still holds the pre-reset
    // snapshot as Changed=false therefore never restores (or deletes) those rows — permanent loss
    // unless Reset also clears sibling in-memory state (ClearLocalState / World invalidate).
    std::unordered_map<uint32, CompletedAchievementData> siblingCompleted;
    siblingCompleted[100].Changed = false;
    siblingCompleted[100].Date = 1;

    CriteriaProgressMap siblingProgress;
    siblingProgress[11].Changed = false;
    siblingProgress[11].Counter = 9;

    std::vector<uint32> dirtyAchievements;
    std::vector<uint32> dirtyCriteria;
    CollectChangedAccountAchievementSaveTargets(siblingCompleted, siblingProgress, dirtyAchievements, dirtyCriteria);

    CHECK(dirtyAchievements.empty());
    CHECK(dirtyCriteria.empty());

    // After ClearLocalState-equivalent wipe, there is nothing left to accidentally resurrect.
    siblingCompleted.clear();
    siblingProgress.clear();
    dirtyAchievements.clear();
    dirtyCriteria.clear();
    CollectChangedAccountAchievementSaveTargets(siblingCompleted, siblingProgress, dirtyAchievements, dirtyCriteria);
    CHECK(dirtyAchievements.empty());
    CHECK(dirtyCriteria.empty());
}

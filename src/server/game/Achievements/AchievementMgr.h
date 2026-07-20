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

#ifndef __TRINITY_ACHIEVEMENTMGR_H
#define __TRINITY_ACHIEVEMENTMGR_H

#include "CriteriaHandler.h"
#include "DatabaseEnvFwd.h"

class Guild;
class WorldSession;

namespace WorldPackets::Achievement
{
    class AllAchievementData;
    class AllAccountCriteria;
}

struct AchievementReward
{
    uint32 TitleId[2];
    uint32 ItemId;
    uint32 SenderCreatureId;
    std::string Subject;
    std::string Body;
    uint32 MailTemplateId;
};

struct AchievementRewardLocale
{
    std::vector<std::string> Subject;
    std::vector<std::string> Body;
};

TC_GAME_API bool MergeLegacyAccountCriteriaProgress(CriteriaProgressMap& progressMap, Criteria const* criteria,
    uint64 counter, std::time_t date, ObjectGuid accountGuid);

struct CompletedAchievementData
{
    std::time_t Date = std::time_t(0);
    GuidSet CompletingPlayers;
    bool Changed;
};

// Lists only dirty account achievement / criteria rows. AccountAchievementMgr::SaveToDB must persist
// exactly these ids with per-row deletes — never a whole-account wipe — so a sibling WorldSession
// (master-alt bot) cannot erase live progress it never loaded.
TC_GAME_API void CollectChangedAccountAchievementSaveTargets(
    std::unordered_map<uint32, CompletedAchievementData> const& completedAchievements,
    CriteriaProgressMap const& criteriaProgress,
    std::vector<uint32>& outAchievementIds,
    std::vector<uint32>& outCriteriaIds);

// Absolute sibling sync helpers for multi-session AccountAchievementMgr (MasterAlt bots).
// Progress: take MAX(local, published); counter==0 forces a remove. Completion: insert-only
// (never RewardAchievement — the publishing session already rewarded).
TC_GAME_API bool ApplySiblingAccountCriteriaProgress(CriteriaProgressMap& progressMap, uint32 criteriaId,
    uint64 counter, std::time_t date, ObjectGuid accountGuid);
TC_GAME_API bool ApplySiblingAccountCompletedAchievement(
    std::unordered_map<uint32, CompletedAchievementData>& completedAchievements, uint32& achievementPoints,
    uint32 achievementId, std::time_t date, uint32 points, bool trackingFlag);

class TC_GAME_API AchievementMgr : public CriteriaHandler
{
public:
    AchievementMgr();
    ~AchievementMgr();

    void CheckAllAchievementCriteria(Player* referencePlayer);

    virtual void CompletedAchievement(AchievementEntry const* entry, Player* referencePlayer) = 0;
    bool HasAchieved(uint32 achievementId) const;
    uint32 GetAchievementPoints() const;
    std::vector<uint32> GetCompletedAchievementIds() const;

protected:
    bool CanUpdateCriteriaTree(Criteria const* criteria, CriteriaTree const* tree, Player* referencePlayer) const override;
    bool CanCompleteCriteriaTree(CriteriaTree const* tree) override;
    void CompletedCriteriaTree(CriteriaTree const* tree, Player* referencePlayer) override;
    void AfterCriteriaTreeUpdate(CriteriaTree const* tree, Player* referencePlayer) override;

    bool IsCompletedAchievement(AchievementEntry const* entry);

    bool RequiredAchievementSatisfied(uint32 achievementId) const override;

protected:
    std::unordered_map<uint32, CompletedAchievementData> _completedAchievements;
    uint32 _achievementPoints;
};

class TC_GAME_API AccountAchievementMgr : public AchievementMgr
{
public:
    explicit AccountAchievementMgr(WorldSession* owner);

    void Reset() override;

    void LoadFromDB(PreparedQueryResult achievementResult, PreparedQueryResult criteriaResult);
    void SaveToDB(LoginDatabaseTransaction trans);
    void MigrateLegacyCharacterData(uint32 gameAccountId);

    void SendAllData(Player const* receiver) const override;
    void AppendAllData(WorldPackets::Achievement::AllAchievementData& achievements,
        WorldPackets::Achievement::AllAccountCriteria& criteria) const;
    void ApplyRetroactiveRewards(Player* player) const;

    void CompletedAchievement(AchievementEntry const* entry, Player* referencePlayer) override;

    // Apply a sibling session's published absolute progress / completion without re-publishing or
    // re-rewarding. Used by World::PublishSiblingAccount* after another live session mutated shared
    // Battle.net account achievement state.
    void ApplyPublishedCriteriaProgress(uint32 criteriaId, uint64 counter, std::time_t date);
    void ApplyPublishedCompletedAchievement(AchievementEntry const* achievement, std::time_t date);

protected:
    bool CanUpdateCriteriaTree(Criteria const* criteria, CriteriaTree const* tree, Player* referencePlayer) const override;
    void SendCriteriaUpdate(Criteria const* entry, CriteriaProgress const* progress, Seconds timeElapsed, bool timedCompleted) const override;
    void SendCriteriaProgressRemoved(uint32 criteriaId) override;
    void AfterCriteriaProgressChanged(Criteria const* criteria, CriteriaProgress const* progress) override;

    void SendAchievementEarned(AchievementEntry const* achievement) const;

    void SendPacket(WorldPacket const* data) const override;

    std::string GetOwnerInfo() const override;
    CriteriaList const& GetCriteriaByType(CriteriaType type, uint32 asset) const override;

private:
    WorldSession* _owner;
};

class TC_GAME_API PlayerAchievementMgr : public AchievementMgr
{
public:
    explicit PlayerAchievementMgr(Player* owner);

    void Reset() override;

    static void DeleteFromDB(ObjectGuid const& guid);
    void LoadFromDB(PreparedQueryResult achievementResult, PreparedQueryResult criteriaResult);
    void SaveToDB(CharacterDatabaseTransaction trans);

    void SendAllData(Player const* receiver) const override;
    void SendAchievementInfo(Player* receiver, uint32 achievementId = 0) const;

    void CompletedAchievement(AchievementEntry const* entry, Player* referencePlayer) override;

    using CriteriaHandler::ModifierTreeSatisfied;
    bool ModifierTreeSatisfied(uint32 modifierTreeId) const;

protected:
    bool CanUpdateCriteriaTree(Criteria const* criteria, CriteriaTree const* tree, Player* referencePlayer) const override;
    void SendCriteriaUpdate(Criteria const* entry, CriteriaProgress const* progress, Seconds timeElapsed, bool timedCompleted) const override;
    void SendCriteriaProgressRemoved(uint32 criteriaId) override;

    void SendAchievementEarned(AchievementEntry const* achievement) const;

    void SendPacket(WorldPacket const* data) const override;

    std::string GetOwnerInfo() const override;
    CriteriaList const& GetCriteriaByType(CriteriaType type, uint32 asset) const override;

private:
    Player* _owner;
};

class TC_GAME_API GuildAchievementMgr : public AchievementMgr
{
public:
    explicit GuildAchievementMgr(Guild* owner);

    void Reset() override;

    static void DeleteFromDB(ObjectGuid const& guid);
    void LoadFromDB(PreparedQueryResult achievementResult, PreparedQueryResult criteriaResult);
    void SaveToDB(CharacterDatabaseTransaction trans);

    void SendAllData(Player const* receiver) const override;
    void SendAchievementInfo(Player* receiver, uint32 achievementId = 0) const;
    void SendAllTrackedCriterias(Player* receiver, std::set<uint32> const& trackedCriterias) const;
    void SendAchievementMembers(Player* receiver, uint32 achievementId) const;

    void CompletedAchievement(AchievementEntry const* entry, Player* referencePlayer) override;

protected:
    void SendCriteriaUpdate(Criteria const* entry, CriteriaProgress const* progress, Seconds timeElapsed, bool timedCompleted) const override;
    void SendCriteriaProgressRemoved(uint32 criteriaId) override;

    void SendAchievementEarned(AchievementEntry const* achievement) const;

    void SendPacket(WorldPacket const* data) const override;

    std::string GetOwnerInfo() const override;
    CriteriaList const& GetCriteriaByType(CriteriaType type, uint32 asset) const override;

private:
    Guild* _owner;
};

class TC_GAME_API AchievementGlobalMgr
{
    AchievementGlobalMgr();
    ~AchievementGlobalMgr();

public:
    AchievementGlobalMgr(AchievementGlobalMgr const&) = delete;
    AchievementGlobalMgr(AchievementGlobalMgr&&) = delete;

    AchievementGlobalMgr& operator=(AchievementGlobalMgr const&) = delete;
    AchievementGlobalMgr& operator=(AchievementGlobalMgr&&) = delete;

    static AchievementGlobalMgr* Instance();

    std::vector<AchievementEntry const*> const* GetAchievementByReferencedId(uint32 id) const;
    AchievementReward const* GetAchievementReward(AchievementEntry const* achievement) const;
    AchievementRewardLocale const* GetAchievementRewardLocale(AchievementEntry const* achievement) const;

    bool IsRealmCompleted(AchievementEntry const* achievement) const;
    void SetRealmCompleted(AchievementEntry const* achievement);

    void LoadAchievementReferenceList();
    void LoadAchievementScripts();
    void LoadCompletedAchievements();
    void LoadRewards();
    void LoadRewardLocales();

    uint32 GetAchievementScriptId(uint32 achievementId) const;

private:
    // store achievements by referenced achievement id to speed up lookup
    std::unordered_map<uint32, std::vector<AchievementEntry const*>> _achievementListByReferencedId;

    // store realm first achievements
    // SystemTimePoint::min() is a placeholder value for realm firsts not yet completed
    // SystemTimePoint::max() is a value assigned to realm firsts complete before worldserver started
    std::unordered_map<uint32 /*achievementId*/, SystemTimePoint /*completionTime*/> _allCompletedAchievements;

    std::unordered_map<uint32, AchievementReward> _achievementRewards;
    std::unordered_map<uint32, AchievementRewardLocale> _achievementRewardLocales;
    std::unordered_map<uint32, uint32> _achievementScripts;
};

#define sAchievementMgr AchievementGlobalMgr::Instance()

#endif

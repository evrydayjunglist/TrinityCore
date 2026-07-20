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

#include "AchievementMgr.h"
#include "AchievementPackets.h"
#include "CellImpl.h"
#include "ChatTextBuilder.h"
#include "DB2HotfixGenerator.h"
#include "DB2Stores.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Item.h"
#include "Language.h"
#include "Log.h"
#include "Mail.h"
#include "MapUtils.h"
#include "ObjectMgr.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "StringConvert.h"
#include "World.h"
#include "WorldSession.h"

struct VisibleAchievementCheck
{
    AchievementEntry const* operator()(std::pair<uint32 const, CompletedAchievementData> const& val)
    {
        AchievementEntry const* achievement = sAchievementStore.LookupEntry(val.first);
        if (achievement && !(achievement->Flags & ACHIEVEMENT_FLAG_HIDDEN))
            return achievement;
        return nullptr;
    }
};

namespace
{
int32 GetAchievementRewardTitleId(AchievementEntry const* achievement, AchievementReward const* reward, Player const* player)
{
    //! Currently there's only one achievement that deals with gender-specific titles.
    //! Since no common attributes were found, (not even in titleRewardFlags field)
    //! we explicitly check by ID. Maybe in the future we could move the achievement_reward
    //! condition fields to the condition system.
    if (achievement->ID == 1793)
        return reward->TitleId[player->GetNativeGender()];

    switch (player->GetTeam())
    {
        case ALLIANCE: return reward->TitleId[0];
        case HORDE: return reward->TitleId[1];
        default: break;
    }

    return 0;
}

void RewardAchievement(Player* player, AchievementEntry const* achievement)
{
    AchievementReward const* reward = sAchievementMgr->GetAchievementReward(achievement);
    if (!reward)
        return;

    if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(GetAchievementRewardTitleId(achievement, reward, player)))
        player->SetTitle(titleEntry);

    if (!reward->SenderCreatureId)
        return;

    MailDraft draft(uint16(reward->MailTemplateId));
    if (!reward->MailTemplateId)
    {
        std::string subject = reward->Subject;
        std::string text = reward->Body;

        LocaleConstant localeConstant = player->GetSession()->GetSessionDbLocaleIndex();
        if (localeConstant != LOCALE_enUS)
        {
            if (AchievementRewardLocale const* locale = sAchievementMgr->GetAchievementRewardLocale(achievement))
            {
                ObjectMgr::GetLocaleString(locale->Subject, localeConstant, subject);
                ObjectMgr::GetLocaleString(locale->Body, localeConstant, text);
            }
        }

        draft = MailDraft(subject, text);
    }

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    if (Item* item = reward->ItemId ? Item::CreateItem(reward->ItemId, 1, ItemContext::NONE, player) : nullptr)
    {
        item->SaveToDB(trans);
        draft.AddItem(item);
    }

    draft.SendMailTo(trans, player, MailSender(MAIL_CREATURE, uint64(reward->SenderCreatureId)));
    CharacterDatabase.CommitTransaction(trans);
}

void SendAchievementEarnedForPlayer(Player* player, AchievementEntry const* achievement)
{
    if (achievement->Flags & ACHIEVEMENT_FLAG_HIDDEN)
        return;

    if (!(achievement->Flags & ACHIEVEMENT_FLAG_TRACKING_FLAG))
    {
        if (Guild* guild = sGuildMgr->GetGuildById(player->GetGuildId()))
        {
            Trinity::BroadcastTextBuilder builder(player, CHAT_MSG_GUILD_ACHIEVEMENT, BROADCAST_TEXT_ACHIEVEMENT_EARNED, player->GetNativeGender(), player, achievement->ID);
            Trinity::LocalizedDo<Trinity::BroadcastTextBuilder> localizer(builder);
            guild->BroadcastWorker(localizer, player);
        }

        if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_KILL | ACHIEVEMENT_FLAG_REALM_FIRST_REACH))
        {
            WorldPackets::Achievement::BroadcastAchievement serverFirstAchievement;
            serverFirstAchievement.Name = player->GetName();
            serverFirstAchievement.PlayerGUID = player->GetGUID();
            serverFirstAchievement.AchievementID = achievement->ID;
            sWorld->SendGlobalMessage(serverFirstAchievement.Write());
        }
        else if (player->IsInWorld())
        {
            Trinity::BroadcastTextBuilder builder(player, CHAT_MSG_ACHIEVEMENT, BROADCAST_TEXT_ACHIEVEMENT_EARNED, player->GetNativeGender(), player, achievement->ID);
            Trinity::LocalizedDo<Trinity::BroadcastTextBuilder> localizer(builder);
            Trinity::PlayerDistWorker<Trinity::LocalizedDo<Trinity::BroadcastTextBuilder>> worker(player, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY), localizer);
            Cell::VisitWorldObjects(player, worker, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY));
        }
    }

    auto achievementEarnedBuilder = [player, achievement](Player const* receiver)
    {
        WorldPackets::Achievement::AchievementEarned achievementEarned;
        achievementEarned.Sender = player->GetGUID();
        achievementEarned.Earner = player->GetGUID();
        achievementEarned.EarnerNativeRealm = achievementEarned.EarnerVirtualRealm = player->m_playerData->VirtualPlayerRealm;
        achievementEarned.AchievementID = achievement->ID;
        achievementEarned.Time = *GameTime::GetUtcWowTime();
        achievementEarned.Time += receiver->GetSession()->GetTimezoneOffset();
        achievementEarned.Initial = receiver->HasAtLoginFlag(AT_LOGIN_FIRST);
        receiver->SendDirectMessage(achievementEarned.Write());
    };

    achievementEarnedBuilder(player);

    if (!(achievement->Flags & ACHIEVEMENT_FLAG_TRACKING_FLAG))
    {
        float dist = sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY);
        Trinity::MessageDistDeliverer notifier(player, achievementEarnedBuilder, dist);
        Cell::VisitWorldObjects(player, notifier, dist);
    }
}
}

bool MergeLegacyAccountCriteriaProgress(CriteriaProgressMap& progressMap, Criteria const* criteria,
    uint64 counter, std::time_t date, ObjectGuid accountGuid)
{
    if (!criteria || !(criteria->FlagsCu & CRITERIA_FLAG_CU_ACCOUNT) || progressMap.contains(criteria->ID))
        return false;

    CriteriaProgress& progress = progressMap[criteria->ID];
    progress.Counter = counter;
    progress.Date = date;
    progress.PlayerGUID = accountGuid;
    progress.Changed = true;
    return true;
}

void CollectChangedAccountAchievementSaveTargets(
    std::unordered_map<uint32, CompletedAchievementData> const& completedAchievements,
    CriteriaProgressMap const& criteriaProgress,
    std::vector<uint32>& outAchievementIds,
    std::vector<uint32>& outCriteriaIds)
{
    outAchievementIds.clear();
    outCriteriaIds.clear();

    for (auto const& [achievementId, completedAchievement] : completedAchievements)
        if (completedAchievement.Changed)
            outAchievementIds.push_back(achievementId);

    for (auto const& [criteriaId, progress] : criteriaProgress)
        if (progress.Changed)
            outCriteriaIds.push_back(criteriaId);
}

bool ApplySiblingAccountCriteriaProgress(CriteriaProgressMap& progressMap, uint32 criteriaId,
    uint64 counter, std::time_t date, ObjectGuid accountGuid)
{
    auto itr = progressMap.find(criteriaId);
    if (itr == progressMap.end())
    {
        // Do not materialize a zero row that was never tracked locally.
        if (!counter)
            return false;

        CriteriaProgress& progress = progressMap[criteriaId];
        progress.Counter = counter;
        progress.Date = date;
        progress.PlayerGUID = accountGuid;
        progress.Changed = true;
        return true;
    }

    if (!counter)
    {
        if (!itr->second.Counter)
            return false;

        itr->second.Counter = 0;
        itr->second.Date = date;
        itr->second.PlayerGUID = accountGuid;
        itr->second.Changed = true;
        return true;
    }

    // ACCUMULATE / HIGHEST durability: never regress a sibling that already advanced further.
    if (itr->second.Counter >= counter)
        return false;

    itr->second.Counter = counter;
    itr->second.Date = date;
    itr->second.PlayerGUID = accountGuid;
    itr->second.Changed = true;
    return true;
}

bool ApplySiblingAccountCompletedAchievement(
    std::unordered_map<uint32, CompletedAchievementData>& completedAchievements, uint32& achievementPoints,
    uint32 achievementId, std::time_t date, uint32 points, bool trackingFlag)
{
    if (completedAchievements.contains(achievementId))
        return false;

    CompletedAchievementData& completedAchievement = completedAchievements[achievementId];
    completedAchievement.Date = date;
    // Mark dirty so a racing sibling SaveToDB still persists the row if the publisher's save is delayed.
    completedAchievement.Changed = true;

    if (!trackingFlag)
        achievementPoints += points;

    return true;
}

AchievementMgr::AchievementMgr() : _achievementPoints(0) { }

AchievementMgr::~AchievementMgr() { }

/**
* called at player login. The player might have fulfilled some achievements when the achievement system wasn't working yet
*/
void AchievementMgr::CheckAllAchievementCriteria(Player* referencePlayer)
{
    // suppress sending packets
    for (CriteriaType criteriaType : CriteriaMgr::GetRetroactivelyUpdateableCriteriaTypes())
        UpdateCriteria(criteriaType, 0, 0, 0, nullptr, referencePlayer);

    // Event modifiers cannot be reconstructed at login. Reconcile already-persisted progress
    // directly against its criteria trees so a completion suppressed at event time (for example,
    // while GM mode was enabled) is not permanently stranded.
    for (auto const& [criteriaId, progress] : _criteriaProgress)
    {
        if (!progress.Counter)
            continue;

        Criteria const* criteria = sCriteriaMgr->GetCriteria(criteriaId);
        CriteriaTreeList const* trees = criteria ? sCriteriaMgr->GetCriteriaTreesByCriteria(criteriaId) : nullptr;
        if (!trees)
            continue;

        for (CriteriaTree const* tree : *trees)
            if (CanUpdateCriteriaTree(criteria, tree, referencePlayer) && CanCompleteCriteriaTree(tree))
                CompletedCriteriaTree(tree, referencePlayer);
    }
}

bool AchievementMgr::HasAchieved(uint32 achievementId) const
{
    return _completedAchievements.find(achievementId) != _completedAchievements.end();
}

uint32 AchievementMgr::GetAchievementPoints() const
{
    return _achievementPoints;
}

std::vector<uint32> AchievementMgr::GetCompletedAchievementIds() const
{
    std::vector<uint32> achievementIds(_completedAchievements.size());
    std::ranges::transform(_completedAchievements, achievementIds.begin(), Trinity::Containers::MapKey);
    return achievementIds;
}

bool AchievementMgr::CanUpdateCriteriaTree(Criteria const* criteria, CriteriaTree const* tree, Player* referencePlayer) const
{
    AchievementEntry const* achievement = tree->Achievement;
    if (!achievement)
        return false;

    if (HasAchieved(achievement->ID))
    {
        TC_LOG_TRACE("criteria.achievement", "AchievementMgr::CanUpdateCriteriaTree: (Id: {} Type {} Achievement {}) Achievement already earned",
            criteria->ID, CriteriaMgr::GetCriteriaTypeString(criteria->Entry->Type), achievement->ID);
        return false;
    }

    if ((achievement->Faction == ACHIEVEMENT_FACTION_HORDE    && referencePlayer->GetTeam() != HORDE) ||
        (achievement->Faction == ACHIEVEMENT_FACTION_ALLIANCE && referencePlayer->GetTeam() != ALLIANCE))
    {
        TC_LOG_TRACE("criteria.achievement", "AchievementMgr::CanUpdateCriteriaTree: (Id: {} Type {} Achievement {}) Wrong faction",
            criteria->ID, CriteriaMgr::GetCriteriaTypeString(criteria->Entry->Type), achievement->ID);
        return false;
    }

    // Don't update realm first achievements if the player's account isn't allowed to do so
    if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
        if (referencePlayer->GetSession()->HasPermission(rbac::RBAC_PERM_CANNOT_EARN_REALM_FIRST_ACHIEVEMENTS))
            return false;

    if (achievement->CovenantID && referencePlayer->m_playerData->CovenantID != achievement->CovenantID)
    {
        TC_LOG_TRACE("criteria.achievement", "AchievementMgr::CanUpdateCriteriaTree: (Id: {} Type {} Achievement {}) Wrong covenant",
            criteria->ID, CriteriaMgr::GetCriteriaTypeString(criteria->Entry->Type), achievement->ID);
        return false;
    }

    return CriteriaHandler::CanUpdateCriteriaTree(criteria, tree, referencePlayer);
}

bool AchievementMgr::CanCompleteCriteriaTree(CriteriaTree const* tree)
{
    AchievementEntry const* achievement = tree->Achievement;
    if (!achievement)
        return false;

    // counter can never complete
    if (achievement->Flags & ACHIEVEMENT_FLAG_COUNTER)
        return false;

    if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
    {
        // someone on this realm has already completed that achievement
        if (sAchievementMgr->IsRealmCompleted(achievement))
            return false;
    }

    return true;
}

void AchievementMgr::CompletedCriteriaTree(CriteriaTree const* tree, Player* referencePlayer)
{
    AchievementEntry const* achievement = tree->Achievement;
    if (!achievement)
        return;

    // counter can never complete
    if (achievement->Flags & ACHIEVEMENT_FLAG_COUNTER)
        return;

    // already completed and stored
    if (HasAchieved(achievement->ID))
        return;

    if (IsCompletedAchievement(achievement))
        CompletedAchievement(achievement, referencePlayer);
}

void AchievementMgr::AfterCriteriaTreeUpdate(CriteriaTree const* tree, Player* referencePlayer)
{
    AchievementEntry const* achievement = tree->Achievement;
    if (!achievement)
        return;

    // check again the completeness for SUMM and REQ COUNT achievements,
    // as they don't depend on the completed criteria but on the sum of the progress of each individual criteria
    if (achievement->Flags & ACHIEVEMENT_FLAG_SUMM)
        if (IsCompletedAchievement(achievement))
            CompletedAchievement(achievement, referencePlayer);

    if (std::vector<AchievementEntry const*> const* achRefList = sAchievementMgr->GetAchievementByReferencedId(achievement->ID))
        for (AchievementEntry const* refAchievement : *achRefList)
            if (IsCompletedAchievement(refAchievement))
                CompletedAchievement(refAchievement, referencePlayer);
}

bool AchievementMgr::IsCompletedAchievement(AchievementEntry const* entry)
{
    // counter can never complete
    if (entry->Flags & ACHIEVEMENT_FLAG_COUNTER)
        return false;

    CriteriaTree const* tree = sCriteriaMgr->GetCriteriaTree(entry->CriteriaTree);
    if (!tree)
        return false;

    // For SUMM achievements, we have to count the progress of each criteria of the achievement.
    // Oddly, the target count is NOT contained in the achievement, but in each individual criteria
    if (entry->Flags & ACHIEVEMENT_FLAG_SUMM)
    {
        int64 progress = 0;
        CriteriaMgr::WalkCriteriaTree(tree, [this, &progress](CriteriaTree const* criteriaTree)
        {
            if (criteriaTree->Criteria)
                if (CriteriaProgress const* criteriaProgress = this->GetCriteriaProgress(criteriaTree->Criteria))
                    progress += criteriaProgress->Counter;
        });
        return progress >= tree->Entry->Amount;
    }

    return IsCompletedCriteriaTree(tree);
}

bool AchievementMgr::RequiredAchievementSatisfied(uint32 achievementId) const
{
    return HasAchieved(achievementId);
}

AccountAchievementMgr::AccountAchievementMgr(WorldSession* owner) : _owner(owner)
{
}

void AccountAchievementMgr::Reset()
{
    AchievementMgr::Reset();

    for (auto const& completedAchievement : _completedAchievements)
    {
        WorldPackets::Achievement::AchievementDeleted achievementDeleted;
        achievementDeleted.AchievementID = completedAchievement.first;
        SendPacket(achievementDeleted.Write());
    }

    _completedAchievements.clear();
    _achievementPoints = 0;

    LoginDatabaseTransaction trans = LoginDatabase.BeginTransaction();
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_BNET_ACCOUNT_ACHIEVEMENT);
    stmt->setUInt32(0, _owner->GetBattlenetAccountId());
    trans->Append(stmt);

    stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_BNET_ACCOUNT_ACHIEVEMENT_PROGRESS);
    stmt->setUInt32(0, _owner->GetBattlenetAccountId());
    trans->Append(stmt);
    LoginDatabase.CommitTransaction(trans);

    if (Player* player = _owner->GetPlayer())
        CheckAllAchievementCriteria(player);
}

void AccountAchievementMgr::LoadFromDB(PreparedQueryResult achievementResult, PreparedQueryResult criteriaResult)
{
    if (achievementResult)
    {
        do
        {
            Field* fields = achievementResult->Fetch();
            uint32 achievementId = fields[0].GetUInt32();
            AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementId);
            if (!achievement || !(achievement->Flags & ACHIEVEMENT_FLAG_ACCOUNT))
                continue;

            CompletedAchievementData& completedAchievement = _completedAchievements[achievementId];
            completedAchievement.Date = fields[1].GetInt64();
            completedAchievement.Changed = false;
            _achievementPoints += achievement->Points;
        } while (achievementResult->NextRow());
    }

    if (criteriaResult)
    {
        time_t now = GameTime::GetGameTime();
        do
        {
            Field* fields = criteriaResult->Fetch();
            uint32 criteriaId = fields[0].GetUInt32();
            Criteria const* criteria = sCriteriaMgr->GetCriteria(criteriaId);
            if (!criteria || !(criteria->FlagsCu & CRITERIA_FLAG_CU_ACCOUNT))
                continue;

            time_t date = fields[2].GetInt64();
            if (criteria->Entry->StartTimer && time_t(date + criteria->Entry->StartTimer) < now)
                continue;

            CriteriaProgress& progress = _criteriaProgress[criteriaId];
            progress.Counter = fields[1].GetUInt64();
            progress.Date = date;
            progress.PlayerGUID = _owner->GetBattlenetAccountGUID();
            progress.Changed = false;
        } while (criteriaResult->NextRow());
    }
}

void AccountAchievementMgr::SaveToDB(LoginDatabaseTransaction trans)
{
    uint32 const battlenetAccountId = _owner->GetBattlenetAccountId();
    if (!battlenetAccountId)
        return;

    // Persist only changed rows. Never DELETE the whole battlenet account's achievement set and
    // rewrite it from this session's memory — playerbots (and any future multi-session account use)
    // create one WorldSession/AccountAchievementMgr per character. A stale sibling session that
    // dirties any criteria would otherwise wipe achievements/progress earned on another live session.
    // Mirror PlayerAchievementMgr::SaveToDB's per-row delete+insert contract.
    std::vector<uint32> dirtyAchievements;
    std::vector<uint32> dirtyCriteria;
    CollectChangedAccountAchievementSaveTargets(_completedAchievements, _criteriaProgress, dirtyAchievements, dirtyCriteria);

    for (uint32 achievementId : dirtyAchievements)
    {
        CompletedAchievementData& completedAchievement = _completedAchievements.at(achievementId);

        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_BNET_ACCOUNT_ACHIEVEMENT_BY_ACHIEVEMENT);
        stmt->setUInt32(0, battlenetAccountId);
        stmt->setUInt32(1, achievementId);
        trans->Append(stmt);

        stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_BNET_ACCOUNT_ACHIEVEMENT);
        stmt->setUInt32(0, battlenetAccountId);
        stmt->setUInt32(1, achievementId);
        stmt->setInt64(2, completedAchievement.Date);
        trans->Append(stmt);

        completedAchievement.Changed = false;
    }

    for (uint32 criteriaId : dirtyCriteria)
    {
        CriteriaProgress& progress = _criteriaProgress.at(criteriaId);

        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_BNET_ACCOUNT_ACHIEVEMENT_PROGRESS_BY_CRITERIA);
        stmt->setUInt32(0, battlenetAccountId);
        stmt->setUInt32(1, criteriaId);
        trans->Append(stmt);

        if (progress.Counter)
        {
            stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_BNET_ACCOUNT_ACHIEVEMENT_PROGRESS);
            stmt->setUInt32(0, battlenetAccountId);
            stmt->setUInt32(1, criteriaId);
            stmt->setUInt64(2, progress.Counter);
            stmt->setInt64(3, progress.Date);
            trans->Append(stmt);
        }

        progress.Changed = false;
    }
}

void AccountAchievementMgr::MigrateLegacyCharacterData(uint32 gameAccountId)
{
    if (!_owner->GetBattlenetAccountId())
        return;

    std::vector<uint32> accountAchievementIds;
    if (QueryResult result = CharacterDatabase.PQuery(
        "SELECT ca.achievement, MIN(ca.date) FROM character_achievement ca "
        "INNER JOIN characters c ON c.guid = ca.guid WHERE c.account = {} GROUP BY ca.achievement", gameAccountId))
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 achievementId = fields[0].GetUInt32();
            AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementId);
            if (!achievement || !(achievement->Flags & ACHIEVEMENT_FLAG_ACCOUNT))
                continue;

            accountAchievementIds.push_back(achievementId);
            if (HasAchieved(achievementId))
                continue;

            CompletedAchievementData& completedAchievement = _completedAchievements[achievementId];
            completedAchievement.Date = fields[1].GetInt64();
            completedAchievement.Changed = true;
            _achievementPoints += achievement->Points;
        } while (result->NextRow());
    }

    std::vector<uint32> accountOnlyCriteriaIds;
    if (QueryResult result = CharacterDatabase.PQuery(
        "SELECT cap.criteria, SUM(cap.counter), MAX(cap.date) FROM character_achievement_progress cap "
        "INNER JOIN characters c ON c.guid = cap.guid WHERE c.account = {} GROUP BY cap.criteria", gameAccountId))
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 criteriaId = fields[0].GetUInt32();
            Criteria const* criteria = sCriteriaMgr->GetCriteria(criteriaId);
            if (!criteria || !(criteria->FlagsCu & CRITERIA_FLAG_CU_ACCOUNT))
                continue;

            if (!(criteria->FlagsCu & CRITERIA_FLAG_CU_PLAYER))
                accountOnlyCriteriaIds.push_back(criteriaId);

            MergeLegacyAccountCriteriaProgress(_criteriaProgress, criteria, fields[1].GetUInt64(),
                fields[2].GetInt64(), _owner->GetBattlenetAccountGUID());
        } while (result->NextRow());
    }

    bool changed = std::ranges::any_of(_completedAchievements, [](auto const& pair) { return pair.second.Changed; }) ||
        std::ranges::any_of(_criteriaProgress, [](auto const& pair) { return pair.second.Changed; });
    if (changed)
    {
        LoginDatabaseTransaction trans = LoginDatabase.BeginTransaction();
        SaveToDB(trans);
        LoginDatabase.DirectCommitTransaction(trans);
    }

    auto makeIdList = [](std::vector<uint32> const& ids)
    {
        std::string values;
        for (uint32 id : ids)
        {
            if (!values.empty())
                values += ',';
            Trinity::StringFormatTo(std::back_inserter(values), "{}", id);
        }
        return values;
    };

    if (!accountAchievementIds.empty())
        CharacterDatabase.DirectPExecute(
            "DELETE ca FROM character_achievement ca INNER JOIN characters c ON c.guid = ca.guid "
            "WHERE c.account = {} AND ca.achievement IN ({})", gameAccountId, makeIdList(accountAchievementIds));

    if (!accountOnlyCriteriaIds.empty())
        CharacterDatabase.DirectPExecute(
            "DELETE cap FROM character_achievement_progress cap INNER JOIN characters c ON c.guid = cap.guid "
            "WHERE c.account = {} AND cap.criteria IN ({})", gameAccountId, makeIdList(accountOnlyCriteriaIds));
}

void AccountAchievementMgr::SendAllData(Player const* /*receiver*/) const
{
    WorldPackets::Achievement::AllAccountCriteria allAccountCriteria;
    WorldPackets::Achievement::AllAchievementData achievementData;
    AppendAllData(achievementData, allAccountCriteria);

    if (!allAccountCriteria.Progress.empty())
        SendPacket(allAccountCriteria.Write());

    SendPacket(achievementData.Write());
}

void AccountAchievementMgr::AppendAllData(WorldPackets::Achievement::AllAchievementData& achievementData,
    WorldPackets::Achievement::AllAccountCriteria& allAccountCriteria) const
{
    VisibleAchievementCheck filterInvisible;
    achievementData.Data.Earned.reserve(achievementData.Data.Earned.size() + _completedAchievements.size());
    allAccountCriteria.Progress.reserve(allAccountCriteria.Progress.size() + _criteriaProgress.size());

    for (auto const& completedAchievement : _completedAchievements)
    {
        if (!filterInvisible(completedAchievement))
            continue;

        WorldPackets::Achievement::EarnedAchievement& earned = achievementData.Data.Earned.emplace_back();
        earned.Id = completedAchievement.first;
        earned.Date.SetUtcTimeFromUnixTime(completedAchievement.second.Date);
        earned.Date += _owner->GetTimezoneOffset();
    }

    for (auto const& [criteriaId, criteriaProgress] : _criteriaProgress)
    {
        WorldPackets::Achievement::CriteriaProgress& progress = allAccountCriteria.Progress.emplace_back();
        progress.Id = criteriaId;
        progress.Quantity = criteriaProgress.Counter;
        progress.Player = _owner->GetBattlenetAccountGUID();
        progress.Flags = 0;
        progress.Date.SetUtcTimeFromUnixTime(criteriaProgress.Date);
        progress.Date += _owner->GetTimezoneOffset();
        progress.TimeFromStart = Seconds::zero();
        progress.TimeFromCreate = Seconds::zero();
    }
}

void AccountAchievementMgr::ApplyRetroactiveRewards(Player* player) const
{
    for (auto const& completedAchievement : _completedAchievements)
    {
        AchievementEntry const* achievement = sAchievementStore.LookupEntry(completedAchievement.first);
        AchievementReward const* reward = achievement ? sAchievementMgr->GetAchievementReward(achievement) : nullptr;
        if (!reward)
            continue;

        if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(GetAchievementRewardTitleId(achievement, reward, player)))
            player->SetTitle(titleEntry);
    }
}

void AccountAchievementMgr::CompletedAchievement(AchievementEntry const* achievement, Player* referencePlayer)
{
    if (!_owner->GetBattlenetAccountId() || !(achievement->Flags & ACHIEVEMENT_FLAG_ACCOUNT) || referencePlayer->IsGameMaster() ||
        referencePlayer->GetSession()->HasPermission(rbac::RBAC_PERM_CANNOT_EARN_ACHIEVEMENTS))
        return;

    if ((achievement->Faction == ACHIEVEMENT_FACTION_HORDE && referencePlayer->GetTeam() != HORDE) ||
        (achievement->Faction == ACHIEVEMENT_FACTION_ALLIANCE && referencePlayer->GetTeam() != ALLIANCE))
        return;

    if (achievement->Flags & ACHIEVEMENT_FLAG_COUNTER || HasAchieved(achievement->ID))
        return;

    if (achievement->Flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_NEWS)
        if (Guild* guild = referencePlayer->GetGuild())
            guild->AddGuildNews(GUILD_NEWS_PLAYER_ACHIEVEMENT, referencePlayer->GetGUID(), achievement->Flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_HEADER, achievement->ID);

    if (!_owner->PlayerLoading())
        SendAchievementEarned(achievement);

    TC_LOG_INFO("criteria.achievement", "AccountAchievementMgr::CompletedAchievement({}). {}", achievement->ID, GetOwnerInfo());

    CompletedAchievementData& completedAchievement = _completedAchievements[achievement->ID];
    completedAchievement.Date = GameTime::GetGameTime();
    completedAchievement.Changed = true;

    if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
        sAchievementMgr->SetRealmCompleted(achievement);

    if (!(achievement->Flags & ACHIEVEMENT_FLAG_TRACKING_FLAG))
        _achievementPoints += achievement->Points;

    referencePlayer->UpdateCriteria(CriteriaType::EarnAchievement, achievement->ID, 0, 0, nullptr);
    referencePlayer->UpdateCriteria(CriteriaType::EarnAchievementPoints, achievement->Points, 0, 0, nullptr);
    sScriptMgr->OnAchievementCompleted(referencePlayer, achievement);
    RewardAchievement(referencePlayer, achievement);

    // Sibling sessions still hold a pre-completion HasAchieved=false snapshot. Without an immediate
    // publish they can CompletedAchievement + RewardAchievement again (duplicate mail/items) and
    // SaveToDB a second completion race. Mirror completion into siblings without re-rewarding.
    sWorld->PublishSiblingAccountAchievementCompleted(_owner, achievement, completedAchievement.Date);
}

void AccountAchievementMgr::ApplyPublishedCriteriaProgress(uint32 criteriaId, uint64 counter, std::time_t date)
{
    if (!ApplySiblingAccountCriteriaProgress(_criteriaProgress, criteriaId, counter, date, _owner->GetBattlenetAccountGUID()))
        return;

    CriteriaProgress const* progress = &_criteriaProgress[criteriaId];
    if (!counter)
    {
        SendCriteriaProgressRemoved(criteriaId);
        return;
    }

    if (Criteria const* criteria = sCriteriaMgr->GetCriteria(criteriaId))
        SendCriteriaUpdate(criteria, progress, Seconds::zero(), false);
}

void AccountAchievementMgr::ApplyPublishedCompletedAchievement(AchievementEntry const* achievement, std::time_t date)
{
    if (!achievement)
        return;

    if (!ApplySiblingAccountCompletedAchievement(_completedAchievements, _achievementPoints, achievement->ID, date,
            achievement->Points, achievement->Flags & ACHIEVEMENT_FLAG_TRACKING_FLAG))
        return;

    if (Player* player = _owner->GetPlayer())
    {
        if (!_owner->PlayerLoading())
            SendAchievementEarned(achievement);

        // Titles only — RewardAchievement mail/items already ran on the publishing session.
        if (AchievementReward const* reward = sAchievementMgr->GetAchievementReward(achievement))
            if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(GetAchievementRewardTitleId(achievement, reward, player)))
                player->SetTitle(titleEntry);
    }
}

void AccountAchievementMgr::AfterCriteriaProgressChanged(Criteria const* criteria, CriteriaProgress const* progress)
{
    if (!criteria || !progress || !_owner->GetBattlenetAccountId())
        return;

    if (!(criteria->FlagsCu & CRITERIA_FLAG_CU_ACCOUNT))
        return;

    // Publish absolute counter so sibling ACCUMULATE bases cannot last-write-wins regress DB progress.
    sWorld->PublishSiblingAccountCriteriaProgress(_owner, criteria->ID, progress->Counter, progress->Date);
}

bool AccountAchievementMgr::CanUpdateCriteriaTree(Criteria const* criteria, CriteriaTree const* tree, Player* referencePlayer) const
{
    if (!_owner->GetBattlenetAccountId() || !IsCriteriaTreeForOwner(tree, AchievementCriteriaOwner::Account))
        return false;

    return AchievementMgr::CanUpdateCriteriaTree(criteria, tree, referencePlayer);
}

void AccountAchievementMgr::SendCriteriaUpdate(Criteria const* criteria, CriteriaProgress const* progress, Seconds timeElapsed, bool timedCompleted) const
{
    WorldPackets::Achievement::AccountCriteriaUpdate criteriaUpdate;
    criteriaUpdate.Progress.Id = criteria->ID;
    criteriaUpdate.Progress.Quantity = progress->Counter;
    criteriaUpdate.Progress.Player = _owner->GetBattlenetAccountGUID();
    criteriaUpdate.Progress.Flags = criteria->Entry->StartTimer && timedCompleted ? 1 : 0;
    criteriaUpdate.Progress.Date.SetUtcTimeFromUnixTime(progress->Date);
    criteriaUpdate.Progress.Date += _owner->GetTimezoneOffset();
    criteriaUpdate.Progress.TimeFromStart = timeElapsed;
    criteriaUpdate.Progress.TimeFromCreate = Seconds::zero();
    SendPacket(criteriaUpdate.Write());
}

void AccountAchievementMgr::SendCriteriaProgressRemoved(uint32 criteriaId)
{
    WorldPackets::Achievement::AccountCriteriaUpdate criteriaUpdate;
    criteriaUpdate.Progress.Id = criteriaId;
    criteriaUpdate.Progress.Quantity = 0;
    criteriaUpdate.Progress.Player = _owner->GetBattlenetAccountGUID();
    criteriaUpdate.Progress.Date = *GameTime::GetUtcWowTime();
    criteriaUpdate.Progress.Date += _owner->GetTimezoneOffset();
    criteriaUpdate.Progress.TimeFromStart = Seconds::zero();
    criteriaUpdate.Progress.TimeFromCreate = Seconds::zero();
    SendPacket(criteriaUpdate.Write());
}

void AccountAchievementMgr::SendAchievementEarned(AchievementEntry const* achievement) const
{
    if (Player* player = _owner->GetPlayer())
        SendAchievementEarnedForPlayer(player, achievement);
}

void AccountAchievementMgr::SendPacket(WorldPacket const* data) const
{
    _owner->SendPacket(data);
}

std::string AccountAchievementMgr::GetOwnerInfo() const
{
    return Trinity::StringFormat("Battle.net account {}", _owner->GetBattlenetAccountId());
}

CriteriaList const& AccountAchievementMgr::GetCriteriaByType(CriteriaType type, uint32 asset) const
{
    return sCriteriaMgr->GetAccountCriteriaByType(type, asset);
}

PlayerAchievementMgr::PlayerAchievementMgr(Player* owner) : _owner(owner)
{
}

void PlayerAchievementMgr::Reset()
{
    AchievementMgr::Reset();

    for (std::pair<uint32 const, CompletedAchievementData> const& completedAchievement : _completedAchievements)
    {
        WorldPackets::Achievement::AchievementDeleted achievementDeleted;
        achievementDeleted.AchievementID = completedAchievement.first;
        SendPacket(achievementDeleted.Write());
    }

    _completedAchievements.clear();
    _achievementPoints = 0;
    DeleteFromDB(_owner->GetGUID());

    // re-fill data
    CheckAllAchievementCriteria(_owner);
}

void PlayerAchievementMgr::DeleteFromDB(ObjectGuid const& guid)
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT);
    stmt->setUInt64(0, guid.GetCounter());
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT_PROGRESS);
    stmt->setUInt64(0, guid.GetCounter());
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

void PlayerAchievementMgr::LoadFromDB(PreparedQueryResult achievementResult, PreparedQueryResult criteriaResult)
{
    if (achievementResult)
    {
        do
        {
            Field* fields = achievementResult->Fetch();
            uint32 achievementid = fields[0].GetUInt32();

            // must not happen: cleanup at server startup in sAchievementMgr->LoadCompletedAchievements()
            AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementid);
            if (!achievement || achievement->Flags & (ACHIEVEMENT_FLAG_ACCOUNT | ACHIEVEMENT_FLAG_GUILD))
                continue;

            CompletedAchievementData& ca = _completedAchievements[achievementid];
            ca.Date = fields[1].GetInt64();
            ca.Changed = false;

            _achievementPoints += achievement->Points;

            // title achievement rewards are retroactive
            if (AchievementReward const* reward = sAchievementMgr->GetAchievementReward(achievement))
                if (uint32 titleId = reward->TitleId[Player::TeamForRace(_owner->GetRace()) == ALLIANCE ? 0 : 1])
                    if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(titleId))
                        _owner->SetTitle(titleEntry);

        } while (achievementResult->NextRow());
    }

    if (criteriaResult)
    {
        time_t now = GameTime::GetGameTime();
        do
        {
            Field* fields = criteriaResult->Fetch();
            uint32 id = fields[0].GetUInt32();
            uint64 counter = fields[1].GetUInt64();
            time_t date = fields[2].GetInt64();

            Criteria const* criteria = sCriteriaMgr->GetCriteria(id);
            if (!criteria)
            {
                // Removing non-existing criteria data for all characters
                TC_LOG_ERROR("criteria.achievement", "Non-existing achievement criteria {} data has been removed from the table `character_achievement_progress`.", id);

                CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_ACHIEV_PROGRESS_CRITERIA);
                stmt->setUInt32(0, id);
                CharacterDatabase.Execute(stmt);

                continue;
            }

            if (!(criteria->FlagsCu & CRITERIA_FLAG_CU_PLAYER))
                continue;

            if (criteria->Entry->StartTimer && time_t(date + criteria->Entry->StartTimer) < now)
                continue;

            CriteriaProgress& progress = _criteriaProgress[id];
            progress.Counter = counter;
            progress.Date = date;
            progress.PlayerGUID = _owner->GetGUID();
            progress.Changed = false;
        } while (criteriaResult->NextRow());
    }
}

void PlayerAchievementMgr::SaveToDB(CharacterDatabaseTransaction trans)
{
    if (!_completedAchievements.empty())
    {
        for (std::pair<uint32 const, CompletedAchievementData>& completedAchievement : _completedAchievements)
        {
            if (!completedAchievement.second.Changed)
                continue;

            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT_BY_ACHIEVEMENT);
            stmt->setUInt32(0, completedAchievement.first);
            stmt->setUInt64(1, _owner->GetGUID().GetCounter());
            trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_ACHIEVEMENT);
            stmt->setUInt64(0, _owner->GetGUID().GetCounter());
            stmt->setUInt32(1, completedAchievement.first);
            stmt->setInt64(2, completedAchievement.second.Date);
            trans->Append(stmt);

            completedAchievement.second.Changed = false;
        }
    }

    if (!_criteriaProgress.empty())
    {
        for (std::pair<uint32 const, CriteriaProgress>& criteriaProgres : _criteriaProgress)
        {
            if (!criteriaProgres.second.Changed)
                continue;

            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT_PROGRESS_BY_CRITERIA);
            stmt->setUInt64(0, _owner->GetGUID().GetCounter());
            stmt->setUInt32(1, criteriaProgres.first);
            trans->Append(stmt);

            if (criteriaProgres.second.Counter)
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_ACHIEVEMENT_PROGRESS);
                stmt->setUInt64(0, _owner->GetGUID().GetCounter());
                stmt->setUInt32(1, criteriaProgres.first);
                stmt->setUInt64(2, criteriaProgres.second.Counter);
                stmt->setInt64(3, criteriaProgres.second.Date);
                trans->Append(stmt);
            }

            criteriaProgres.second.Changed = false;
        }
    }
}

void PlayerAchievementMgr::SendAllData(Player const* /*receiver*/) const
{
    VisibleAchievementCheck filterInvisible;
    WorldPackets::Achievement::AllAccountCriteria allAccountCriteria;

    WorldPackets::Achievement::AllAchievementData achievementData;
    achievementData.Data.Earned.reserve(_completedAchievements.size());
    achievementData.Data.Progress.reserve(_criteriaProgress.size());

    for (std::pair<uint32 const, CompletedAchievementData> const& completedAchievement : _completedAchievements)
    {
        AchievementEntry const* achievement = filterInvisible(completedAchievement);
        if (!achievement)
            continue;

        WorldPackets::Achievement::EarnedAchievement& earned = achievementData.Data.Earned.emplace_back();
        earned.Id = completedAchievement.first;
        earned.Date.SetUtcTimeFromUnixTime(completedAchievement.second.Date);
        earned.Date += _owner->GetSession()->GetTimezoneOffset();
        if (!(achievement->Flags & ACHIEVEMENT_FLAG_ACCOUNT))
        {
            earned.Owner = _owner->GetGUID();
            earned.VirtualRealmAddress = earned.NativeRealmAddress = _owner->m_playerData->VirtualPlayerRealm;
        }
    }

    for (std::pair<uint32 const, CriteriaProgress> const& criteriaProgres : _criteriaProgress)
    {
        WorldPackets::Achievement::CriteriaProgress& progress = achievementData.Data.Progress.emplace_back();
        progress.Id = criteriaProgres.first;
        progress.Quantity = criteriaProgres.second.Counter;
        progress.Player = criteriaProgres.second.PlayerGUID;
        progress.Flags = 0;
        progress.Date.SetUtcTimeFromUnixTime(criteriaProgres.second.Date);
        progress.Date += _owner->GetSession()->GetTimezoneOffset();
        progress.TimeFromStart = Seconds::zero();
        progress.TimeFromCreate = Seconds::zero();

    }

    _owner->GetSession()->GetAccountAchievementMgr()->AppendAllData(achievementData, allAccountCriteria);

    if (!allAccountCriteria.Progress.empty())
        SendPacket(allAccountCriteria.Write());

    SendPacket(achievementData.Write());
}

void PlayerAchievementMgr::SendAchievementInfo(Player* receiver, uint32 /*achievementId = 0 */) const
{
    VisibleAchievementCheck filterInvisible;
    WorldPackets::Achievement::RespondInspectAchievements inspectedAchievements;
    inspectedAchievements.Player = _owner->GetGUID();
    inspectedAchievements.Data.Earned.reserve(_completedAchievements.size());
    inspectedAchievements.Data.Progress.reserve(_criteriaProgress.size());

    for (std::pair<uint32 const, CompletedAchievementData> const& completedAchievement : _completedAchievements)
    {
        AchievementEntry const* achievement = filterInvisible(completedAchievement);
        if (!achievement)
            continue;

        WorldPackets::Achievement::EarnedAchievement& earned = inspectedAchievements.Data.Earned.emplace_back();
        earned.Id = completedAchievement.first;
        earned.Date.SetUtcTimeFromUnixTime(completedAchievement.second.Date);
        earned.Date += receiver->GetSession()->GetTimezoneOffset();
        if (!(achievement->Flags & ACHIEVEMENT_FLAG_ACCOUNT))
        {
            earned.Owner = _owner->GetGUID();
            earned.VirtualRealmAddress = earned.NativeRealmAddress = _owner->m_playerData->VirtualPlayerRealm;
        }
    }

    for (std::pair<uint32 const, CriteriaProgress> const& criteriaProgres : _criteriaProgress)
    {
        WorldPackets::Achievement::CriteriaProgress& progress = inspectedAchievements.Data.Progress.emplace_back();
        progress.Id = criteriaProgres.first;
        progress.Quantity = criteriaProgres.second.Counter;
        progress.Player = criteriaProgres.second.PlayerGUID;
        progress.Flags = 0;
        progress.Date.SetUtcTimeFromUnixTime(criteriaProgres.second.Date);
        progress.Date += receiver->GetSession()->GetTimezoneOffset();
        progress.TimeFromStart = Seconds::zero();
        progress.TimeFromCreate = Seconds::zero();
    }

    receiver->SendDirectMessage(inspectedAchievements.Write());
}

void PlayerAchievementMgr::CompletedAchievement(AchievementEntry const* achievement, Player* referencePlayer)
{
    if (achievement->Flags & (ACHIEVEMENT_FLAG_ACCOUNT | ACHIEVEMENT_FLAG_GUILD))
        return;

    // Disable for GameMasters with GM-mode enabled or for players that don't have the related RBAC permission
    if (_owner->IsGameMaster() || _owner->GetSession()->HasPermission(rbac::RBAC_PERM_CANNOT_EARN_ACHIEVEMENTS))
        return;

    if ((achievement->Faction == ACHIEVEMENT_FACTION_HORDE    && referencePlayer->GetTeam() != HORDE) ||
        (achievement->Faction == ACHIEVEMENT_FACTION_ALLIANCE && referencePlayer->GetTeam() != ALLIANCE))
        return;

    if (achievement->Flags & ACHIEVEMENT_FLAG_COUNTER || HasAchieved(achievement->ID))
        return;

    if (achievement->Flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_NEWS)
        if (Guild* guild = referencePlayer->GetGuild())
            guild->AddGuildNews(GUILD_NEWS_PLAYER_ACHIEVEMENT, referencePlayer->GetGUID(), achievement->Flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_HEADER, achievement->ID);

    if (!_owner->GetSession()->PlayerLoading())
        SendAchievementEarned(achievement);

    TC_LOG_INFO("criteria.achievement", "PlayerAchievementMgr::CompletedAchievement({}). {}", achievement->ID, GetOwnerInfo());

    CompletedAchievementData& ca = _completedAchievements[achievement->ID];
    ca.Date = GameTime::GetGameTime();
    ca.Changed = true;

    if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
        sAchievementMgr->SetRealmCompleted(achievement);

    if (!(achievement->Flags & ACHIEVEMENT_FLAG_TRACKING_FLAG))
        _achievementPoints += achievement->Points;

    referencePlayer->UpdateCriteria(CriteriaType::EarnAchievement, achievement->ID, 0, 0, nullptr);
    referencePlayer->UpdateCriteria(CriteriaType::EarnAchievementPoints, achievement->Points, 0, 0, nullptr);

    sScriptMgr->OnAchievementCompleted(referencePlayer, achievement);
    RewardAchievement(_owner, achievement);
}

bool PlayerAchievementMgr::ModifierTreeSatisfied(uint32 modifierTreeId) const
{
    if (ModifierTreeNode const* modifierTree = sCriteriaMgr->GetModifierTree(modifierTreeId))
        return ModifierTreeSatisfied(modifierTree, 0, 0, nullptr, _owner);

    return false;
}

bool PlayerAchievementMgr::CanUpdateCriteriaTree(Criteria const* criteria, CriteriaTree const* tree, Player* referencePlayer) const
{
    if (!IsCriteriaTreeForOwner(tree, AchievementCriteriaOwner::Player))
        return false;

    return AchievementMgr::CanUpdateCriteriaTree(criteria, tree, referencePlayer);
}

void PlayerAchievementMgr::SendCriteriaUpdate(Criteria const* criteria, CriteriaProgress const* progress, Seconds timeElapsed, bool timedCompleted) const
{
    if (criteria->FlagsCu & CRITERIA_FLAG_CU_PLAYER)
    {
        WorldPackets::Achievement::CriteriaUpdate criteriaUpdate;

        criteriaUpdate.CriteriaID = criteria->ID;
        criteriaUpdate.Quantity = progress->Counter;
        criteriaUpdate.PlayerGUID = _owner->GetGUID();
        criteriaUpdate.Flags = 0;
        if (criteria->Entry->StartTimer)
            criteriaUpdate.Flags = timedCompleted ? 1 : 0; // 1 is for keeping the counter at 0 in client

        criteriaUpdate.CurrentTime.SetUtcTimeFromUnixTime(progress->Date);
        criteriaUpdate.CurrentTime += _owner->GetSession()->GetTimezoneOffset();
        criteriaUpdate.ElapsedTime = timeElapsed;
        criteriaUpdate.CreationTime = 0;

        SendPacket(criteriaUpdate.Write());
    }
}

void PlayerAchievementMgr::SendCriteriaProgressRemoved(uint32 criteriaId)
{
    WorldPackets::Achievement::CriteriaDeleted criteriaDeleted;
    criteriaDeleted.CriteriaID = criteriaId;
    SendPacket(criteriaDeleted.Write());
}

void PlayerAchievementMgr::SendAchievementEarned(AchievementEntry const* achievement) const
{
    TC_LOG_DEBUG("criteria.achievement", "PlayerAchievementMgr::SendAchievementEarned({})", achievement->ID);
    SendAchievementEarnedForPlayer(_owner, achievement);
}

void PlayerAchievementMgr::SendPacket(WorldPacket const* data) const
{
    _owner->SendDirectMessage(data);
}

CriteriaList const& PlayerAchievementMgr::GetCriteriaByType(CriteriaType type, uint32 asset) const
{
    return sCriteriaMgr->GetPlayerCriteriaByType(type, asset);
}

GuildAchievementMgr::GuildAchievementMgr(Guild* owner) : _owner(owner)
{
}

void GuildAchievementMgr::Reset()
{
    AchievementMgr::Reset();

    ObjectGuid guid = _owner->GetGUID();
    for (std::pair<uint32 const, CompletedAchievementData> const& completedAchievement : _completedAchievements)
    {
        _owner->BroadcastWorker([&](Player const* receiver)
        {
            WorldPackets::Achievement::GuildAchievementDeleted guildAchievementDeleted;
            guildAchievementDeleted.AchievementID = completedAchievement.first;
            guildAchievementDeleted.GuildGUID = guid;
            guildAchievementDeleted.TimeDeleted = *GameTime::GetUtcWowTime();
            guildAchievementDeleted.TimeDeleted += receiver->GetSession()->GetTimezoneOffset();
            receiver->SendDirectMessage(guildAchievementDeleted.Write());
        });
    }

    _achievementPoints = 0;
    _completedAchievements.clear();
    DeleteFromDB(guid);
}

void GuildAchievementMgr::DeleteFromDB(ObjectGuid const& guid)
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ALL_GUILD_ACHIEVEMENTS);
    stmt->setUInt64(0, guid.GetCounter());
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ALL_GUILD_ACHIEVEMENT_CRITERIA);
    stmt->setUInt64(0, guid.GetCounter());
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

void GuildAchievementMgr::LoadFromDB(PreparedQueryResult achievementResult, PreparedQueryResult criteriaResult)
{
    if (achievementResult)
    {
        do
        {
            Field* fields = achievementResult->Fetch();
            uint32 achievementid = fields[0].GetUInt32();

            // must not happen: cleanup at server startup in sAchievementMgr->LoadCompletedAchievements()
            AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementid);
            if (!achievement)
                continue;

            CompletedAchievementData& ca = _completedAchievements[achievementid];
            ca.Date = fields[1].GetInt64();
            for (std::string_view guid : Trinity::Tokenize(fields[2].GetStringView(), ',', false))
                if (Optional<ObjectGuid::LowType> parsedGuid = Trinity::StringTo<ObjectGuid::LowType>(guid))
                    ca.CompletingPlayers.insert(ObjectGuid::Create<HighGuid::Player>(*parsedGuid));

            ca.Changed = false;

            _achievementPoints += achievement->Points;
        } while (achievementResult->NextRow());
    }

    if (criteriaResult)
    {
        time_t now = GameTime::GetGameTime();
        do
        {
            Field* fields = criteriaResult->Fetch();
            uint32 id = fields[0].GetUInt32();
            uint64 counter = fields[1].GetUInt64();
            time_t date = fields[2].GetInt64();
            ObjectGuid::LowType guid = fields[3].GetUInt64();

            Criteria const* criteria = sCriteriaMgr->GetCriteria(id);
            if (!criteria)
            {
                // we will remove not existed criteria for all guilds
                TC_LOG_ERROR("criteria.achievement", "Non-existing achievement criteria {} data removed from table `guild_achievement_progress`.", id);

                CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_ACHIEV_PROGRESS_CRITERIA_GUILD);
                stmt->setUInt32(0, id);
                CharacterDatabase.Execute(stmt);
                continue;
            }

            if (criteria->Entry->StartTimer && time_t(date + criteria->Entry->StartTimer) < now)
                continue;

            CriteriaProgress& progress = _criteriaProgress[id];
            progress.Counter = counter;
            progress.Date = date;
            progress.PlayerGUID = ObjectGuid::Create<HighGuid::Player>(guid);
            progress.Changed = false;
        } while (criteriaResult->NextRow());
    }
}

void GuildAchievementMgr::SaveToDB(CharacterDatabaseTransaction trans)
{
    CharacterDatabasePreparedStatement* stmt;
    for (std::pair<uint32 const, CompletedAchievementData>& completedAchievement : _completedAchievements)
    {
        if (!completedAchievement.second.Changed)
            continue;

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_ACHIEVEMENT);
        stmt->setUInt64(0, _owner->GetId());
        stmt->setUInt32(1, completedAchievement.first);
        trans->Append(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GUILD_ACHIEVEMENT);
        stmt->setUInt64(0, _owner->GetId());
        stmt->setUInt32(1, completedAchievement.first);
        stmt->setInt64(2, completedAchievement.second.Date);
        std::string guidstr;
        auto completersItr = completedAchievement.second.CompletingPlayers.begin();
        auto completersEnd = completedAchievement.second.CompletingPlayers.end();
        if (completersItr != completersEnd)
        {
            Trinity::StringFormatTo(std::back_inserter(guidstr), "{}", completersItr->GetCounter());
            while (++completersItr != completersEnd)
                Trinity::StringFormatTo(std::back_inserter(guidstr), ",{}", completersItr->GetCounter());
        }

        stmt->setString(3, std::move(guidstr));
        trans->Append(stmt);

        completedAchievement.second.Changed = false;
    }

    for (std::pair<uint32 const, CriteriaProgress>& criteriaProgres : _criteriaProgress)
    {
        if (!criteriaProgres.second.Changed)
            continue;

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_ACHIEVEMENT_CRITERIA);
        stmt->setUInt64(0, _owner->GetId());
        stmt->setUInt32(1, criteriaProgres.first);
        trans->Append(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GUILD_ACHIEVEMENT_CRITERIA);
        stmt->setUInt64(0, _owner->GetId());
        stmt->setUInt32(1, criteriaProgres.first);
        stmt->setUInt64(2, criteriaProgres.second.Counter);
        stmt->setInt64(3, criteriaProgres.second.Date);
        stmt->setUInt64(4, criteriaProgres.second.PlayerGUID.GetCounter());
        trans->Append(stmt);

        criteriaProgres.second.Changed = false;
    }
}

void GuildAchievementMgr::SendAllData(Player const* receiver) const
{
    VisibleAchievementCheck filterInvisible;
    WorldPackets::Achievement::AllGuildAchievements allGuildAchievements;
    allGuildAchievements.Earned.reserve(_completedAchievements.size());

    for (std::pair<uint32 const, CompletedAchievementData> const& completedAchievement : _completedAchievements)
    {
        AchievementEntry const* achievement = filterInvisible(completedAchievement);
        if (!achievement)
            continue;

        WorldPackets::Achievement::EarnedAchievement& earned = allGuildAchievements.Earned.emplace_back();
        earned.Id = completedAchievement.first;
        earned.Date.SetUtcTimeFromUnixTime(completedAchievement.second.Date);
        earned.Date += receiver->GetSession()->GetTimezoneOffset();
    }

    receiver->SendDirectMessage(allGuildAchievements.Write());
}

void GuildAchievementMgr::SendAchievementInfo(Player* receiver, uint32 achievementId /*= 0*/) const
{
    WorldPackets::Achievement::GuildCriteriaUpdate guildCriteriaUpdate;
    if (AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementId))
    {
        if (CriteriaTree const* tree = sCriteriaMgr->GetCriteriaTree(achievement->CriteriaTree))
        {
            CriteriaMgr::WalkCriteriaTree(tree, [this, &guildCriteriaUpdate, receiver](CriteriaTree const* node)
            {
                if (node->Criteria)
                {
                    auto progress = this->_criteriaProgress.find(node->Criteria->ID);
                    if (progress != this->_criteriaProgress.end())
                    {
                        WorldPackets::Achievement::GuildCriteriaProgress& guildCriteriaProgress = guildCriteriaUpdate.Progress.emplace_back();
                        guildCriteriaProgress.CriteriaID = node->Criteria->ID;
                        guildCriteriaProgress.DateCreated = 0;
                        guildCriteriaProgress.DateStarted = 0;
                        guildCriteriaProgress.DateUpdated.SetUtcTimeFromUnixTime(progress->second.Date);
                        guildCriteriaProgress.DateUpdated += receiver->GetSession()->GetTimezoneOffset();
                        guildCriteriaProgress.Quantity = progress->second.Counter;
                        guildCriteriaProgress.PlayerGUID = progress->second.PlayerGUID;
                        guildCriteriaProgress.Flags = 0;
                    }
                }
            });
        }
    }

    receiver->SendDirectMessage(guildCriteriaUpdate.Write());
}

void GuildAchievementMgr::SendAllTrackedCriterias(Player* receiver, std::set<uint32> const& trackedCriterias) const
{
    WorldPackets::Achievement::GuildCriteriaUpdate guildCriteriaUpdate;
    guildCriteriaUpdate.Progress.reserve(trackedCriterias.size());

    for (uint32 criteriaId : trackedCriterias)
    {
        auto progress = _criteriaProgress.find(criteriaId);
        if (progress == _criteriaProgress.end())
            continue;

        WorldPackets::Achievement::GuildCriteriaProgress& guildCriteriaProgress = guildCriteriaUpdate.Progress.emplace_back();
        guildCriteriaProgress.CriteriaID = criteriaId;
        guildCriteriaProgress.DateCreated = 0;
        guildCriteriaProgress.DateStarted = 0;
        guildCriteriaProgress.DateUpdated.SetUtcTimeFromUnixTime(progress->second.Date);
        guildCriteriaProgress.DateUpdated += receiver->GetSession()->GetTimezoneOffset();
        guildCriteriaProgress.Quantity = progress->second.Counter;
        guildCriteriaProgress.PlayerGUID = progress->second.PlayerGUID;
        guildCriteriaProgress.Flags = 0;
    }

    receiver->SendDirectMessage(guildCriteriaUpdate.Write());
}

void GuildAchievementMgr::SendAchievementMembers(Player* receiver, uint32 achievementId) const
{
    auto itr = _completedAchievements.find(achievementId);
    if (itr != _completedAchievements.end())
    {
        WorldPackets::Achievement::GuildAchievementMembers guildAchievementMembers;
        guildAchievementMembers.GuildGUID = _owner->GetGUID();
        guildAchievementMembers.AchievementID = achievementId;
        guildAchievementMembers.Member.reserve(itr->second.CompletingPlayers.size());
        for (ObjectGuid const& member : itr->second.CompletingPlayers)
            guildAchievementMembers.Member.emplace_back(member);

        receiver->SendDirectMessage(guildAchievementMembers.Write());
    }
}

void GuildAchievementMgr::CompletedAchievement(AchievementEntry const* achievement, Player* referencePlayer)
{
    TC_LOG_DEBUG("criteria.achievement", "GuildAchievementMgr::CompletedAchievement({})", achievement->ID);

    if (achievement->Flags & ACHIEVEMENT_FLAG_COUNTER || HasAchieved(achievement->ID))
        return;

    if (achievement->Flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_NEWS)
        if (Guild* guild = referencePlayer->GetGuild())
            guild->AddGuildNews(GUILD_NEWS_GUILD_ACHIEVEMENT, ObjectGuid::Empty, achievement->Flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_HEADER, achievement->ID);

    SendAchievementEarned(achievement);
    CompletedAchievementData& ca = _completedAchievements[achievement->ID];
    ca.Date = GameTime::GetGameTime();
    ca.Changed = true;

    if (achievement->Flags & ACHIEVEMENT_FLAG_SHOW_GUILD_MEMBERS)
    {
        if (referencePlayer->GetGuildId() == _owner->GetId())
            ca.CompletingPlayers.insert(referencePlayer->GetGUID());

        if (Group const* group = referencePlayer->GetGroup())
            for (GroupReference const& ref : group->GetMembers())
                if (ref.GetSource()->GetGuildId() == _owner->GetId())
                    ca.CompletingPlayers.insert(ref.GetSource()->GetGUID());
    }

    if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
        sAchievementMgr->SetRealmCompleted(achievement);

    if (!(achievement->Flags & ACHIEVEMENT_FLAG_TRACKING_FLAG))
        _achievementPoints += achievement->Points;

    UpdateCriteria(CriteriaType::EarnAchievement, achievement->ID, 0, 0, nullptr, referencePlayer);
    UpdateCriteria(CriteriaType::EarnAchievementPoints, achievement->Points, 0, 0, nullptr, referencePlayer);

    sScriptMgr->OnAchievementCompleted(referencePlayer, achievement);
}

void GuildAchievementMgr::SendCriteriaUpdate(Criteria const* entry, CriteriaProgress const* progress, Seconds /*timeElapsed*/, bool /*timedCompleted*/) const
{
    for (Player const* member : _owner->GetMembersTrackingCriteria(entry->ID))
    {
        WorldPackets::Achievement::GuildCriteriaUpdate guildCriteriaUpdate;
        WorldPackets::Achievement::GuildCriteriaProgress& guildCriteriaProgress = guildCriteriaUpdate.Progress.emplace_back();
        guildCriteriaProgress.CriteriaID = entry->ID;
        guildCriteriaProgress.DateCreated = 0;
        guildCriteriaProgress.DateStarted = 0;
        guildCriteriaProgress.DateUpdated.SetUtcTimeFromUnixTime(progress->Date);
        guildCriteriaProgress.DateUpdated += member->GetSession()->GetTimezoneOffset();
        guildCriteriaProgress.Quantity = progress->Counter;
        guildCriteriaProgress.PlayerGUID = progress->PlayerGUID;
        guildCriteriaProgress.Flags = 0;

        member->SendDirectMessage(guildCriteriaUpdate.Write());
    }
}

void GuildAchievementMgr::SendCriteriaProgressRemoved(uint32 criteriaId)
{
    WorldPackets::Achievement::GuildCriteriaDeleted guildCriteriaDeleted;
    guildCriteriaDeleted.GuildGUID = _owner->GetGUID();
    guildCriteriaDeleted.CriteriaID = criteriaId;
    SendPacket(guildCriteriaDeleted.Write());
}

void GuildAchievementMgr::SendAchievementEarned(AchievementEntry const* achievement) const
{
    if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_KILL | ACHIEVEMENT_FLAG_REALM_FIRST_REACH))
    {
        // broadcast realm first reached
        WorldPackets::Achievement::BroadcastAchievement serverFirstAchievement;
        serverFirstAchievement.Name = _owner->GetName();
        serverFirstAchievement.PlayerGUID = _owner->GetGUID();
        serverFirstAchievement.AchievementID = achievement->ID;
        serverFirstAchievement.GuildAchievement = true;
        sWorld->SendGlobalMessage(serverFirstAchievement.Write());
    }

    _owner->BroadcastWorker([&](Player const* receiver)
    {
        WorldPackets::Achievement::GuildAchievementEarned guildAchievementEarned;
        guildAchievementEarned.AchievementID = achievement->ID;
        guildAchievementEarned.GuildGUID = _owner->GetGUID();
        guildAchievementEarned.TimeEarned = *GameTime::GetUtcWowTime();
        guildAchievementEarned.TimeEarned += receiver->GetSession()->GetTimezoneOffset();
        receiver->SendDirectMessage(guildAchievementEarned.Write());
    });
}

void GuildAchievementMgr::SendPacket(WorldPacket const* data) const
{
    _owner->BroadcastPacket(data);
}

CriteriaList const& GuildAchievementMgr::GetCriteriaByType(CriteriaType type, uint32 /*asset*/) const
{
    return sCriteriaMgr->GetGuildCriteriaByType(type);
}

AchievementGlobalMgr::AchievementGlobalMgr() = default;
AchievementGlobalMgr::~AchievementGlobalMgr() = default;

std::string PlayerAchievementMgr::GetOwnerInfo() const
{
    return Trinity::StringFormat("{} {}", _owner->GetGUID().ToString(), _owner->GetName());
}

std::string GuildAchievementMgr::GetOwnerInfo() const
{
    return Trinity::StringFormat("Guild ID {} {}", _owner->GetId(), _owner->GetName());
}

AchievementGlobalMgr* AchievementGlobalMgr::Instance()
{
    static AchievementGlobalMgr instance;
    return &instance;
}

std::vector<AchievementEntry const*> const* AchievementGlobalMgr::GetAchievementByReferencedId(uint32 id) const
{
    auto itr = _achievementListByReferencedId.find(id);
    return itr != _achievementListByReferencedId.end() ? &itr->second : nullptr;
}

AchievementReward const* AchievementGlobalMgr::GetAchievementReward(AchievementEntry const* achievement) const
{
    auto iter = _achievementRewards.find(achievement->ID);
    return iter != _achievementRewards.end() ? &iter->second : nullptr;
}

AchievementRewardLocale const* AchievementGlobalMgr::GetAchievementRewardLocale(AchievementEntry const* achievement) const
{
    auto iter = _achievementRewardLocales.find(achievement->ID);
    return iter != _achievementRewardLocales.end() ? &iter->second : nullptr;
}

bool AchievementGlobalMgr::IsRealmCompleted(AchievementEntry const* achievement) const
{
    auto itr = _allCompletedAchievements.find(achievement->ID);
    if (itr == _allCompletedAchievements.end())
        return false;

    if (itr->second == SystemTimePoint ::min())
        return false;

    if (itr->second == SystemTimePoint::max())
        return true;

    // Allow completing the realm first kill for entire minute after first person did it
    // it may allow more than one group to achieve it (highly unlikely)
    // but apparently this is how blizz handles it as well
    if (achievement->Flags & ACHIEVEMENT_FLAG_REALM_FIRST_KILL)
        return (GameTime::GetSystemTime() - itr->second) > Minutes(1);

    return true;
}

void AchievementGlobalMgr::SetRealmCompleted(AchievementEntry const* achievement)
{
    if (IsRealmCompleted(achievement))
        return;

    _allCompletedAchievements[achievement->ID] = GameTime::GetSystemTime();
}

//==========================================================
void AchievementGlobalMgr::LoadAchievementReferenceList()
{
    uint32 oldMSTime = getMSTime();

    if (sAchievementStore.GetNumRows() == 0)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 achievement references.");
        return;
    }

    uint32 count = 0;

    for (uint32 entryId = 0; entryId < sAchievementStore.GetNumRows(); ++entryId)
    {
        AchievementEntry const* achievement = sAchievementStore.LookupEntry(entryId);
        if (!achievement || !achievement->SharesCriteria)
            continue;

        _achievementListByReferencedId[achievement->SharesCriteria].push_back(achievement);
        ++count;
    }

    DB2HotfixGenerator<AchievementEntry> hotfixes(sAchievementStore);

    // Once Bitten, Twice Shy (10 player) - Icecrown Citadel
    // Correct map requirement (currently has Ulduar); 6.0.3 note - it STILL has ulduar requirement
    hotfixes.ApplyHotfix(4539, [](AchievementEntry* achievement)
    {
        achievement->InstanceID = 631;
    });

    TC_LOG_INFO("server.loading", ">> Loaded {} achievement references in {} ms.", count, GetMSTimeDiffToNow(oldMSTime));
}

void AchievementGlobalMgr::LoadAchievementScripts()
{
    uint32 oldMSTime = getMSTime();

    _achievementScripts.clear();                            // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT AchievementId, ScriptName FROM achievement_scripts");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 achievement scripts. DB table `achievement_scripts` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 achievementId         = fields[0].GetUInt32();
        std::string_view scriptName  = fields[1].GetStringView();

        AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementId);
        if (!achievement)
        {
            TC_LOG_ERROR("sql.sql", "Table `achievement_scripts` contains non-existing Achievement (ID: {}), skipped.", achievementId);
            continue;
        }
        _achievementScripts[achievementId] = sObjectMgr->GetScriptId(scriptName);
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} achievement scripts in {} ms", _achievementScripts.size(), GetMSTimeDiffToNow(oldMSTime));
}

void AchievementGlobalMgr::LoadCompletedAchievements()
{
    uint32 oldMSTime = getMSTime();

    // Populate _allCompletedAchievements with all realm first achievement ids to make multithreaded access safer
    // while it will not prevent races, it will prevent crashes that happen because std::unordered_map key was added
    // instead the only potential race will happen on value associated with the key
    for (AchievementEntry const* achievement : sAchievementStore)
        if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
            _allCompletedAchievements[achievement->ID] = SystemTimePoint::min();

    QueryResult result = CharacterDatabase.Query("SELECT achievement FROM character_achievement GROUP BY achievement");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 realm first completed achievements. DB table `character_achievement` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 achievementId = fields[0].GetUInt32();
        AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementId);
        if (!achievement)
        {
            // Remove non-existing achievements from all characters
            TC_LOG_ERROR("criteria.achievement", "Non-existing achievement {} data has been removed from the table `character_achievement`.", achievementId);

            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_ACHIEVMENT);
            stmt->setUInt32(0, achievementId);
            CharacterDatabase.Execute(stmt);

            continue;
        }
        else if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
            _allCompletedAchievements[achievementId] = SystemTimePoint::max();
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} realm first completed achievements in {} ms.", (unsigned long)_allCompletedAchievements.size(), GetMSTimeDiffToNow(oldMSTime));
}

void AchievementGlobalMgr::LoadRewards()
{
    uint32 oldMSTime = getMSTime();

    _achievementRewards.clear();                           // need for reload case

    //                                               0   1       2       3       4       5        6     7
    QueryResult result = WorldDatabase.Query("SELECT ID, TitleA, TitleH, ItemID, Sender, Subject, Body, MailTemplateID FROM achievement_reward");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 achievement rewards. DB table `achievement_reward` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        uint32 id = fields[0].GetUInt32();
        AchievementEntry const* achievement = sAchievementStore.LookupEntry(id);
        if (!achievement)
        {
            TC_LOG_ERROR("sql.sql", "Table `achievement_reward` contains a wrong achievement ID ({}), ignored.", id);
            continue;
        }

        AchievementReward reward;
        reward.TitleId[0]       = fields[1].GetUInt32();
        reward.TitleId[1]       = fields[2].GetUInt32();
        reward.ItemId           = fields[3].GetUInt32();
        reward.SenderCreatureId = fields[4].GetUInt32();
        reward.Subject          = fields[5].GetString();
        reward.Body             = fields[6].GetString();
        reward.MailTemplateId   = fields[7].GetUInt32();

        // must be title or mail at least
        if (!reward.TitleId[0] && !reward.TitleId[1] && !reward.SenderCreatureId)
        {
            TC_LOG_ERROR("sql.sql", "Table `achievement_reward` (ID: {}) does not contain title or item reward data. Ignored.", id);
            continue;
        }

        if (achievement->Faction == ACHIEVEMENT_FACTION_ANY && (!reward.TitleId[0] ^ !reward.TitleId[1]))
            TC_LOG_ERROR("sql.sql", "Table `achievement_reward` (ID: {}) contains the title (A: {} H: {}) for only one team.", id, reward.TitleId[0], reward.TitleId[1]);

        if (reward.TitleId[0])
        {
            CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(reward.TitleId[0]);
            if (!titleEntry)
            {
                TC_LOG_ERROR("sql.sql", "Table `achievement_reward` (Entry: {}) contains an invalid title id ({}) in `title_A`, set to 0", id, reward.TitleId[0]);
                reward.TitleId[0] = 0;
            }
        }

        if (reward.TitleId[1])
        {
            CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(reward.TitleId[1]);
            if (!titleEntry)
            {
                TC_LOG_ERROR("sql.sql", "Table `achievement_reward` (Entry: {}) contains an invalid title id ({}) in `title_H`, set to 0", id, reward.TitleId[1]);
                reward.TitleId[1] = 0;
            }
        }

        //check mail data before item for report including wrong item case
        if (reward.SenderCreatureId)
        {
            if (!sObjectMgr->GetCreatureTemplate(reward.SenderCreatureId))
            {
                TC_LOG_ERROR("sql.sql", "Table `achievement_reward` (ID: {}) contains an invalid creature ID {} as sender, mail reward skipped.", id, reward.SenderCreatureId);
                reward.SenderCreatureId = 0;
            }
        }
        else
        {
            if (reward.ItemId)
                TC_LOG_ERROR("sql.sql", "Table `achievement_reward` (ID: {}) does not have sender data, but contains an item reward. Item will not be rewarded.", id);

            if (!reward.Subject.empty())
                TC_LOG_ERROR("sql.sql", "Table `achievement_reward` (ID: {}) does not have sender data, but contains a mail subject.", id);

            if (!reward.Body.empty())
                TC_LOG_ERROR("sql.sql", "Table `achievement_reward` (ID: {}) does not have sender data, but contains mail text.", id);

            if (reward.MailTemplateId)
                TC_LOG_ERROR("sql.sql", "Table `achievement_reward` (ID: {}) does not have sender data, but has a MailTemplate.", id);
        }

        if (reward.MailTemplateId)
        {
            if (!sMailTemplateStore.LookupEntry(reward.MailTemplateId))
            {
                TC_LOG_ERROR("sql.sql", "Table `achievement_reward` (ID: {}) is using an invalid MailTemplate ({}).", id, reward.MailTemplateId);
                reward.MailTemplateId = 0;
            }
            else if (!reward.Subject.empty() || !reward.Body.empty())
                TC_LOG_ERROR("sql.sql", "Table `achievement_reward` (ID: {}) is using MailTemplate ({}) and mail subject/text.", id, reward.MailTemplateId);
        }

        if (reward.ItemId)
        {
            if (!sObjectMgr->GetItemTemplate(reward.ItemId))
            {
                TC_LOG_ERROR("sql.sql", "Table `achievement_reward` (ID: {}) contains an invalid item id {}, reward mail will not contain the rewarded item.", id, reward.ItemId);
                reward.ItemId = 0;
            }
        }

        _achievementRewards[id] = reward;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} achievement rewards in {} ms.", uint32(_achievementRewards.size()), GetMSTimeDiffToNow(oldMSTime));
}

void AchievementGlobalMgr::LoadRewardLocales()
{
    uint32 oldMSTime = getMSTime();

    _achievementRewardLocales.clear();                       // need for reload case

    //                                               0   1       2        3
    QueryResult result = WorldDatabase.Query("SELECT ID, Locale, Subject, Body FROM achievement_reward_locale");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 achievement reward locale strings.  DB table `achievement_reward_locale` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 id = fields[0].GetUInt32();
        std::string_view localeName = fields[1].GetStringView();

        if (_achievementRewards.find(id) == _achievementRewards.end())
        {
            TC_LOG_ERROR("sql.sql", "Table `achievement_reward_locale` (ID: {}) contains locale strings for a non-existing achievement reward.", id);
            continue;
        }

        LocaleConstant locale = GetLocaleByName(localeName);
        if (!IsValidLocale(locale) || locale == LOCALE_enUS)
            continue;

        AchievementRewardLocale& data = _achievementRewardLocales[id];
        ObjectMgr::AddLocaleString(fields[2].GetStringView(), locale, data.Subject);
        ObjectMgr::AddLocaleString(fields[3].GetStringView(), locale, data.Body);
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} achievement reward locale strings in {} ms.", uint32(_achievementRewardLocales.size()), GetMSTimeDiffToNow(oldMSTime));
}

uint32 AchievementGlobalMgr::GetAchievementScriptId(uint32 achievementId) const
{
    if (uint32 const* scriptId = Trinity::Containers::MapGetValuePtr(_achievementScripts, achievementId))
        return *scriptId;
    return 0;
}

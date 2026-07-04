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

#include "RandomPlayerbotFactory.h"

#include "AccountMgr.h"
#include "BattlenetAccountMgr.h"
#include "CharacterCache.h"
#include "Containers.h"
#include "DB2Stores.h"
#include "DB2Structure.h"
#include "DBCEnums.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "MotionMaster.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "Random.h"
#include "RealmList.h"
#include "SharedDefines.h"
#include "World.h"
#include "WorldSession.h"
#include "CharacterPackets.h"

#include <algorithm>
#include <string>
#include <vector>

namespace
{
// Hero classes (DK / DH / Evoker) — detected data-first by the CLASS_* enum, never a hardcoded id
// list. They start in instanced intro scenarios (DK Acherus, DH Mardum, Evoker Forbidden Reach) the
// wander strategy can't path out of yet; generation still produces them (Session 2), and teaching
// bots to complete/exit those starting scenarios is future work (NYI, owner decision 2026-07-04).
bool IsHeroClass(uint8 cls)
{
    return cls == CLASS_DEATH_KNIGHT || cls == CLASS_DEMON_HUNTER || cls == CLASS_EVOKER;
}

bool IsGeneratableClass(uint8 cls)
{
    if (cls == CLASS_NONE || cls >= MAX_CLASSES)
        return false;
    if (!((1 << (cls - 1)) & CLASSMASK_ALL_PLAYABLE))
        return false;
    if (!sChrClassesStore.LookupEntry(cls))
        return false;

    // Optional curated include-list (data-first): if set, only these class ids generate. Mirrors
    // GetAllowedRaces; empty (default) = every class allowed by the gates below.
    std::vector<uint32> const allowed = Playerbots::GetAllowedClasses();
    if (!allowed.empty() && std::ranges::find(allowed, uint32(cls)) == allowed.end())
        return false;

    // Hero-class arm, AC-shaped (AC's IsValidRaceClassCombination expansion skip +
    // AiPlayerbot.DisableDeathKnightLogin). Gated behind EnableHeroClasses (master toggle) with a
    // DK-specific DisableDeathKnightLogin analog. TC-native: AC (WotLK) has DK only, so DH/Evoker are
    // necessarily fork additions kept AC-minded (config-driven). Start level/money come from
    // Player::Create (CONFIG_START_{DEATH_KNIGHT,DEMON_HUNTER,EVOKER}_PLAYER_LEVEL), never a literal.
    if (IsHeroClass(cls))
    {
        if (!Playerbots::GetEnableHeroClasses())
            return false;
        if (cls == CLASS_DEATH_KNIGHT && Playerbots::GetDisableDeathKnightLogin())
            return false;
    }

    if ((1 << (cls - 1)) & sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_CLASSMASK))
        return false;
    return true;
}

// Data-first race eligibility for generation. Allied races are detected by ChrRacesFlag::IsAlliedRace
// (the same flag Player::Create keys on) + the realm expansion — never a hardcoded {52,70,...} id
// list and never race_unlock_requirement.achievementId, which is the wrong signal both ways (it
// leaked non-allied special races that happen to carry achievementId 0, e.g. Dracthyr, and dropped
// real allied races that carry an unlock achievement). Base races start in the open world at the
// normal player level; allied races start at an open-world embassy (both wanderable). A non-allied
// race with an elevated start level is an instanced-scenario special race (Dracthyr — Forbidden
// Reach) that Session 1 does not generate; those go with the hero classes in Session 2, since the
// wander strategy cannot leave a starting scenario. See
// playerbots-special-races-classes-s1-allied-races-handoff.md.
bool IsGeneratableRace(uint8 race)
{
    if (race == RACE_NONE)
        return false;

    RaceUnlockRequirement const* req = sObjectMgr->GetRaceUnlockRequirement(race);
    if (!req)
        return false;  // races with no unlock row (drakes / NPC-only races) are never generatable
    if (req->Expansion > sWorld->getIntConfig(CONFIG_EXPANSION))
        return false;  // expansion gate (AC's IsValidRaceClassCombination expansion skip, data-first)

    Trinity::RaceMask<uint64> const disabled{ sWorld->GetUInt64Config(CONFIG_CHARACTER_CREATING_DISABLED_RACEMASK) };
    if (disabled.HasRace(race))
        return false;

    ChrRacesEntry const* chrRace = sChrRacesStore.LookupEntry(race);
    if (!chrRace)
        return false;

    // Optional curated include-list (data-first): if set, only these race ids generate.
    std::vector<uint32> const allowed = Playerbots::GetAllowedRaces();
    if (!allowed.empty() && std::ranges::find(allowed, uint32(race)) == allowed.end())
        return false;

    if (chrRace->GetFlags().HasFlag(ChrRacesFlag::IsAlliedRace))
        return Playerbots::GetEnableAlliedRaces();

    // Non-allied race: base races start at the normal player level. A higher StartingLevel marks an
    // instanced-scenario special race (Dracthyr — Forbidden Reach), detected data-first by the level
    // signal, never a hardcoded {52,70} id list. Dracthyr is Evoker's paired race, so it rides the
    // same gate as the hero classes: generatable only when hero classes are enabled (Session 2). Like
    // the hero classes it spawns in its intro scenario for now (scenario traversal is NYI — see
    // IsGeneratableClass). Its start level (10) still comes from Player::Create for free.
    if (chrRace->StartingLevel > int32(sWorld->getIntConfig(CONFIG_START_PLAYER_LEVEL)))
        return Playerbots::GetEnableHeroClasses();

    return true;
}

// AC-shaped (RandomPlayerbotFactory::IsValidRaceClassCombination) — a race/class pair is
// generatable iff the race and class are individually eligible and the pair is a real playable
// combination (sObjectMgr->GetPlayerInfo, which also encodes allied-race class restrictions).
// TC-native: the expansion / allied / hero gating lives in IsGeneratableRace + IsGeneratableClass.
bool IsValidRaceClassCombination(uint8 race, uint8 cls)
{
    return IsGeneratableRace(race) && IsGeneratableClass(cls) && sObjectMgr->GetPlayerInfo(race, cls) != nullptr;
}

std::string MakeAccountName(uint32 index)
{
    return Playerbots::GetRandomBotAccountPrefix() + std::to_string(index);
}

std::string MakeRandomPassword()
{
    std::string password;
    password.reserve(10);
    for (int i = 0; i < 10; ++i)
        password += static_cast<char>(urand('!', 'z'));
    return password;
}

// Simple, expandable syllable name generator (AC ships a syllable fallback too, so this is
// AC-precedented). V1 is race/gender-agnostic; the signature can later take race/gender to layer
// on per-race syllable sets. Validity + uniqueness are enforced by the caller.
std::string GenerateSyllableName()
{
    static char const consonants[] = "bcdfghjklmnprstvwz";
    static char const vowels[] = "aeiou";
    size_t const consonantCount = sizeof(consonants) - 1;
    size_t const vowelCount = sizeof(vowels) - 1;

    uint32 const syllables = urand(2, 3);
    std::string name;
    for (uint32 i = 0; i < syllables; ++i)
    {
        name += consonants[urand(0, consonantCount - 1)];
        name += vowels[urand(0, vowelCount - 1)];
        // occasional trailing consonant for a closed syllable, keeping the name pronounceable
        if (urand(0, 2) == 0 && name.size() <= 9)
            name += consonants[urand(0, consonantCount - 1)];
    }

    if (name.size() > 12)
        name.resize(12);
    name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
    return name;
}

std::string GenerateBotName()
{
    for (int tries = 0; tries < 20; ++tries)
    {
        std::string name = GenerateSyllableName();

        if (ObjectMgr::CheckPlayerName(name, LOCALE_enUS, true) != CHAR_NAME_SUCCESS)
            continue;

        // Uniqueness: the character cache holds every existing character (and every one this run
        // adds immediately after create), so a cache miss means the name is free — no DB round-trip.
        if (!sCharacterCache->GetCharacterGuidByName(name).IsEmpty())
            continue;

        return name;
    }

    return std::string();
}

// Builds a random-but-valid Customizations set for race/gender and confirms it with the same
// WorldSession::ValidateAppearance the client create path uses. Only unconditional options/choices
// (no ChrCustomizationReq requirements) are picked, which both guarantees validity without a logged-in
// Player and yields a sensible default look; gated cosmetic unlocks are simply omitted. No hardcoded
// choice ids (retail-data-first). See handoff § 6.1.
//
// Allied races are covered by this same path: DB2 evidence (build 12.0.7.67808) shows every allied
// race/gender has 8-29 unconditional options, so an unconditional-only set is always rich and valid.
// This function never drops a race on an appearance problem — a race with no options at all (a data
// anomaly) or a validation miss falls back to the default (empty) look, which ValidateAppearance
// accepts (it only validates the choices actually supplied). See handoff § 6.1.
bool BuildRandomAppearance(WorldSession* session, uint8 race, uint8 cls, uint8 gender,
    WorldPackets::Character::CharacterCreateInfo& createInfo)
{
    createInfo.Customizations.clear();

    std::vector<ChrCustomizationOptionEntry const*> const* options = sDB2Manager.GetCustomiztionOptions(race, gender);
    if (!options || options->empty())
    {
        // No customization options for this race/gender (data anomaly). ValidateAppearance accepts an
        // empty set, so create with the race default rather than dropping the race.
        TC_LOG_WARN("playerbots", "RandomPlayerbotFactory: race {} gender {} has no customization options — using default appearance.", race, gender);
        return true;
    }

    auto reqHasRequirements = [](int32 reqId) -> bool
    {
        if (reqId == 0)
            return false;
        ChrCustomizationReqEntry const* req = sChrCustomizationReqStore.LookupEntry(reqId);
        return req && req->GetFlags().HasFlag(ChrCustomizationReqFlag::HasRequirements);
    };

    for (int tries = 0; tries < 5; ++tries)
    {
        createInfo.Customizations.clear();
        for (ChrCustomizationOptionEntry const* option : *options)
        {
            if (reqHasRequirements(option->ChrCustomizationReqID))
                continue;

            std::vector<ChrCustomizationChoiceEntry const*> const* choices = sDB2Manager.GetCustomiztionChoices(option->ID);
            if (!choices || choices->empty())
                continue;

            std::vector<ChrCustomizationChoiceEntry const*> usable;
            usable.reserve(choices->size());
            for (ChrCustomizationChoiceEntry const* choice : *choices)
                if (!reqHasRequirements(choice->ChrCustomizationReqID))
                    usable.push_back(choice);

            if (usable.empty())
                continue;

            ChrCustomizationChoiceEntry const* pick = Trinity::Containers::SelectRandomContainerElement(usable);
            UF::ChrCustomizationChoice choiceOut;
            choiceOut.ChrCustomizationOptionID = option->ID;
            choiceOut.ChrCustomizationChoiceID = pick->ID;
            createInfo.Customizations.push_back(choiceOut);
        }

        if (session->ValidateAppearance(Races(race), Classes(cls), Gender(gender),
            MakeChrCustomizationChoiceRange(createInfo.Customizations)))
            return true;
    }

    // Unreachable in practice (an unconditional-only set always validates), but never drop the race
    // on a validation miss — fall back to the default (empty) look, which ValidateAppearance accepts.
    createInfo.Customizations.clear();
    TC_LOG_WARN("playerbots", "RandomPlayerbotFactory: could not randomize appearance for race {} gender {} — using default appearance.", race, gender);
    return true;
}

// Precomputes every generatable (race, class) combo, split by the faction usable for it. Neutral
// races (Pandaren) go into both buckets so faction balance can draw them either way.
void BuildValidCombos(std::vector<std::pair<uint8, uint8>>& alliance, std::vector<std::pair<uint8, uint8>>& horde)
{
    for (auto const& [race, requirement] : sObjectMgr->GetRaceUnlockRequirements())
    {
        (void)requirement;
        TeamId const team = Player::TeamIdForRace(race);
        for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls)
        {
            if (!IsValidRaceClassCombination(race, cls))
                continue;

            if (team == TEAM_ALLIANCE || team == TEAM_NEUTRAL)
                alliance.emplace_back(race, cls);
            if (team == TEAM_HORDE || team == TEAM_NEUTRAL)
                horde.emplace_back(race, cls);
        }
    }
}

bool CreateBotCharacter(WorldSession* session, uint32 accountId, bool preferAlliance,
    std::vector<std::pair<uint8, uint8>> const& alliance, std::vector<std::pair<uint8, uint8>> const& horde)
{
    std::vector<std::pair<uint8, uint8>> const& pool = preferAlliance ? alliance : horde;
    std::vector<std::pair<uint8, uint8>> const& fallback = preferAlliance ? horde : alliance;

    std::vector<std::pair<uint8, uint8>> const& combos = !pool.empty() ? pool : fallback;
    if (combos.empty())
        return false;

    auto const [race, cls] = Trinity::Containers::SelectRandomContainerElement(combos);
    uint8 const gender = urand(0, 1) ? GENDER_MALE : GENDER_FEMALE;

    std::string const name = GenerateBotName();
    if (name.empty())
    {
        TC_LOG_ERROR("playerbots", "RandomPlayerbotFactory: failed to generate a unique bot name.");
        return false;
    }

    WorldPackets::Character::CharacterCreateInfo createInfo;
    createInfo.Race = race;
    createInfo.Class = cls;
    createInfo.Sex = gender;
    createInfo.Name = name;

    // Fills createInfo.Customizations with a valid randomized look; never fails the race (falls back
    // to the default appearance on a data anomaly — see BuildRandomAppearance / handoff § 6.1).
    BuildRandomAppearance(session, race, cls, gender, createInfo);

    Player* bot = new Player(session);
    bot->GetMotionMaster()->Initialize();
    if (!bot->Create(sObjectMgr->GetGenerator<HighGuid::Player>().Generate(), &createInfo))
    {
        bot->CleanupsBeforeDelete();
        delete bot;
        TC_LOG_ERROR("playerbots", "RandomPlayerbotFactory: Player::Create failed for '{}' (race {}, class {}).", name, race, cls);
        return false;
    }

    bot->setCinematic(2);              // never play the intro cinematic
    bot->SetAtLoginFlag(AT_LOGIN_NONE);

    bot->SaveToDB(true);              // synchronous create-commit
    sCharacterCache->AddCharacterCacheEntry(bot->GetGUID(), accountId, bot->GetName(), bot->GetNativeGender(),
        bot->GetRace(), bot->GetClass(), bot->GetLevel(), false);

    bot->CleanupsBeforeDelete();
    delete bot;

    TC_LOG_DEBUG("playerbots", "RandomPlayerbotFactory: created bot '{}' (race {}, class {}, account {}).", name, race, cls, accountId);
    return true;
}

uint32 CreateBotAccount(std::string const& name)
{
    std::string const password = Playerbots::GetRandomBotRandomPassword() ? MakeRandomPassword() : name;

    // Provision a Bnet-linked game account so WorldSession::CreateForBot resolves a real
    // battlenetAccountId (no battlenet_* FK growth — see playerbots-bot-bnet-account-fk-handoff.md).
    // We create the Bnet account WITHOUT the auto "<id>#1" game account, then create our own
    // "<prefix><N>"-named game account linked to it, so the AC prefix model + prefix-aware reserved
    // check still apply.
    std::string const email = name + "@playerbot.local";

    if (!Battlenet::AccountMgr::GetId(email))
    {
        if (Battlenet::AccountMgr::CreateBattlenetAccount(email, password, false, nullptr) != AccountOpResult::AOR_OK)
        {
            TC_LOG_ERROR("playerbots", "RandomPlayerbotFactory: failed to create Battle.net account for '{}'.", name);
            return 0;
        }
    }

    uint32 const bnetId = Battlenet::AccountMgr::GetId(email);
    if (!bnetId)
    {
        TC_LOG_ERROR("playerbots", "RandomPlayerbotFactory: could not resolve Battle.net account id for '{}'.", name);
        return 0;
    }

    if (AccountMgr::instance()->CreateAccount(name, password, email, bnetId, 1) != AccountOpResult::AOR_OK)
    {
        TC_LOG_ERROR("playerbots", "RandomPlayerbotFactory: failed to create game account '{}'.", name);
        return 0;
    }

    return AccountMgr::GetId(name);
}

// Phase A shared plumbing (playerbots-special-races-classes-s1-allied-races-handoff.md § 5 Phase A;
// reused by Session 2). Unlocks all expansion content on a bot account by setting account.expansion
// to the realm's CONFIG_EXPANSION, so every reserved bot account has the special races/classes
// unlocked exactly like a current-expansion human account (the LOGIN_UPD_EXPANSION path used by
// ".account set expansion"). Bnet linkage is untouched, so WorldSession::CreateForBot still resolves
// a real battlenetAccountId (no battlenet_* FK regression). Idempotent: only writes when the stored
// value differs, so a re-run of an already-correct account touches nothing.
void EnsureBotAccountExpansion(uint32 accountId, uint8 expansion)
{
    if (QueryResult result = LoginDatabase.PQuery("SELECT expansion FROM account WHERE id = {}", accountId))
        if (result->Fetch()[0].GetUInt8() == expansion)
            return;

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_EXPANSION);
    stmt->setUInt8(0, expansion);
    stmt->setUInt32(1, accountId);
    LoginDatabase.Execute(stmt);
}

uint32 CalculateAccountCount(uint32 target, uint32 charsPerAccount)
{
    uint32 const divisor = charsPerAccount ? charsPerAccount : 10;  // AC hardcoded fallback
    uint32 const calculated = std::max<uint32>((target + divisor - 1) / divisor, 1u);

    uint32 const configured = Playerbots::GetRandomBotAccountCount();
    if (configured >= calculated)
        return configured;

    if (configured > 0)
        TC_LOG_WARN("playerbots", "RandomPlayerbotFactory: RandomBotAccountCount ({}) is below the required minimum ({}) — using the calculated value.",
            configured, calculated);

    return calculated;
}

// AC's deleteRandomBotAccounts analog: loudly delete every generated bot-account character and the
// bot accounts, then stop the server so the operator resets the flag. Off by default; never silent.
void RunTeardown()
{
    std::string const prefix = Playerbots::GetRandomBotAccountPrefix();
    if (prefix.empty())
    {
        TC_LOG_ERROR("playerbots", "RandomPlayerbotFactory: DeleteRandomBotAccounts set but RandomBotAccountPrefix is empty — refusing to delete (cannot identify bot accounts).");
        return;
    }

    TC_LOG_INFO("server.loading", "RandomPlayerbotFactory: DeleteRandomBotAccounts is set — deleting all generated bot accounts and characters...");

    std::vector<uint32> botAccounts;
    if (QueryResult result = LoginDatabase.PQuery("SELECT id FROM account WHERE username LIKE '{}%'", prefix))
    {
        do
        {
            botAccounts.push_back(result->Fetch()[0].GetUInt32());
        } while (result->NextRow());
    }

    uint32 deletedChars = 0;
    for (uint32 accountId : botAccounts)
    {
        if (QueryResult result = CharacterDatabase.PQuery("SELECT guid FROM characters WHERE account = {}", accountId))
        {
            do
            {
                ObjectGuid const guid = ObjectGuid::Create<HighGuid::Player>(result->Fetch()[0].GetUInt64());
                Player::DeleteFromDB(guid, accountId, true, true);
                ++deletedChars;
            } while (result->NextRow());
        }
    }

    for (uint32 accountId : botAccounts)
        AccountMgr::DeleteAccount(accountId);

    TC_LOG_INFO("server.loading", "RandomPlayerbotFactory: deleted {} characters across {} bot accounts. "
        "Set Playerbots.DeleteRandomBotAccounts back to 0 and restart to regenerate.", deletedChars, botAccounts.size());

    World::StopNow(SHUTDOWN_EXIT_CODE);
}
}  // namespace

void RandomPlayerbotFactory::GenerateRandomBots()
{
    if (!Playerbots::IsEnabled())
        return;

    if (Playerbots::GetDeleteRandomBotAccounts())
    {
        RunTeardown();
        return;
    }

    if (!Playerbots::IsRandomBotFeatureEnabled())
        return;

    if (Playerbots::GetRandomBotAccountPrefix().empty())
    {
        TC_LOG_ERROR("server.loading", "RandomPlayerbotFactory: random-bot generation requires Playerbots.RandomBotAccountPrefix — skipping generation.");
        return;
    }

    uint32 const target = Playerbots::GetMaxRandomBots();
    uint32 charsPerAccount = sWorld->getIntConfig(CONFIG_CHARACTERS_PER_ACCOUNT);
    if (charsPerAccount == 0)
        charsPerAccount = 10;  // AC hardcoded fallback if the realm config is unreadable

    // ---- Phase A: ensure enough reserved bot accounts exist (idempotent) ----
    // All bot accounts run at the realm expansion so allied/special races are unlocked (server-side
    // Player::Create bypasses the client expansion gate, but this keeps the stored account state
    // honest and is the shared plumbing Session 2 reuses).
    uint8 const botExpansion = uint8(sWorld->getIntConfig(CONFIG_EXPANSION));
    uint32 const neededAccounts = CalculateAccountCount(target, charsPerAccount);
    std::vector<uint32> accountIds;
    accountIds.reserve(neededAccounts);
    uint32 createdAccounts = 0;
    for (uint32 i = 0; i < neededAccounts; ++i)
    {
        std::string const name = MakeAccountName(i);
        uint32 id = AccountMgr::GetId(name);
        if (!id)
        {
            id = CreateBotAccount(name);
            if (id)
                ++createdAccounts;
        }
        if (id)
        {
            EnsureBotAccountExpansion(id, botExpansion);
            accountIds.push_back(id);
        }
    }

    if (accountIds.empty())
    {
        TC_LOG_ERROR("server.loading", "RandomPlayerbotFactory: no bot accounts available — generation aborted.");
        return;
    }

    // ---- Phase B: ensure enough bot characters exist (idempotent shortfall only) ----
    uint32 existing = 0;
    for (uint32 accountId : accountIds)
        existing += AccountMgr::GetCharactersCount(accountId);

    if (existing >= target)
    {
        TC_LOG_INFO("server.loading", "RandomPlayerbotFactory: {} bot account(s), {} existing bot character(s) (>= target {}) — nothing to generate.",
            accountIds.size(), existing, target);
        return;
    }

    std::vector<std::pair<uint8, uint8>> allianceCombos;
    std::vector<std::pair<uint8, uint8>> hordeCombos;
    BuildValidCombos(allianceCombos, hordeCombos);
    if (allianceCombos.empty() && hordeCombos.empty())
    {
        TC_LOG_ERROR("server.loading", "RandomPlayerbotFactory: no generatable race/class combinations found — generation aborted.");
        return;
    }

    uint32 toCreate = target - existing;
    uint32 createdChars = 0;
    int32 const realmId = int32(sRealmList->GetCurrentRealmId().Realm);

    for (uint32 accountId : accountIds)
    {
        if (toCreate == 0)
            break;

        uint32 const have = AccountMgr::GetCharactersCount(accountId);
        if (have >= charsPerAccount)
            continue;

        uint32 room = charsPerAccount - have;

        std::string accountName;
        AccountMgr::GetName(accountId, accountName);
        AccountTypes const security = AccountTypes(AccountMgr::GetSecurity(accountId, realmId));

        WorldSession* session = WorldSession::CreateForBot(accountId, accountName, security, botExpansion);

        while (room > 0 && toCreate > 0)
        {
            bool const preferAlliance = (createdChars % 2) == 0;  // light 50/50 faction balance
            if (CreateBotCharacter(session, accountId, preferAlliance, allianceCombos, hordeCombos))
            {
                ++createdChars;
                --toCreate;
                --room;
            }
            else
            {
                break;  // stop filling this account on failure to avoid a tight retry loop
            }
        }

        delete session;
    }

    TC_LOG_INFO("server.loading", "RandomPlayerbotFactory: generation complete — {} new account(s), {} new character(s); pool now {} character(s) across {} bot account(s).",
        createdAccounts, createdChars, existing + createdChars, accountIds.size());
}

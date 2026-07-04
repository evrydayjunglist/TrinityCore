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

#ifndef TRINITY_PLAYERBOTS_CONFIG_H
#define TRINITY_PLAYERBOTS_CONFIG_H

#include "AccountMgr.h"
#include "Config.h"
#include "StringConvert.h"
#include "Util.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace Playerbots
{
inline bool IsEnabled()
{
    return sConfigMgr->GetBoolDefault("Playerbots.Enable", false);
}

inline uint32 GetReservedAccountMinId()
{
    return sConfigMgr->GetIntDefault("Playerbots.ReservedAccount.MinId", 0);
}

inline uint32 GetReservedAccountMaxId()
{
    return sConfigMgr->GetIntDefault("Playerbots.ReservedAccount.MaxId", 0);
}

inline uint32 GetLogLevel()
{
    return sConfigMgr->GetIntDefault("Playerbots.LogLevel", 0);
}

inline uint32 GetReactDelay()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.ReactDelay", 100), 1u);
}

inline uint32 GetGlobalCooldown()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.GlobalCooldown", 500), 1u);
}

inline float GetFollowDistance()
{
    return std::max<float>(sConfigMgr->GetFloatDefault("Playerbots.FollowDistance", 3.0f), 0.5f);
}

inline uint32 GetMaxActiveBots()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.MaxActiveBots", 1), 1u);
}

inline bool AllowAccountBots()
{
    return sConfigMgr->GetBoolDefault("Playerbots.AllowAccountBots", true);
}

inline uint32 GetMaxAddedBots()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.MaxAddedBots", 1), 1u);
}

inline std::vector<uint32> GetReservedAccountIds()
{
    std::vector<uint32> ids;
    std::string const idsConfig = sConfigMgr->GetStringDefault("Playerbots.ReservedAccount.Ids", "");
    if (idsConfig.empty())
        return ids;

    for (auto token : Trinity::Tokenize(idsConfig, ',', false))
    {
        if (Optional<uint32> id = Trinity::StringTo<uint32>(token))
            ids.push_back(*id);
    }
    return ids;
}

// AC-style random-bot account name prefix (RandomPlayerbotFactory). Accounts named
// "<prefix><N>" are the generated bot accounts. This is also the third reserved-account
// reservation mode (alongside ReservedAccount.Ids and MinId/MaxId) — see
// playerbots-random-bot-generation-handoff.md § 6.2. Empty disables prefix mode.
inline std::string GetRandomBotAccountPrefix()
{
    return sConfigMgr->GetStringDefault("Playerbots.RandomBotAccountPrefix", "rndbot");
}

// Case-insensitive prefix test against a game-account username (usernames are stored
// upper-cased, so callers may pass either case).
inline bool IsReservedAccountName(std::string_view username)
{
    std::string const prefix = GetRandomBotAccountPrefix();
    if (prefix.empty() || username.size() < prefix.size())
        return false;

    return std::equal(prefix.begin(), prefix.end(), username.begin(),
        [](char a, char b)
        {
            return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
        });
}

inline bool IsReservedAccountPolicyConfigured()
{
    if (!GetReservedAccountIds().empty())
        return true;

    uint32 const minId = GetReservedAccountMinId();
    uint32 const maxId = GetReservedAccountMaxId();
    if (minId != 0 && maxId != 0 && minId <= maxId)
        return true;

    return !GetRandomBotAccountPrefix().empty();
}

inline bool IsReservedAccount(uint32 accountId)
{
    if (accountId == 0)
        return false;

    // Any configured mode reserves the account (checked as an OR, not first-match-wins) — a
    // deployment may combine explicit ids / a range with the AC prefix mode.
    std::vector<uint32> const explicitIds = GetReservedAccountIds();
    if (!explicitIds.empty() && std::ranges::find(explicitIds, accountId) != explicitIds.end())
        return true;

    uint32 const minId = GetReservedAccountMinId();
    uint32 const maxId = GetReservedAccountMaxId();
    if (minId != 0 && maxId != 0 && minId <= maxId && accountId >= minId && accountId <= maxId)
        return true;

    // Prefix mode: resolve the account username and match the configured prefix. Not a hot path
    // (bot-login guards + startup roster build), so the AccountMgr::GetName lookup is acceptable.
    if (!GetRandomBotAccountPrefix().empty())
    {
        std::string name;
        if (AccountMgr::GetName(accountId, name) && IsReservedAccountName(name))
            return true;
    }

    return false;
}

inline std::string GetReservedAccountPolicySummary()
{
    std::vector<std::string> parts;

    std::vector<uint32> const explicitIds = GetReservedAccountIds();
    if (!explicitIds.empty())
    {
        std::string result = "explicit Ids:";
        for (uint32 id : explicitIds)
            result += " " + std::to_string(id);
        parts.push_back(std::move(result));
    }

    uint32 const minId = GetReservedAccountMinId();
    uint32 const maxId = GetReservedAccountMaxId();
    if (minId != 0 && maxId != 0 && minId <= maxId)
        parts.push_back("range MinId-MaxId: " + std::to_string(minId) + "-" + std::to_string(maxId));

    std::string const prefix = GetRandomBotAccountPrefix();
    if (!prefix.empty())
        parts.push_back("name prefix: " + prefix);

    if (parts.empty())
        return "not configured";

    std::string summary;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i)
            summary += "; ";
        summary += parts[i];
    }
    return summary;
}

inline bool GetEnableDatabases()
{
    return sConfigMgr->GetBoolDefault("Playerbots.Updates.EnableDatabases", false);
}

inline std::string GetPlayerbotsDatabaseInfo()
{
    return sConfigMgr->GetStringDefault("PlayerbotsDatabaseInfo", "");
}

inline uint32 GetMinRandomBots()
{
    return sConfigMgr->GetIntDefault("Playerbots.MinRandomBots", 0);
}

inline uint32 GetMaxRandomBots()
{
    return sConfigMgr->GetIntDefault("Playerbots.MaxRandomBots", 0);
}

inline bool GetRandomBotAutologin()
{
    return sConfigMgr->GetBoolDefault("Playerbots.RandomBotAutologin", false);
}

inline bool GetDisabledWithoutRealPlayer()
{
    return sConfigMgr->GetBoolDefault("Playerbots.DisabledWithoutRealPlayer", true);
}

inline bool IsRandomBotFeatureEnabled()
{
    return IsEnabled() && GetMaxRandomBots() > 0;
}

inline std::vector<uint32> ParseCommaUintList(std::string const& value)
{
    std::vector<uint32> ids;
    if (value.empty())
        return ids;

    for (auto token : Trinity::Tokenize(value, ',', false))
    {
        if (Optional<uint32> id = Trinity::StringTo<uint32>(token))
            ids.push_back(*id);
    }
    return ids;
}

inline std::vector<std::string> ParseCommaStringList(std::string const& value)
{
    std::vector<std::string> items;
    if (value.empty())
        return items;

    for (auto token : Trinity::Tokenize(value, ',', false))
    {
        std::string trimmed = std::string(token);
        if (!trimmed.empty())
            items.push_back(std::move(trimmed));
    }
    return items;
}

// DEPRECATED (playerbots-random-bot-generation-handoff.md § 5 Phase C): the fixed-roster
// hand-list is superseded by AC-style DB enumeration of characters on the reserved bot
// accounts (RandomPlayerbotMgr::BuildRoster). Retained only so a legacy deployed conf that
// still sets these keys doesn't warn; no longer read by the roster builder. Do not extend.
inline std::vector<uint32> GetRandomBotAccounts()
{
    return ParseCommaUintList(sConfigMgr->GetStringDefault("Playerbots.RandomBotAccounts", ""));
}

// DEPRECATED — see GetRandomBotAccounts above.
inline std::vector<std::string> GetRandomBotCharacterNames()
{
    return ParseCommaStringList(sConfigMgr->GetStringDefault("Playerbots.RandomBotCharacterNames", ""));
}

// RandomPlayerbotFactory (generation). Number of reserved bot accounts to provision. 0 = auto:
// ceil(MaxRandomBots / CONFIG_CHARACTERS_PER_ACCOUNT), AC's CalculateTotalAccountCount shape. A
// manual value below the calculated minimum is warned about and overridden.
inline uint32 GetRandomBotAccountCount()
{
    return sConfigMgr->GetIntDefault("Playerbots.RandomBotAccountCount", 0);
}

// When true, generated bot accounts get a random 10-char password instead of the account name
// (AC's randomBotRandomPassword). Bots log in socketlessly, so this is purely defense-in-depth.
inline bool GetRandomBotRandomPassword()
{
    return sConfigMgr->GetBoolDefault("Playerbots.RandomBotRandomPassword", false);
}

// AC's deleteRandomBotAccounts analog — the loud, opt-in "regenerate from scratch" switch. When
// true, startup deletes every generated bot-account character (and the bot accounts), logs
// loudly, and stops the server so the operator can reset this flag. Off by default; never a
// silent action. See playerbots-random-bot-generation-handoff.md § 5 Phase C.
inline bool GetDeleteRandomBotAccounts()
{
    return sConfigMgr->GetBoolDefault("Playerbots.DeleteRandomBotAccounts", false);
}

// Session 1 — special races (playerbots-special-races-classes-s1-allied-races-handoff.md).
// When true (default), generated random bots may roll allied races (Void Elf, Dark Iron, Vulpera,
// Earthen, ...) — detected data-first via ChrRacesFlag::IsAlliedRace + expansion, never a hardcoded
// id list. Set to 0 to restrict generation to base races only. AC (WotLK) has no allied races, so
// this is a TC-native but AC-minded (config-driven) toggle.
inline bool GetEnableAlliedRaces()
{
    return sConfigMgr->GetBoolDefault("Playerbots.EnableAlliedRaces", true);
}

// Optional curated include-list of race ids a generated bot may roll (comma-separated). Empty
// (default) = every race the realm's expansion allows (base + allied per EnableAlliedRaces).
// Non-empty restricts generation to exactly these race ids (still intersected with the expansion /
// disabled-race masks and EnableAlliedRaces). Data-first knob; not a substitute for flag-based
// allied detection.
inline std::vector<uint32> GetAllowedRaces()
{
    return ParseCommaUintList(sConfigMgr->GetStringDefault("Playerbots.AllowedRaces", ""));
}

// Session 2 seam (playerbots-special-races-classes-s2-hero-classes-handoff.md): hero classes
// (Death Knight / Demon Hunter / Evoker) start in instanced scenarios the wander strategy cannot
// path out of, with no world relocation yet, so generation keeps them excluded until Session 2
// implements the create-time open-world override. Default off. This is the clean config-gated
// extension point Session 2 flips on — not a hardcoded "hero excluded". Enabling it before
// Session 2 lands will create hero bots stuck in their starting scenario.
inline bool GetEnableHeroClasses()
{
    return sConfigMgr->GetBoolDefault("Playerbots.EnableHeroClasses", false);
}

// Review follow-up C1 (playerbots-review-c-followups-handoff.md): periodic DB flush for active
// random bots, matching AC's periodic-save intent without porting its full online/offline DB
// event machinery. 0 disables (no clamp to 1u like the other timers — 0 is a legitimate "off").
inline uint32 GetRandomBotSaveIntervalSeconds()
{
    return sConfigMgr->GetIntDefault("Playerbots.RandomBotSaveIntervalSeconds", 300);
}

// Gate 10 — world RPG loop / random bot leveling (ported from AC's AiPlayerbot.* level block,
// see playerbots-gate-10-world-rpg-slice-handoff.md § TC-Midnight adaptations)
inline uint32 GetRandomBotMinLevel()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.RandomBotMinLevel", 1), 1u);
}

inline uint32 GetRandomBotMaxLevel()
{
    return sConfigMgr->GetIntDefault("Playerbots.RandomBotMaxLevel", 0);
}

inline bool GetDisableRandomLevels()
{
    return sConfigMgr->GetBoolDefault("Playerbots.DisableRandomLevels", false);
}

inline uint32 GetRandombotStartingLevel()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.RandombotStartingLevel", 1), 1u);
}

// AC semantics: a toggle, not a level — when true the bot's level is locked (no XP gain at all).
inline bool GetRandomBotFixedLevel()
{
    return sConfigMgr->GetBoolDefault("Playerbots.RandomBotFixedLevel", false);
}

inline float GetRandomBotXPRate()
{
    return std::max<float>(sConfigMgr->GetFloatDefault("Playerbots.RandomBotXPRate", 1.0f), 0.0f);
}

// Chance (0-100) that an idle random bot runs the "newrpg" wander/grind/quest-giver loop
// instead of staying passive. Fork-native knob (AC's own randomBotRpgChance selects between
// RPG-vs-grind sub-states inside an already-RPG bot; here the RPG subset itself is the only
// active behavior, so this is the single on/off-with-a-dial knob for it).
inline float GetRandomBotRpgChance()
{
    return std::clamp<float>(sConfigMgr->GetFloatDefault("Playerbots.RandomBotRpgChance", 100.0f), 0.0f, 100.0f);
}

// Gate 10b — RPG state-machine knobs. Weights mirror AC's AiPlayerbot.RpgStatusProbWeight.*
// defaults (WanderRandom 15 / GoGrind 15 / DoQuest 60 — mod-playerbots-master
// PlayerbotAIConfig.cpp:678-685); a status whose weight is 0 is never chosen. Durations mirror
// AC's NewRpgAction.h constants (statusWanderRandomDuration 5 min, statusDoQuestDuration 30 min,
// poiStayTime 5 min) but are config keys here per the Gate 10b handoff ("tunable knobs"),
// expressed in seconds.
inline uint32 GetRpgStatusProbWeightWanderRandom()
{
    return sConfigMgr->GetIntDefault("Playerbots.RpgStatusProbWeight.WanderRandom", 15);
}

inline uint32 GetRpgStatusProbWeightGoGrind()
{
    return sConfigMgr->GetIntDefault("Playerbots.RpgStatusProbWeight.GoGrind", 15);
}

inline uint32 GetRpgStatusProbWeightDoQuest()
{
    return sConfigMgr->GetIntDefault("Playerbots.RpgStatusProbWeight.DoQuest", 60);
}

inline uint32 GetRpgStatusWanderRandomDurationMs()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.RpgStatusDuration.WanderRandom", 300), 1u) * 1000;
}

inline uint32 GetRpgStatusDoQuestDurationMs()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.RpgStatusDuration.DoQuest", 1800), 1u) * 1000;
}

// AC: NewRpgAction.h poiStayTime (5 min) — how long a bot holds at a quest POI with zero
// objective progress before the quest goes on its abandon set.
inline uint32 GetRpgPoiStayTimeMs()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.RpgPoiStayTime", 300), 1u) * 1000;
}

// Fork-native (not ported from AC — see playerbots-bot-wander-ground-clip-handoff.md §5): the
// mmap navmesh's own "normal ground" cutoff is 55 degrees (src/common/mmaps_common/Generator/
// TileBuilder.cpp), which PathGenerator already confines Player-type sources to (steep 55-70
// degree ground is combat-creature-only). That still leaves room for a bot to be routed along a
// technically-connected 45-55 degree incline that reads as "climbing a wall" to a human watching —
// this is a stricter, additional bot-side sanity check on top of the engine's own navmesh
// classification, not a replacement for it. Tune down (never above the engine's own 55 degree
// ceiling — a higher value would meaninglessly reduce this check's own effect) if bots refuse
// legitimate terrain too often.
inline float GetMaxWalkableSlopeDegrees()
{
    // Default 35 degrees: derived from a local player slope-walk sniff (2026-07-03,
    // playerbots-bot-wander-ground-clip-handoff.md §7) — a human's steepest *sustained* head-on
    // climb on the same terrain was ~35-37 degrees (steeper isolated readings were edge-skirting,
    // not a real ascent), while an unchecked bot walked a 41 degree pitch. 35 sits just under the
    // measured human sustained limit.
    return std::clamp<float>(sConfigMgr->GetFloatDefault("Playerbots.MaxWalkableSlopeDegrees", 35.0f), 1.0f, 55.0f);
}

// Quest loot + object interaction (playerbots-quest-loot-and-object-interaction-handoff.md).
// AC reference: mod-playerbots-master lootDistance / the OpenLootAction anti-ninja + IsLootAllowed
// quest filter — reimplemented TC-native and packetless (Player::SendLoot/StoreLootItem,
// GameObject::Use directly, no synthetic CMSG). All bounded so a bot never drifts into Gate 18
// gear/vendor economy: V1 loots quest-relevant items only by default.

// How far a bot will look for a lootable corpse / usable quest object (yd). Matches the core's
// own AELootCreatureCheck::LootDistance (30yd) so the bot's reach mirrors a real looting player.
inline float GetLootDistance()
{
    return std::clamp<float>(sConfigMgr->GetFloatDefault("Playerbots.LootDistance", 30.0f), 5.0f, 100.0f);
}

// V1 guardrail: when true (default) a bot only stores loot that starts a quest or satisfies an
// item objective in its own log (AC's StoreLootAction::IsLootAllowed quest core). Turn it off to
// let bots grab everything they're allowed to — kept off the default path so looting never turns
// into the gear/vendor economy that's Gate 18's scope.
inline bool GetLootQuestItemsOnly()
{
    return sConfigMgr->GetBoolDefault("Playerbots.LootQuestItemsOnly", true);
}

// Whether bots pick up coin from corpses/objects they loot (retail bots do; harmless to bags).
inline bool GetLootMoney()
{
    return sConfigMgr->GetBoolDefault("Playerbots.LootMoney", true);
}

// Reach for using a quest-objective gameobject (yd). The core's interaction check
// (GetGameObjectIfCanInteractWith) enforces the real ~INTERACTION_DISTANCE ceiling regardless;
// this only decides how close the bot walks before it tries, so keep it at/below that.
inline float GetQuestObjectUseDistance()
{
    return std::clamp<float>(sConfigMgr->GetFloatDefault("Playerbots.QuestObjectUseDistance", 5.0f), 1.0f, 10.0f);
}
}

#endif

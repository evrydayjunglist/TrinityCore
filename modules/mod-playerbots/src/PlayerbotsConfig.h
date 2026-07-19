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

// Gate 12 — class-agnostic combat brain knobs (targeting / range / flee).
inline bool GetCombatFleeEnabled()
{
    return sConfigMgr->GetBoolDefault("Playerbots.Combat.FleeEnabled", true);
}

inline uint8 GetCombatFleeHealthPct()
{
    return static_cast<uint8>(std::clamp<uint32>(
        sConfigMgr->GetIntDefault("Playerbots.Combat.FleeHealthPct", 35), 1u, 100u));
}

inline uint8 GetCombatFleeHealthExitPct()
{
    uint8 const enterPct = GetCombatFleeHealthPct();
    uint8 const exitPct = static_cast<uint8>(std::clamp<uint32>(
        sConfigMgr->GetIntDefault("Playerbots.Combat.FleeHealthExitPct", 50), 1u, 100u));
    // Hysteresis: exit must be at or above enter, otherwise oscillation.
    return std::max(exitPct, enterPct);
}

inline float GetCombatFleeDistance()
{
    return std::clamp<float>(sConfigMgr->GetFloatDefault("Playerbots.Combat.FleeDistance", 20.0f), 5.0f, 60.0f);
}

inline float GetCombatRangedDistance()
{
    return std::clamp<float>(sConfigMgr->GetFloatDefault("Playerbots.Combat.RangedDistance", 28.0f), 8.0f, 40.0f);
}

// -1 = ChrSpecialization Caster/Ranged heuristic, 0 = force melee, 1 = force ranged.
inline int32 GetCombatPreferRanged()
{
    return sConfigMgr->GetIntDefault("Playerbots.Combat.PreferRanged", -1);
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

// Session 2 — hero classes (playerbots-special-races-classes-s2-hero-classes-handoff.md). When true
// (default), generated random bots may roll Death Knight / Demon Hunter / Evoker — detected data-first
// by the CLASS_* enum, never a hardcoded id list. This also un-gates the Dracthyr race (Evoker's
// paired race, an elevated-StartingLevel scenario-start race). Set to 0 to generate base + allied
// classes only. AC-minded: AC (WotLK) has DK but no DH/Evoker, so this is the TC-native master toggle
// alongside the AC-shaped DisableDeathKnightLogin below.
//
// Hero-class bots currently spawn in their instanced intro scenarios (Acherus / Mardum / Forbidden
// Reach) — the wander strategy cannot leave those yet. That is an accepted state for now (owner
// decision 2026-07-04); teaching bots to complete/exit the starting scenarios is future work (NYI).
// Their start level/money come from Player::Create, never a literal.
inline bool GetEnableHeroClasses()
{
    return sConfigMgr->GetBoolDefault("Playerbots.EnableHeroClasses", true);
}

// AC's AiPlayerbot.DisableDeathKnightLogin analog. When true, generated bots never roll Death Knight
// (even with EnableHeroClasses on). AC combines this with an expansion check (noDK = config || not
// WotLK); on Midnight all three hero classes are available at the realm expansion, so the toggle is
// the gate. Default off (DK enabled) to match AC's default. DH/Evoker are governed by EnableHeroClasses.
inline bool GetDisableDeathKnightLogin()
{
    return sConfigMgr->GetBoolDefault("Playerbots.DisableDeathKnightLogin", false);
}

// Optional curated include-list of class ids a generated bot may roll (comma-separated). Empty
// (default) = every class allowed by the expansion / disabled-class mask / hero-class toggles.
// Non-empty restricts generation to exactly these class ids (still intersected with those gates).
// Mirrors GetAllowedRaces; data-first knob, not a substitute for the enum-based hero detection.
inline std::vector<uint32> GetAllowedClasses()
{
    return ParseCommaUintList(sConfigMgr->GetStringDefault("Playerbots.AllowedClasses", ""));
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

// GO_CAMP = travel to a distant NPC hub (HubLocationCache) then mingle there via WANDER_NPC. AC
// default weight 15 (mod-playerbots PlayerbotAIConfig). This is what lets a bot stranded in a
// giver-less pocket reach a town and pick up new quests. Weight 0 disables hub travel.
inline uint32 GetRpgStatusProbWeightGoCamp()
{
    return sConfigMgr->GetIntDefault("Playerbots.RpgStatusProbWeight.GoCamp", 15);
}

inline uint32 GetRpgStatusProbWeightDoQuest()
{
    return sConfigMgr->GetIntDefault("Playerbots.RpgStatusProbWeight.DoQuest", 60);
}

// Active quest-giver seeking (playerbots-rpg-active-questgiver-seeking-handoff.md §3). AC's
// RPG_WANDER_NPC has no direct AiPlayerbot weight — it lives inside AC's precomputed target
// context; here it's a first-class idle candidate, so give it a solid default weight so a bot
// stranded on junk/finished quests reliably rolls "go find the next quest giver" rather than
// idling. Weight-0 disables seeking (bots fall back to wander/grind, pre-handoff behaviour).
inline uint32 GetRpgStatusProbWeightWanderNpc()
{
    return sConfigMgr->GetIntDefault("Playerbots.RpgStatusProbWeight.WanderNpc", 30);
}

// REST = AC's timed sit (idle filler). AC has no explicit weight for it (it's the fallback), so a
// modest default keeps bots from sitting too often while still adding lifelike pauses. It is also
// the empty-availability fallback regardless of this weight. Weight 0 removes it from the roll.
inline uint32 GetRpgStatusProbWeightRest()
{
    return sConfigMgr->GetIntDefault("Playerbots.RpgStatusProbWeight.Rest", 10);
}

// How far (yd) a bot scans for hub NPCs / quest givers when it enters WANDER_NPC (mingling). Wider
// than QuestGiverAction's 80yd opportunistic-pickup radius — the point is to reach NPCs that aren't
// already adjacent, both to seek a giver and to find a >= 3-NPC hub. Bounded so the per-roll grid
// scan stays cheap at fleet scale (a per-zone cached target value is the scale-up if profiling
// flags it, not a bigger live scan).
inline float GetRpgWanderNpcRadius()
{
    return std::clamp<float>(sConfigMgr->GetFloatDefault("Playerbots.RpgWanderNpcRadius", 200.0f), 80.0f, 500.0f);
}

// How long (ms) a bot dwells at each hub NPC before cycling to the next (AC's npcStayTime, 8s).
inline uint32 GetRpgWanderNpcStayTimeMs()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.RpgWanderNpcStayTime", 8), 1u) * 1000;
}

// RPG combat/objective completion V1 (playerbots-rpg-combat-objective-completion-handoff.md § 6):
// how close (yd) a *neutral* quest kill target (e.g. mottled boars for "Cutting Teeth") must be for
// AttackAnythingAction to engage it. A touch wider than the 30yd hostile ATTACK_SEARCH_RADIUS so a
// bot planted on its objective POI is a little more eager to close on its quest mobs. Bounded so the
// per-tick grid scan stays cheap at fleet scale (only runs while the bot holds an incomplete
// QUEST_OBJECTIVE_MONSTER objective and is out of combat).
inline float GetRpgQuestKillSearchRadius()
{
    return std::clamp<float>(sConfigMgr->GetFloatDefault("Playerbots.RpgQuestKillSearchRadius", 40.0f), 10.0f, 80.0f);
}

inline uint32 GetRpgStatusWanderRandomDurationMs()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.RpgStatusDuration.WanderRandom", 300), 1u) * 1000;
}

inline uint32 GetRpgStatusDoQuestDurationMs()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.RpgStatusDuration.DoQuest", 1800), 1u) * 1000;
}

// AC constants (config keys here): statusWanderNpcDuration 5 min (how long a bot mingles a hub
// before returning to idle), statusRestDuration 30 s (how long a bot sits).
inline uint32 GetRpgStatusWanderNpcDurationMs()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.RpgStatusDuration.WanderNpc", 300), 1u) * 1000;
}

inline uint32 GetRpgStatusRestDurationMs()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.RpgStatusDuration.Rest", 30), 1u) * 1000;
}

// AC: NewRpgAction.h poiStayTime (5 min) — how long a bot holds at a quest POI with zero
// objective progress before the quest goes on its abandon set.
inline uint32 GetRpgPoiStayTimeMs()
{
    return std::max<uint32>(sConfigMgr->GetIntDefault("Playerbots.RpgPoiStayTime", 300), 1u) * 1000;
}

// RPG quest-loop convergence F2 (playerbots-rpg-quest-convergence-fixes-handoff.md § 4-F2): while
// parked at a quest objective POI with zero progress, how often the bot walks to a fresh
// random-weighted interior point of the same objective blob instead of circling one fixed point
// for the whole stay window. This is what lets a bot *discover* stealthed objective mobs the
// retail-like way — patrol the area until a frontal detect-range encounter happens naturally —
// with stealth detection itself untouched. The zero-progress abandon budget (RpgPoiStayTime) keeps
// running across sweep legs.
inline uint32 GetRpgPoiSweepIntervalMs()
{
    return std::clamp<uint32>(sConfigMgr->GetIntDefault("Playerbots.RpgPoiSweepIntervalSeconds", 25), 10u, 120u) * 1000;
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
// quest filter — open via Player::SendLoot / GameObject::Use; store via loot Handle*Opcodes
// (StoreLootAction). All bounded so a bot never drifts into Gate 18 gear/vendor economy:
// V1 loots quest-relevant items only by default.

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

// Interaction range for completing a QUEST_OBJECTIVE_TALKTO objective (TalkToQuestNpcAction). Same
// default/clamp as the quest-object use distance — both mirror the core's ~5yd interaction range.
inline float GetQuestTalkToDistance()
{
    return std::clamp<float>(sConfigMgr->GetFloatDefault("Playerbots.QuestTalkToDistance", 5.0f), 1.0f, 10.0f);
}

// Bot death handling V1 (the corpse run). Pause after death before a bot releases its spirit, so
// the release reads lifelike rather than instant (0 = release immediately). The core reclaim-delay
// window is intentionally left to Player::GetCorpseReclaimDelay — no knob undercuts it.
inline uint32 GetRpgDeathReleaseDelaySeconds()
{
    return std::clamp<uint32>(sConfigMgr->GetIntDefault("Playerbots.RpgDeathReleaseDelaySeconds", 3), 0, 60);
}

// How long a released ghost tries to *walk* back to its corpse before the one-shot
// teleport-to-corpse stranding fallback fires (corpse across genuinely unwalkable terrain/water).
// Walking is the primary path; this only prevents a permanently-stranded ghost.
inline uint32 GetRpgDeathCorpseRunTimeoutSeconds()
{
    return std::clamp<uint32>(sConfigMgr->GetIntDefault("Playerbots.RpgDeathCorpseRunTimeoutSeconds", 600), 60, 1800);
}

// Bot outbound-packet observation (playerbots-bot-packet-observation-handoff.md). Master toggle
// for the SMSG signal layer: the module's ServerScript observer bails immediately when this is
// off, so the one-line core seam's OnPacketSend dispatch simply finds no interested handler and
// bot behavior falls back to the per-tick polls. Default on with the module (the layer is inert
// without registered signal features anyway).
inline bool GetPacketObservationEnabled()
{
    return sConfigMgr->GetBoolDefault("Playerbots.PacketObservation.Enable", true);
}

// Gated AC-like payload parse (playerbots-bot-packet-payload-parse-handoff.md). When off, the
// observation layer stays signal-only (opcode wake-up). When on, registered opcodes also enqueue
// a packet copy for tick-time Write()-mirrored parse + Layer-2 cross-check. Default OFF until
// owner soak PASS, then conf.dist flips to 1.
inline bool GetPacketPayloadParseEnabled()
{
    return sConfigMgr->GetBoolDefault("Playerbots.PacketObservation.PayloadParse.Enable", true);
}
}

#endif

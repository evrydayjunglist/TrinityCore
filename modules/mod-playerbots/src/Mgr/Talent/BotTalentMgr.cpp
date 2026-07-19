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

#include "BotTalentMgr.h"
#include "Config.h"
#include "DB2Stores.h"
#include "DBCEnums.h"
#include "EnumFlag.h"
#include "Log.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "SharedDefines.h"
#include "TraitMgr.h"
#include "TraitPacketsCommon.h"
#include "WorldSession.h"

#include <algorithm>
#include <string>

namespace
{
UF::TraitConfig const* FindActiveCombatTraitConfig(Player* bot, int32 specId)
{
    return bot->m_activePlayerData->TraitConfigs.FindIf([specId](UF::TraitConfig const& traitConfig)
    {
        return static_cast<TraitConfigType>(*traitConfig.Type) == TraitConfigType::Combat
            && traitConfig.ChrSpecializationID == specId
            && EnumFlag(static_cast<TraitCombatConfigFlags>(*traitConfig.CombatConfigFlags))
                   .HasFlag(TraitCombatConfigFlags::ActiveForSpec);
    }).second;
}

int32 FindFreeLocalIdentifier(Player* bot, int32 specId)
{
    int32 index = 1;
    while (bot->m_activePlayerData->TraitConfigs.FindIf([specId, index](UF::TraitConfig const& traitConfig)
    {
        return static_cast<TraitConfigType>(*traitConfig.Type) == TraitConfigType::Combat
            && traitConfig.ChrSpecializationID == specId
            && traitConfig.LocalIdentifier == index;
    }).first)
        ++index;

    return index;
}

bool IsUsableCombatSpecialization(ChrSpecializationEntry const* spec, uint8 botClass)
{
    if (!spec || spec->ClassID != botClass)
        return false;

    // OrderIndex 4 is the char-create "initial" slot — core skips ActiveForSpec TraitConfig for it.
    if (spec->OrderIndex == INITIAL_SPECIALIZATION_INDEX)
        return false;

    return true;
}

ChrSpecializationEntry const* ResolvePreferredSpecialization(Player* bot)
{
    uint8 const botClass = bot->GetClass();
    std::string const key = "Playerbots.Talent.PreferSpec." + std::to_string(botClass);
    int32 const preferId = sConfigMgr->GetIntDefault(key, 0);
    if (preferId <= 0)
        return nullptr;

    ChrSpecializationEntry const* preferred = sChrSpecializationStore.LookupEntry(uint32(preferId));
    if (!IsUsableCombatSpecialization(preferred, botClass))
    {
        TC_LOG_WARN("playerbots", "BotTalentMgr: PreferSpec class={} id={} invalid for bot={} — ignoring",
            uint32(botClass), preferId, bot->GetName());
        return nullptr;
    }

    return preferred;
}

ChrSpecializationEntry const* PickDefaultCombatSpecialization(uint8 botClass)
{
    ChrSpecializationEntry const* recommended = nullptr;
    ChrSpecializationEntry const* first = nullptr;

    // Mirror Player::_LoadTraits: create configs for OrderIndex 0 .. MAX_SPECIALIZATIONS-2 only.
    for (uint32 i = 0; i < MAX_SPECIALIZATIONS - 1; ++i)
    {
        ChrSpecializationEntry const* spec = sDB2Manager.GetChrSpecializationByIndex(botClass, i);
        if (!IsUsableCombatSpecialization(spec, botClass))
            continue;

        if (!first)
            first = spec;

        if (spec->GetFlags().HasFlag(ChrSpecializationFlag::Recommended))
        {
            recommended = spec;
            break;
        }
    }

    return recommended ? recommended : first;
}

bool EnsurePrimarySpecialization(Player* bot)
{
    ChrSpecializationEntry const* chosen = ResolvePreferredSpecialization(bot);

    if (!chosen)
    {
        ChrSpecializationEntry const* current = bot->GetPrimarySpecializationEntry();
        if (IsUsableCombatSpecialization(current, bot->GetClass()))
            return true;

        chosen = PickDefaultCombatSpecialization(bot->GetClass());
    }

    if (!chosen)
    {
        TC_LOG_ERROR("playerbots", "BotTalentMgr: no ChrSpecialization DB2 row for class={} bot={} — NYI",
            uint32(bot->GetClass()), bot->GetName());
        return false;
    }

    if (bot->GetPrimarySpecializationEntry() == chosen)
        return true;

    if (Playerbots::GetLogLevel() >= 1)
    {
        TC_LOG_DEBUG("playerbots", "BotTalentMgr: ActivateTalentGroup bot={} class={} spec={} ({})",
            bot->GetName(), uint32(bot->GetClass()), chosen->ID, chosen->Name[LOCALE_enUS]);
    }

    bot->ActivateTalentGroup(chosen);
    return bot->GetPrimarySpecializationEntry() == chosen;
}

UF::TraitConfig const* EnsureActiveCombatTraitConfig(Player* bot)
{
    int32 const specId = int32(bot->GetPrimarySpecialization());
    if (UF::TraitConfig const* existing = FindActiveCombatTraitConfig(bot, specId))
    {
        if (int32(*bot->m_activePlayerData->ActiveCombatTraitConfigID) != existing->ID)
            bot->SetActiveCombatTraitConfigID(existing->ID);
        return existing;
    }

    ChrSpecializationEntry const* spec = bot->GetPrimarySpecializationEntry();
    if (!spec)
        return nullptr;

    WorldPackets::Traits::TraitConfig traitConfig;
    traitConfig.Type = TraitConfigType::Combat;
    traitConfig.ChrSpecializationID = spec->ID;
    traitConfig.CombatConfigFlags = TraitCombatConfigFlags::ActiveForSpec;
    traitConfig.LocalIdentifier = FindFreeLocalIdentifier(bot, spec->ID);
    if (WorldSession* session = bot->GetSession())
        traitConfig.Name = spec->Name[session->GetSessionDbcLocale()];
    else
        traitConfig.Name = spec->Name[LOCALE_enUS];

    bot->CreateTraitConfig(traitConfig);

    UF::TraitConfig const* created = FindActiveCombatTraitConfig(bot, spec->ID);
    if (!created)
    {
        TC_LOG_ERROR("playerbots", "BotTalentMgr: CreateTraitConfig failed bot={} spec={} — NYI",
            bot->GetName(), spec->ID);
        return nullptr;
    }

    bot->SetActiveCombatTraitConfigID(created->ID);
    return created;
}

bool HasSpentStarterRanks(WorldPackets::Traits::TraitConfig const& config)
{
    return std::ranges::any_of(config.Entries, [](WorldPackets::Traits::TraitEntry const& entry)
    {
        return entry.Rank > 0;
    });
}

bool ApplyStarterCombatTraits(Player* bot, bool forceRefresh)
{
    UF::TraitConfig const* active = EnsureActiveCombatTraitConfig(bot);
    if (!active)
        return false;

    if (!forceRefresh
        && EnumFlag(static_cast<TraitCombatConfigFlags>(*active->CombatConfigFlags))
               .HasFlag(TraitCombatConfigFlags::StarterBuild))
        return true; // idempotent — already on starter loadout

    WorldPackets::Traits::TraitConfig newConfigState(*active);
    TraitMgr::InitializeStarterBuildTraitConfig(newConfigState, bot);

    if (!HasSpentStarterRanks(newConfigState))
    {
        TC_LOG_WARN("playerbots",
            "BotTalentMgr: InitializeStarterBuildTraitConfig left no spent ranks bot={} spec={} — NYI (no TraitTreeLoadout)",
            bot->GetName(), newConfigState.ChrSpecializationID);
        return false;
    }

    if (TraitMgr::ValidateConfig(newConfigState, bot) != TraitMgr::LearnResult::Ok)
    {
        TC_LOG_WARN("playerbots",
            "BotTalentMgr: ValidateConfig failed after starter fill bot={} spec={} — NYI",
            bot->GetName(), newConfigState.ChrSpecializationID);
        return false;
    }

    newConfigState.CombatConfigFlags |= TraitCombatConfigFlags::StarterBuild;
    // First apply: mirror TraitHandler (new LocalIdentifier). Refresh: keep ID to avoid action-bar thrash.
    if (!EnumFlag(static_cast<TraitCombatConfigFlags>(*active->CombatConfigFlags))
            .HasFlag(TraitCombatConfigFlags::StarterBuild))
        newConfigState.LocalIdentifier = FindFreeLocalIdentifier(bot, newConfigState.ChrSpecializationID);

    // Human TraitHandler uses withCastTime=true (commit cast → EffectChangeActiveCombatTraitConfig).
    // Socketless bots skip the cast and apply immediately — same finalize path, no QueuePacket.
    bot->UpdateTraitConfig(std::move(newConfigState), 0, false);

    if (Playerbots::GetLogLevel() >= 1)
    {
        UF::TraitConfig const* applied = bot->GetTraitConfig(bot->m_activePlayerData->ActiveCombatTraitConfigID);
        TC_LOG_DEBUG("playerbots", "BotTalentMgr: starter traits applied bot={} spec={} configId={} entries={}",
            bot->GetName(),
            int32(bot->GetPrimarySpecialization()),
            applied ? int32(applied->ID) : 0,
            applied ? int32(applied->Entries.size()) : 0);
    }

    return true;
}
} // namespace

void BotTalentMgr::EnsureSpecAndStarterTraits(Player* bot, bool forceRefresh)
{
    if (!bot || !Playerbots::IsEnabled())
        return;

    if (!Playerbots::GetTalentAutoApply())
        return;

    if (!EnsurePrimarySpecialization(bot))
        return;

    ApplyStarterCombatTraits(bot, forceRefresh);
}

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

#include "AreaTrigger.h"
#include "AreaTriggerAI.h"
#include "Containers.h"
#include "Creature.h"
#include "EventProcessor.h"
#include "Log.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SceneMgr.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include <unordered_set>

enum DracthyrForbiddenReach
{
    MAP_FORBIDDEN_REACH = 2570,
    AREA_WAR_CRECHE     = 13806
};

enum DracthyrLoginSpells
{
    // Spells
    SPELL_DRACTHYR_LOGIN                = 369728, // teleports to random room, plays scene for the room, binds the home position
    SPELL_STASIS_1                      = 369735, // triggers 366620
    SPELL_STASIS_2                      = 366620, // triggers 366636
    SPELL_STASIS_3                      = 366636, // removes 365560, sends first quest (64864)
    SPELL_STASIS_4                      = 365560, // freeze the target
    SPELL_DRACTHYR_MOVIE_ROOM_01        = 394245, // scene for room 1
    SPELL_DRACTHYR_MOVIE_ROOM_02        = 394279, // scene for room 2
    SPELL_DRACTHYR_MOVIE_ROOM_03        = 394281, // scene for room 3
    SPELL_DRACTHYR_MOVIE_ROOM_04        = 394282, // scene for room 4
    //SPELL_DRACTHYR_MOVIE_ROOM_05        = 394283, // scene for room 5 (only plays sound, unused?)
    SPELL_MAINTAIN_DERVISHIAN           = 369731, // Alliance Personal Summon
    SPELL_SUMMON_DERVISHIAN             = 369730,
    SPELL_MAINTAIN_KODETHI              = 370112, // Horde Personal Summon
    SPELL_SUMMON_KODETHI                = 370111,
    SPELL_AWAKEN_DRACTYHR_QUEST_ABANDON = 369744,
    SPELL_STASIS_FEEDBACK_KNOCKBACK     = 364074,
    SPELL_STASIS_FEEDBACK_VISUAL        = 374633,
    SPELL_DISINTEGRATE_SPELLCLICK       = 362355,
    SPELL_DRACTHYR_STASIS_NPC           = 382137,
    SPELL_DRACTHYR_STASIS_LOWER         = 362331,

    NPC_DERVISHIAN                      = 181494,
    NPC_KODETHI                         = 187223,
    NPC_AZURATHEL_STASIS                = 183380,
    NPC_AZURATHEL_QUEST_ENDER           = 181056,
    NPC_KILL_CREDIT_CAVE_IN             = 187015,

    QUEST_AWAKEN_DRACTYHR               = 64864,

    SCENE_PKG_DRACTHYR_EVOKER_INTRO     = 3730
};

struct DracthyrLoginRoom
{
    uint32 MovieSpellId;
    Position PlayerPosition;
    Position SummonPosition;
};

static constexpr std::array<DracthyrLoginRoom, 4> LoginRoomData =
{{
    {
        SPELL_DRACTHYR_MOVIE_ROOM_01,
        { 5725.32f, -3024.26f, 251.047f, 0.01745329238474369f },
        { 5739.97216796875f, -3023.970458984375f, 251.172332763671875f, 3.193952560424804687f }
    },
    {
        SPELL_DRACTHYR_MOVIE_ROOM_02,
        { 5743.03f, -3067.28f, 251.047f, 0.798488140106201171f },
        { 5754.3046875f, -3056.34716796875f, 251.1725006103515625f, 3.926990747451782226f }
    },
    {
        SPELL_DRACTHYR_MOVIE_ROOM_03,
        { 5787.1597f, -3083.3906f, 251.04698f, 1.570796370506286621f },
        { 5787.44970703125f, -3069.335205078125f, 251.168121337890625f, 4.729842185974121093f }
    },
    {
        SPELL_DRACTHYR_MOVIE_ROOM_04,
        { 5829.32f, -3064.49f, 251.047f, 2.364955902099609375f },
        { 5818.533203125f, -3054.5625f, 251.3630828857421875f, 5.480333805084228515f }
    }
}};

static std::unordered_set<ObjectGuid> s_dracthyrIntroPending;
static std::unordered_set<ObjectGuid> s_dracthyrIntroQuestOffered;
static std::unordered_map<ObjectGuid, uint32> s_dracthyrLoginRoom;

static Optional<uint32> GetDracthyrLoginRoomIndex(Player const* player)
{
    if (!player)
        return {};

    auto itr = s_dracthyrLoginRoom.find(player->GetGUID());
    if (itr == s_dracthyrLoginRoom.end())
        return {};

    return itr->second;
}

static uint32 GetNearestDracthyrLoginRoomIndex(Player const* player)
{
    auto currentRoom = std::ranges::min_element(LoginRoomData, [player](DracthyrLoginRoom const& left, DracthyrLoginRoom const& right)
    {
        return player->GetDistance(left.PlayerPosition) < player->GetDistance(right.PlayerPosition);
    });

    if (currentRoom == LoginRoomData.end())
        return 0;

    return std::distance(LoginRoomData.begin(), currentRoom);
}

static uint32 GetDracthyrGuideEntry(Player const* player)
{
    return player->GetRace() == RACE_DRACTHYR_ALLIANCE ? NPC_DERVISHIAN : NPC_KODETHI;
}

static uint32 GetDracthyrMaintainSpell(Player const* player)
{
    return player->GetRace() == RACE_DRACTHYR_ALLIANCE ? SPELL_MAINTAIN_DERVISHIAN : SPELL_MAINTAIN_KODETHI;
}

static uint32 GetDracthyrSummonSpell(Player const* player)
{
    return player->GetRace() == RACE_DRACTHYR_ALLIANCE ? SPELL_SUMMON_DERVISHIAN : SPELL_SUMMON_KODETHI;
}

static Creature* FindDracthyrGuide(Player const* player)
{
    if (!player)
        return nullptr;

    uint32 const entry = GetDracthyrGuideEntry(player);

    std::list<Creature*> creatures;
    player->GetCreatureListWithOptionsInGrid(creatures, 100.0f, { .CreatureId = entry, .IgnorePhases = true, .PrivateObjectOwnerGuid = player->GetGUID() });
    if (!creatures.empty())
        return creatures.front();

    if (Creature* guide = player->FindNearestCreatureWithOptions(100.0f, { .CreatureId = entry, .IgnorePhases = true }))
        return guide;

    return nullptr;
}

static bool IsDracthyrEvoker(Player const* player)
{
    if (!player || player->GetClass() != CLASS_EVOKER)
        return false;

    switch (player->GetRace())
    {
        case RACE_DRACTHYR_HORDE:
        case RACE_DRACTHYR_ALLIANCE:
            return true;
        default:
            return false;
    }
}

static Optional<uint32> GetExpectedDracthyrLoginRoomIndex(Player const* player)
{
    if (Optional<uint32> stored = GetDracthyrLoginRoomIndex(player))
        return stored;

    return GetNearestDracthyrLoginRoomIndex(player);
}

static bool IsDracthyrGuideAtExpectedRoom(Player const* player)
{
    Creature* guide = FindDracthyrGuide(player);
    if (!guide)
        return false;

    Optional<uint32> roomIndex = GetExpectedDracthyrLoginRoomIndex(player);
    if (!roomIndex || *roomIndex >= LoginRoomData.size())
        return false;

    return guide->GetDistance(LoginRoomData[*roomIndex].SummonPosition) <= 20.0f;
}

static void DespawnDracthyrGuide(Player* player)
{
    if (!player)
        return;

    std::list<Creature*> guides;
    player->GetCreatureListWithOptionsInGrid(guides, 100.0f,
        { .CreatureId = GetDracthyrGuideEntry(player), .IgnorePhases = true, .PrivateObjectOwnerGuid = player->GetGUID() });
    for (Creature* guide : guides)
        guide->DespawnOrUnsummon(0s);

    player->RemoveAura(SPELL_MAINTAIN_KODETHI);
    player->RemoveAura(SPELL_MAINTAIN_DERVISHIAN);
}

static void SummonDracthyrGuideDirect(Player* player)
{
    if (!player || IsDracthyrGuideAtExpectedRoom(player))
        return;

    uint32 roomIndex = GetExpectedDracthyrLoginRoomIndex(player).value_or(0);
    if (roomIndex >= LoginRoomData.size())
        return;

    uint32 const entry = GetDracthyrGuideEntry(player);
    Position const pos = LoginRoomData[roomIndex].SummonPosition;

    SummonPropertiesEntry const* props = nullptr;
    if (SpellInfo const* summonInfo = sSpellMgr->GetSpellInfo(GetDracthyrSummonSpell(player), DIFFICULTY_NONE))
    {
        for (SpellEffectInfo const& effect : summonInfo->GetEffects())
        {
            if (effect.IsEffect(SPELL_EFFECT_SUMMON) || effect.IsEffect(SPELL_EFFECT_SUMMON_PET))
            {
                props = sSummonPropertiesStore.LookupEntry(effect.MiscValueB);
                break;
            }
        }
    }

    player->GetMap()->SummonCreature(entry, pos, props, 0ms, player, GetDracthyrSummonSpell(player), 0, player->GetGUID());
}

static void EnsureDracthyrGuide(Player* player)
{
    if (!IsDracthyrEvoker(player))
        return;

    if (IsDracthyrGuideAtExpectedRoom(player))
        return;

    DespawnDracthyrGuide(player);

    uint32 const maintainSpell = GetDracthyrMaintainSpell(player);
    uint32 const summonSpell = GetDracthyrSummonSpell(player);

    player->CastSpell(player, maintainSpell, true);
    player->CastSpell(player, summonSpell, true);

    if (!IsDracthyrGuideAtExpectedRoom(player))
        SummonDracthyrGuideDirect(player);
}

static void ClearDracthyrIntroPresentationAuras(Player* player)
{
    if (!player)
        return;

    player->RemoveAurasDueToSpell(SPELL_STASIS_4);
    player->RemoveAurasDueToSpell(SPELL_DRACTHYR_MOVIE_ROOM_01);
    player->RemoveAurasDueToSpell(SPELL_DRACTHYR_MOVIE_ROOM_02);
    player->RemoveAurasDueToSpell(SPELL_DRACTHYR_MOVIE_ROOM_03);
    player->RemoveAurasDueToSpell(SPELL_DRACTHYR_MOVIE_ROOM_04);
    player->GetSceneMgr().CancelSceneByPackageId(SCENE_PKG_DRACTHYR_EVOKER_INTRO);
}

static Creature* DespawnExtraDracthyrGuidesAndGetKeeper(Player* player)
{
    if (!player)
        return nullptr;

    uint32 const entry = GetDracthyrGuideEntry(player);
    std::list<Creature*> guides;
    player->GetCreatureListWithOptionsInGrid(guides, 100.0f,
        { .CreatureId = entry, .IgnorePhases = true, .PrivateObjectOwnerGuid = player->GetGUID() });

    Position keeperPos = player->GetPosition();
    if (Optional<uint32> roomIndex = GetExpectedDracthyrLoginRoomIndex(player))
        if (*roomIndex < LoginRoomData.size())
            keeperPos = LoginRoomData[*roomIndex].SummonPosition;

    Creature* keeper = nullptr;
    for (Creature* guide : guides)
    {
        if (!keeper || guide->GetDistance(keeperPos) < keeper->GetDistance(keeperPos))
            keeper = guide;
    }

    for (Creature* guide : guides)
    {
        if (guide != keeper)
            guide->DespawnOrUnsummon(0s);
    }

    return keeper;
}

static bool TryOfferDracthyrAwakenQuest(Player* player)
{
    if (!player)
        return false;

    if (s_dracthyrIntroQuestOffered.contains(player->GetGUID()))
        return true;

    Quest const* quest = sObjectMgr->GetQuestTemplate(QUEST_AWAKEN_DRACTYHR);
    if (!quest || player->GetQuestStatus(QUEST_AWAKEN_DRACTYHR) != QUEST_STATUS_NONE)
        return true;

    if (!player->CanTakeQuest(quest, false))
        return false;

    ClearDracthyrIntroPresentationAuras(player);
    player->RemoveAurasWithInterruptFlags(SpellAuraInterruptFlags::Interacting);

    EnsureDracthyrGuide(player);
    Creature* guide = DespawnExtraDracthyrGuidesAndGetKeeper(player);
    if (!guide)
    {
        TC_LOG_ERROR("scripts", "Dracthyr intro: cannot offer quest {} — personal guide not spawned", QUEST_AWAKEN_DRACTYHR);
        return false;
    }

    player->PlayerTalkClass->SendCloseGossip();
    player->PlayerTalkClass->SendQuestQueryResponse(quest);

    // Player GUID + DisplayPopup shows the arch-range popup. QuestGiverCreatureID is the guide entry
    // (portrait). Accept closes the UI with CMSG_CLOSE_INTERACTION; grant is handled there.
    uint32 const guideEntry = GetDracthyrGuideEntry(player);
    player->PlayerTalkClass->SendQuestGiverQuestDetails(
        quest, player->GetGUID(), true, true, guideEntry, guide->GetDisplayId());

    s_dracthyrIntroQuestOffered.insert(player->GetGUID());
    return true;
}

struct DracthyrGuideSummonCheckEvent : public BasicEvent
{
    explicit DracthyrGuideSummonCheckEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (!_player || !_player->IsInWorld())
            return true;

        if (_player->GetQuestStatus(QUEST_AWAKEN_DRACTYHR) == QUEST_STATUS_INCOMPLETE)
            EnsureDracthyrGuide(_player);
        else if (!FindDracthyrGuide(_player))
            EnsureDracthyrGuide(_player);

        return true;
    }

private:
    Player* _player;
};

static void RunDracthyrLoginIntro(Player* player, uint32 roomIndex, bool updateAreaAuras);

struct DracthyrLoginIntroEvent : public BasicEvent
{
    DracthyrLoginIntroEvent(Player* player, uint32 roomIndex, bool updateAreaAuras)
        : _player(player), _roomIndex(roomIndex), _updateAreaAuras(updateAreaAuras) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        RunDracthyrLoginIntro(_player, _roomIndex, _updateAreaAuras);
        return true;
    }

private:
    Player* _player;
    uint32 _roomIndex;
    bool _updateAreaAuras;
};

static void RunDracthyrLoginIntro(Player* player, uint32 roomIndex, bool /*updateAreaAuras*/)
{
    if (player)
        s_dracthyrIntroPending.erase(player->GetGUID());

    if (!player || !player->IsInWorld())
        return;

    if (roomIndex >= LoginRoomData.size())
        return;

    DracthyrLoginRoom const& room = LoginRoomData[roomIndex];

    s_dracthyrLoginRoom[player->GetGUID()] = roomIndex;
    s_dracthyrIntroQuestOffered.erase(player->GetGUID());
    DespawnDracthyrGuide(player);

    WorldLocation dest(player->GetMapId(), room.PlayerPosition);
    if (!player->TeleportTo(dest, TELE_TO_SPELL, {}, SPELL_DRACTHYR_LOGIN))
    {
        TC_LOG_WARN("server", "Dracthyr login intro: teleport failed for {}", player->GetName());
        return;
    }

    player->SetHomebind(*player, player->GetAreaId());
    player->CastSpell(player, SPELL_STASIS_4, true);
    player->CastSpell(player, room.MovieSpellId, true);
    EnsureDracthyrGuide(player);
    player->m_Events.AddEventAtOffset(new DracthyrGuideSummonCheckEvent(player), 500ms);
}

static void ScheduleDracthyrLoginIntro(Player* player, bool updateAreaAuras, Milliseconds delay = 1s)
{
    if (!player)
        return;

    ObjectGuid const guid = player->GetGUID();
    if (s_dracthyrIntroPending.contains(guid))
        return;

    s_dracthyrIntroPending.insert(guid);

    uint32 const roomIndex = urand(0, LoginRoomData.size() - 1);
    player->m_Events.AddEventAtOffset(new DracthyrLoginIntroEvent(player, roomIndex, updateAreaAuras), delay);
}

// 369728 - Dracthyr Login
// 369744 - Awaken, Dracthyr OnquestAbandon
class spell_dracthyr_login : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_DRACTHYR_MOVIE_ROOM_01, SPELL_DRACTHYR_MOVIE_ROOM_02, SPELL_DRACTHYR_MOVIE_ROOM_03, SPELL_DRACTHYR_MOVIE_ROOM_04 });
    }

    void HandleCast()
    {
        Player* player = GetCaster()->ToPlayer();
        if (!player)
            return;

        ScheduleDracthyrLoginIntro(player, GetSpellInfo()->Id == SPELL_AWAKEN_DRACTYHR_QUEST_ABANDON);
    }

    void SuppressDefaultTeleport(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(effIndex);
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_dracthyr_login::HandleCast);
        OnEffectHitTarget += SpellEffectFn(spell_dracthyr_login::SuppressDefaultTeleport, EFFECT_0, SPELL_EFFECT_TELEPORT_UNITS);
    }
};

// First-login intro: playercreateinfo_cast_spell for 369728 is unreliable during the login
// handshake, so drive the room teleport + movie from OnLogin instead (369744 abandon still uses spell_dracthyr_login).
class player_dracthyr_intro_login : public PlayerScript
{
public:
    player_dracthyr_intro_login() : PlayerScript("player_dracthyr_intro_login") { }

    void OnLogin(Player* player, bool firstLogin) override
    {
        if (!IsDracthyrEvoker(player))
            return;

        if (firstLogin)
        {
            ObjectGuid const guid = player->GetGUID();
            if (!s_dracthyrIntroPending.contains(guid))
            {
                s_dracthyrIntroPending.insert(guid);
                uint32 const roomIndex = urand(0, LoginRoomData.size() - 1);
                RunDracthyrLoginIntro(player, roomIndex, false);
            }
            return;
        }

        if (player->GetMapId() == MAP_FORBIDDEN_REACH &&
            player->GetQuestStatus(QUEST_AWAKEN_DRACTYHR) == QUEST_STATUS_INCOMPLETE)
            EnsureDracthyrGuide(player);
    }

    void OnUpdateZone(Player* player, uint32 /*newZone*/, uint32 newArea) override
    {
        if (!IsDracthyrEvoker(player) || player->GetMapId() != MAP_FORBIDDEN_REACH)
            return;

        // Hub spawn fires zone update before OnLogin intro; spell_area would summon at nearest room (wrong).
        if (!GetDracthyrLoginRoomIndex(player) &&
            player->GetQuestStatus(QUEST_AWAKEN_DRACTYHR) == QUEST_STATUS_NONE)
            return;

        if (newArea != AREA_WAR_CRECHE &&
            player->GetQuestStatus(QUEST_AWAKEN_DRACTYHR) != QUEST_STATUS_INCOMPLETE)
            return;

        EnsureDracthyrGuide(player);
    }

    void OnLogout(Player* player) override
    {
        s_dracthyrLoginRoom.erase(player->GetGUID());
        s_dracthyrIntroPending.erase(player->GetGUID());
        s_dracthyrIntroQuestOffered.erase(player->GetGUID());
    }
};

// 3730 - Dracthyr Evoker Intro (Post Movie)
class scene_dracthyr_evoker_intro : public SceneScript
{
public:
    scene_dracthyr_evoker_intro() : SceneScript("scene_dracthyr_evoker_intro") { }

    void OnSceneComplete(Player* player, uint32 /*sceneInstanceID*/, SceneTemplate const* /*sceneTemplate*/) override
    {
        EnsureDracthyrGuide(player);
        player->CastSpell(player, SPELL_STASIS_1, true);
    }

    void OnSceneCancel(Player* player, uint32 /*sceneInstanceID*/, SceneTemplate const* /*sceneTemplate*/) override
    {
        EnsureDracthyrGuide(player);
        player->CastSpell(player, SPELL_STASIS_1, true);
    }
};

// 369730 - Summon Dervishian
// 370111 - Summon Kodethi
class spell_dracthyr_summon_dervishian : public SpellScript
{
    void SetDest(SpellDestination& dest) const
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return;

        uint32 roomIndex = GetExpectedDracthyrLoginRoomIndex(player).value_or(0);
        if (roomIndex >= LoginRoomData.size())
            return;

        Position const& targetPos = LoginRoomData[roomIndex].SummonPosition;
        dest.Relocate(targetPos);
    }

    void Register() override
    {
        OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_dracthyr_summon_dervishian::SetDest, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
    }
};

// 366636 - Stasis 3 (Awaken, Dracthyr quest offer — player GUID like EffectQuestStart)
class spell_dracthyr_stasis_3 : public SpellScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return spellInfo->HasEffect(SPELL_EFFECT_QUEST_START);
    }

    void OfferAwakenQuest(SpellEffIndex effIndex)
    {
        if (GetEffectInfo().Effect != SPELL_EFFECT_QUEST_START)
            return;

        PreventHitDefaultEffect(effIndex);

        Player* player = GetHitPlayer();
        if (!player)
            player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return;

        TryOfferDracthyrAwakenQuest(player);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_dracthyr_stasis_3::OfferAwakenQuest, EFFECT_ALL, SPELL_EFFECT_ANY);
    }
};

// 362355 - Disintegrate (spellclick on stasis dracthyr)
class spell_disintegrate_dracthyr_awaken : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_DRACTHYR_STASIS_NPC, SPELL_DRACTHYR_STASIS_LOWER });
    }

    void HandleHitTarget()
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        Unit* target = GetHitUnit();
        if (!player || !target)
            return;

        target->RemoveAurasDueToSpell(SPELL_DRACTHYR_STASIS_NPC);
        target->RemoveAurasDueToSpell(SPELL_DRACTHYR_STASIS_LOWER);

        uint32 const entry = target->GetEntry();
        player->KilledMonsterCredit(entry, target->GetGUID());

        if (CreatureTemplate const* cInfo = sObjectMgr->GetCreatureTemplate(entry))
        {
            for (uint32 killCredit : cInfo->KillCredit)
            {
                if (killCredit)
                    player->KilledMonsterCredit(killCredit, ObjectGuid::Empty);
            }
        }

        if (entry == NPC_AZURATHEL_STASIS)
        {
            player->KilledMonsterCredit(NPC_KILL_CREDIT_CAVE_IN, ObjectGuid::Empty);

            if (Creature* creature = target->ToCreature())
            {
                creature->UpdateEntry(NPC_AZURATHEL_QUEST_ENDER);
                creature->RemoveNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);
                creature->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP | UNIT_NPC_FLAG_QUESTGIVER);
            }
        }
        else if (entry == NPC_KODETHI || entry == NPC_DERVISHIAN)
        {
            if (Creature* creature = target->ToCreature())
                creature->RemoveNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);
        }
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_disintegrate_dracthyr_awaken::HandleHitTarget);
    }
};

// 64864 - Awaken, Dracthyr
class quest_awaken_dracthyr : public QuestScript
{
public:
    quest_awaken_dracthyr() : QuestScript("quest_awaken_dracthyr") { }

    void OnQuestStatusChange(Player* player, Quest const* quest, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (quest->GetQuestId() == QUEST_AWAKEN_DRACTYHR && newStatus == QUEST_STATUS_INCOMPLETE)
            EnsureDracthyrGuide(player);

        if (newStatus == QUEST_STATUS_NONE)
        {
            player->CastSpell(player, SPELL_AWAKEN_DRACTYHR_QUEST_ABANDON, false);
            // remove summon aura to relocate questgiver to new random room
            player->RemoveAura(SPELL_MAINTAIN_DERVISHIAN);
            player->RemoveAura(SPELL_MAINTAIN_KODETHI);
        }
    }
};

// 30308 - Stasis Feedback
struct at_dracthyr_stasis_feedback : AreaTriggerAI
{
    at_dracthyr_stasis_feedback(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger) { }

    void OnUnitEnter(Unit* unit) override
    {
        if (!unit->IsPlayer())
            return;

        unit->CastSpell(nullptr, SPELL_STASIS_FEEDBACK_KNOCKBACK, false);
        if (Unit* caster = at->GetCaster())
            caster->CastSpell(caster->GetPosition(), SPELL_STASIS_FEEDBACK_VISUAL, true);
    }
};

void AddSC_zone_the_forbidden_reach()
{
    new player_dracthyr_intro_login();
    RegisterSpellScript(spell_dracthyr_login);
    new scene_dracthyr_evoker_intro();
    RegisterSpellScript(spell_dracthyr_summon_dervishian);
    RegisterSpellScript(spell_dracthyr_stasis_3);
    RegisterSpellScript(spell_disintegrate_dracthyr_awaken);
    new quest_awaken_dracthyr();
    RegisterAreaTriggerAI(at_dracthyr_stasis_feedback);
}

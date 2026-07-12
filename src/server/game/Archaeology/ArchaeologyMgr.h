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

#ifndef TRINITY_ARCHAEOLOGYMGR_H
#define TRINITY_ARCHAEOLOGYMGR_H

#include "Define.h"
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

struct ResearchSiteEntry;

// Per dig site: which research branch its fragments belong to, and how many finds exhaust it.
// The site->branch theming is not carried by client DB2, so it is fork reference data loaded from
// the world table `archaeology_dig_site` (seeded from the Archy addon join). See
// docs/midnight-assessment/archaeology/archaeology-phase1-foundation-handoff.md.
struct ArchaeologyDigSiteInfo
{
    uint32 BranchID = 0;
    uint8 FindCount = 0;
    std::vector<std::pair<float, float>> Polygon; // dig-site boundary, world X/Y vertices in order
};

// Server-side owner of Archaeology (secondary profession) research data loaded from the client
// DB2 stores. Indexes the dig-site pools per continent so active sites can be assigned to players,
// and the site->branch reference data. Dig-site polygons (QuestPOIBlob) follow with the survey slice.
class TC_GAME_API ArchaeologyMgr
{
    private:
        ArchaeologyMgr();
        ~ArchaeologyMgr();

    public:
        ArchaeologyMgr(ArchaeologyMgr const&) = delete;
        ArchaeologyMgr(ArchaeologyMgr&&) = delete;
        ArchaeologyMgr& operator=(ArchaeologyMgr const&) = delete;
        ArchaeologyMgr& operator=(ArchaeologyMgr&&) = delete;

        static ArchaeologyMgr* instance();

        // Index dig sites (ResearchSite.db2) by map. Call once at startup after DB2 stores load.
        void LoadResearchSites();

        // Load the site->branch reference data from `archaeology_dig_site`. Call after LoadResearchSites
        // (needs sResearchSiteStore) and once the world DB is available.
        void LoadDigSiteData();

        // Load dig-site boundary polygons from `archaeology_dig_site_point`. Call after LoadDigSiteData.
        void LoadDigSitePoints();

        // True if the world position (x, y) is inside the dig site's boundary polygon.
        bool IsInsideDigSite(uint32 researchSiteId, float x, float y) const;

        // Deterministic world location of the Nth find at a dig site (a point inside its polygon).
        // Returns false if the site has no usable polygon.
        bool GetFindLocation(uint32 researchSiteId, uint32 findIndex, float& x, float& y) const;

        // Dig-site pool for a continent/map, or nullptr if the map has none.
        std::vector<ResearchSiteEntry const*> const* GetResearchSitesForMap(uint32 mapId) const;

        // Randomly pick up to `count` distinct dig-site IDs from a map's pool (fewer if the pool is
        // smaller). Empty if the map has no dig sites.
        std::vector<uint32> RollResearchSitesForMap(uint32 mapId, uint32 count) const;

        // Pick one random branch-mapped dig site on a map that is not in `exclude` (used to replace an
        // exhausted site). Returns 0 if none are available.
        uint32 RollReplacementSite(uint32 mapId, std::vector<uint32> const& exclude) const;

        // Pick a random research project for a branch (rarity-weighted toward commons), preferring
        // projects not in `completed`. Returns the ResearchProject.db2 ID, or 0 if the branch has none.
        uint32 RollResearchProject(uint32 branchId, std::unordered_set<uint32> const& completed) const;

        // Branch/find-count for a dig site, or nullptr if the site has no mapping.
        ArchaeologyDigSiteInfo const* GetDigSiteInfo(uint32 researchSiteId) const;

    private:
        std::unordered_map<uint32 /*mapId*/, std::vector<ResearchSiteEntry const*>> _researchSitesByMap;
        std::unordered_map<uint32 /*researchSiteId*/, ArchaeologyDigSiteInfo> _digSiteInfo;
};

#define sArchaeologyMgr ArchaeologyMgr::instance()

#endif

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
#include <vector>

struct ResearchSiteEntry;

// Server-side owner of Archaeology (secondary profession) research data loaded from the client
// DB2 stores. Currently indexes the dig-site pools per continent so active sites can be assigned to
// players; branch/project indexing and QuestPOIBlob dig-site polygons follow in later sub-slices.
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

        // Dig-site pool for a continent/map, or nullptr if the map has none.
        std::vector<ResearchSiteEntry const*> const* GetResearchSitesForMap(uint32 mapId) const;

        // Randomly pick up to `count` distinct dig-site IDs from a map's pool (fewer if the pool is
        // smaller). Empty if the map has no dig sites.
        std::vector<uint32> RollResearchSitesForMap(uint32 mapId, uint32 count) const;

    private:
        std::unordered_map<uint32 /*mapId*/, std::vector<ResearchSiteEntry const*>> _researchSitesByMap;
};

#define sArchaeologyMgr ArchaeologyMgr::instance()

#endif

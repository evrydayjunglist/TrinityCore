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

#ifndef UpdateFetcher_h__
#define UpdateFetcher_h__

#include "Common.h"
#include "DatabaseEnvFwd.h"
#include <boost/filesystem/path.hpp>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

struct TC_DATABASE_API UpdateResult
{
    UpdateResult()
        : updated(0), recent(0), archived(0) { }

    UpdateResult(size_t const updated_, size_t const recent_, size_t const archived_)
        : updated(updated_), recent(recent_), archived(archived_) { }

    size_t updated;
    size_t recent;
    size_t archived;
};

class TC_DATABASE_API UpdateFetcher
{
public:
    using Path = boost::filesystem::path;

    enum State
    {
        RELEASED,
        ARCHIVED,
        MODULE
    };

    struct DirectoryEntry
    {
        DirectoryEntry(Path const& path_, State state_) : path(path_), state(state_) { }

        Path path;
        State state;
    };

    struct AppliedFileEntry
    {
        AppliedFileEntry(std::string name_, std::string hash_, State state_, uint64 timestamp_)
            : name(std::move(name_)), hash(std::move(hash_)), state(state_), timestamp(timestamp_) { }

        std::string name;
        std::string hash;
        State state;
        uint64 timestamp;

        static inline State StateConvert(std::string_view const& state)
        {
            if (state == "RELEASED"sv)
                return RELEASED;
            if (state == "MODULE"sv)
                return MODULE;
            return ARCHIVED;
        }

        static inline std::string_view StateConvert(State const state)
        {
            switch (state)
            {
                case RELEASED:
                    return "RELEASED"sv;
                case MODULE:
                    return "MODULE"sv;
                case ARCHIVED:
                default:
                    return "ARCHIVED"sv;
            }
        }

        std::string_view GetStateAsString() const
        {
            return StateConvert(state);
        }
    };

    using DirectoryStorage = std::vector<DirectoryEntry>;
    using AppliedFileList = std::vector<AppliedFileEntry>;
    using DirectoryProvider = std::function<DirectoryStorage()>;
    using AppliedFileProvider = std::function<AppliedFileList()>;

    UpdateFetcher(Path const& updateDirectory,
        std::vector<Path> moduleDirectories,
        std::function<void(std::string const&)> const& apply,
        std::function<void(Path const& path)> const& applyFile,
        std::function<QueryResult(std::string const&)> const& retrieve);
    UpdateFetcher(Path const& updateDirectory,
        std::vector<Path> moduleDirectories,
        std::function<void(std::string const&)> const& apply,
        std::function<void(Path const& path)> const& applyFile,
        DirectoryProvider directoryProvider,
        AppliedFileProvider appliedFileProvider);
    ~UpdateFetcher();

    UpdateResult Update(bool const redundancyChecks, bool const allowRehash,
                  bool const archivedRedundancy, int32 const cleanDeadReferencesMaxCount) const;

private:
    enum UpdateMode
    {
        MODE_APPLY,
        MODE_REHASH
    };

    typedef std::pair<Path, State> LocaleFileEntry;

    struct PathCompare
    {
        static std::string MakeComparisonObject(LocaleFileEntry const& arg);
        static std::string MakeComparisonObject(std::string const& arg);

        template<typename L, typename R>
        bool operator()(L const& left, R const& right) const
        {
            return PathCompare::MakeComparisonObject(left) < PathCompare::MakeComparisonObject(right);
        }

        using is_transparent = int;
    };

    typedef std::set<LocaleFileEntry, PathCompare> LocaleFileStorage;
    typedef std::unordered_map<std::string, std::string> HashToFileNameStorage;
    typedef std::map<std::string, AppliedFileEntry, PathCompare> AppliedFileStorage;

    LocaleFileStorage GetFileList() const;
    void FillFileListRecursively(Path const& path, LocaleFileStorage& storage,
        State const state, uint32 const depth) const;

    DirectoryStorage ReceiveIncludedDirectories() const;
    AppliedFileStorage ReceiveAppliedFiles() const;

    std::string ReadSQLUpdate(Path const& file) const;

    uint32 Apply(Path const& path) const;

    void UpdateEntry(AppliedFileEntry const& entry, uint32 const speed = 0) const;
    void RenameEntry(std::string const& from, std::string const& to) const;
    void CleanUp(AppliedFileStorage const& storage) const;

    void UpdateState(std::string const& name, State const state) const;

    std::unique_ptr<Path> const _sourceDirectory;
    std::vector<Path> const _moduleDirectories;

    std::function<void(std::string const&)> const _apply;
    std::function<void(Path const& path)> const _applyFile;
    std::function<QueryResult(std::string const&)> const _retrieve;
    DirectoryProvider const _directoryProvider;
    AppliedFileProvider const _appliedFileProvider;
};

#endif // UpdateFetcher_h__

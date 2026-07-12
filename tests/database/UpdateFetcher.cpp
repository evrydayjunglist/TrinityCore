/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; as a special exception the author gives
 * unlimited permission to copy and/or distribute it, with or without
 * modifications, as long as this notice is preserved.
 */

#include "tc_catch2.h"

#include "CryptoHash.h"
#include "DBUpdater.h"
#include "UpdateFetcher.h"
#include "Util.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <string>
#include <vector>

namespace
{
class TemporaryDirectory
{
public:
    TemporaryDirectory()
        : _path(boost::filesystem::temp_directory_path() /
            boost::filesystem::unique_path("trinity-updater-%%%%-%%%%"))
    {
        boost::filesystem::create_directories(_path);
    }

    ~TemporaryDirectory()
    {
        boost::filesystem::remove_all(_path);
    }

    boost::filesystem::path CreateDirectory(std::string const& name) const
    {
        boost::filesystem::path path = _path / name;
        boost::filesystem::create_directories(path);
        return path;
    }

    boost::filesystem::path Write(boost::filesystem::path const& directory,
        std::string const& name, std::string const& contents) const
    {
        boost::filesystem::path path = directory / name;
        std::ofstream stream(path.string(), std::ios::trunc);
        stream << contents;
        return path;
    }

    boost::filesystem::path const& GetPath() const { return _path; }

private:
    boost::filesystem::path _path;
};

std::string Hash(std::string const& contents)
{
    return ByteArrayToHexStr(Trinity::Crypto::SHA1::GetDigestOf(contents));
}
}

TEST_CASE("Module update directories use exact core database targets", "[UpdateFetcher]")
{
    boost::filesystem::path const source("source");
    std::vector<std::string> const modules = { "mod-example", "mod-playerbots" };

    auto const auth = DBUpdater<LoginDatabaseConnection>::GetModuleUpdateDirectories(source, modules);
    auto const characters = DBUpdater<CharacterDatabaseConnection>::GetModuleUpdateDirectories(source, modules);
    auto const world = DBUpdater<WorldDatabaseConnection>::GetModuleUpdateDirectories(source, modules);
    auto const hotfix = DBUpdater<HotfixDatabaseConnection>::GetModuleUpdateDirectories(source, modules);

    REQUIRE(auth == std::vector<boost::filesystem::path>({
        source / "modules" / "mod-example" / "data" / "sql" / "auth",
        source / "modules" / "mod-playerbots" / "data" / "sql" / "auth" }));
    REQUIRE(characters.front().filename() == "characters");
    REQUIRE(world.front().filename() == "world");
    REQUIRE(hotfix.empty());
    REQUIRE(DBUpdater<WorldDatabaseConnection>::GetModuleUpdateDirectories(source, {}).empty());
}

TEST_CASE("Core updates precede deterministically ordered module updates", "[UpdateFetcher]")
{
    TemporaryDirectory temp;
    boost::filesystem::path core = temp.CreateDirectory("core");
    boost::filesystem::path firstModule = temp.CreateDirectory("module-a");
    boost::filesystem::path secondModule = temp.CreateDirectory("module-b");
    temp.Write(core, "03_core.sql", "DO 3;\n");
    temp.Write(firstModule, "01_module.sql", "DO 1;\n");
    temp.Write(secondModule, "02_module.sql", "DO 2;\n");

    std::vector<std::string> appliedFiles;
    std::vector<std::string> metadataQueries;
    UpdateFetcher fetcher(temp.GetPath(), { firstModule, temp.GetPath() / "missing", secondModule },
        [&](std::string const& query) { metadataQueries.push_back(query); },
        [&](boost::filesystem::path const& path) { appliedFiles.push_back(path.filename().string()); },
        [=] { return UpdateFetcher::DirectoryStorage{ { core, UpdateFetcher::RELEASED } }; },
        [] { return UpdateFetcher::AppliedFileList{}; });

    UpdateResult const result = fetcher.Update(true, true, false, 3);

    REQUIRE(result.updated == 3);
    REQUIRE((appliedFiles == std::vector<std::string>{ "03_core.sql", "01_module.sql", "02_module.sql" }));
    REQUIRE(metadataQueries[0].find("'RELEASED'") != std::string::npos);
    REQUIRE(metadataQueries[1].find("'MODULE'") != std::string::npos);
    REQUIRE(metadataQueries[2].find("'MODULE'") != std::string::npos);
}

TEST_CASE("Duplicate update basenames fail before application", "[UpdateFetcher]")
{
    TemporaryDirectory temp;
    boost::filesystem::path core = temp.CreateDirectory("core");
    boost::filesystem::path module = temp.CreateDirectory("module");
    temp.Write(core, "duplicate.sql", "DO 1;\n");
    temp.Write(module, "duplicate.sql", "DO 2;\n");

    std::vector<boost::filesystem::path> appliedFiles;
    UpdateFetcher fetcher(temp.GetPath(), { module },
        [](std::string const&) { },
        [&](boost::filesystem::path const& path) { appliedFiles.push_back(path); },
        [=] { return UpdateFetcher::DirectoryStorage{ { core, UpdateFetcher::RELEASED } }; },
        [] { return UpdateFetcher::AppliedFileList{}; });

    REQUIRE_THROWS_AS(fetcher.Update(true, true, false, 3), UpdateException);
    REQUIRE(appliedFiles.empty());
}

TEST_CASE("Duplicate update basenames are compared case-insensitively", "[UpdateFetcher]")
{
    TemporaryDirectory temp;
    boost::filesystem::path core = temp.CreateDirectory("core");
    boost::filesystem::path module = temp.CreateDirectory("module");
    temp.Write(core, "Collision.sql", "DO 1;\n");
    temp.Write(module, "collision.sql", "DO 2;\n");

    std::vector<boost::filesystem::path> appliedFiles;
    UpdateFetcher fetcher(temp.GetPath(), { module },
        [](std::string const&) { },
        [&](boost::filesystem::path const& path) { appliedFiles.push_back(path); },
        [=] { return UpdateFetcher::DirectoryStorage{ { core, UpdateFetcher::RELEASED } }; },
        [] { return UpdateFetcher::AppliedFileList{}; });

    REQUIRE_THROWS_AS(fetcher.Update(true, true, false, 3), UpdateException);
    REQUIRE(appliedFiles.empty());
}

TEST_CASE("Module update hashes make repeat runs idempotent", "[UpdateFetcher]")
{
    TemporaryDirectory temp;
    boost::filesystem::path module = temp.CreateDirectory("module");
    std::string const contents = "DO 1;\n";
    boost::filesystem::path const update = temp.Write(module, "mod_example_hash.sql", contents);

    std::vector<boost::filesystem::path> appliedFiles;
    std::vector<std::string> metadataQueries;
    UpdateFetcher firstRun(temp.GetPath(), { module },
        [&](std::string const& query) { metadataQueries.push_back(query); },
        [&](boost::filesystem::path const& path) { appliedFiles.push_back(path); },
        [] { return UpdateFetcher::DirectoryStorage{}; },
        [] { return UpdateFetcher::AppliedFileList{}; });

    REQUIRE(firstRun.Update(true, true, false, 3).updated == 1);
    REQUIRE(appliedFiles == std::vector<boost::filesystem::path>{ update });
    REQUIRE(metadataQueries.back().find(Hash(contents)) != std::string::npos);
    REQUIRE(metadataQueries.back().find("'MODULE'") != std::string::npos);

    appliedFiles.clear();
    metadataQueries.clear();
    UpdateFetcher secondRun(temp.GetPath(), { module },
        [&](std::string const& query) { metadataQueries.push_back(query); },
        [&](boost::filesystem::path const& path) { appliedFiles.push_back(path); },
        [] { return UpdateFetcher::DirectoryStorage{}; },
        [&] {
            return UpdateFetcher::AppliedFileList{
                { "mod_example_hash.sql", Hash(contents), UpdateFetcher::MODULE, 0 } };
        });

    REQUIRE(secondRun.Update(true, true, false, 3).updated == 0);
    REQUIRE(appliedFiles.empty());
    REQUIRE(metadataQueries.empty());

    std::string const changedContents = "DO 2;\n";
    temp.Write(module, "mod_example_hash.sql", changedContents);
    UpdateFetcher changedRun(temp.GetPath(), { module },
        [](std::string const&) { },
        [&](boost::filesystem::path const& path) { appliedFiles.push_back(path); },
        [] { return UpdateFetcher::DirectoryStorage{}; },
        [&] {
            return UpdateFetcher::AppliedFileList{
                { "mod_example_hash.sql", Hash(contents), UpdateFetcher::MODULE, 0 } };
        });

    REQUIRE(changedRun.Update(true, true, false, 3).updated == 1);
    REQUIRE(appliedFiles.size() == 1);
}

TEST_CASE("Disabled module history is retained and re-enable is safe", "[UpdateFetcher]")
{
    TemporaryDirectory temp;
    boost::filesystem::path core = temp.CreateDirectory("core");
    std::vector<std::string> metadataQueries;

    UpdateFetcher disabledRun(temp.GetPath(), {},
        [&](std::string const& query) { metadataQueries.push_back(query); },
        [](boost::filesystem::path const&) { FAIL("No files should be applied"); },
        [=] { return UpdateFetcher::DirectoryStorage{ { core, UpdateFetcher::RELEASED } }; },
        [] {
            return UpdateFetcher::AppliedFileList{
                { "missing_core.sql", "core-hash", UpdateFetcher::RELEASED, 0 },
                { "mod_example_retained.sql", "module-hash", UpdateFetcher::MODULE, 0 } };
        });

    REQUIRE(disabledRun.Update(true, true, false, 3).updated == 0);
    REQUIRE(metadataQueries.size() == 1);
    REQUIRE(metadataQueries.front().find("missing_core.sql") != std::string::npos);
    REQUIRE(metadataQueries.front().find("mod_example_retained.sql") == std::string::npos);

    boost::filesystem::path module = temp.CreateDirectory("module");
    std::string const contents = "DO 1;\n";
    temp.Write(module, "mod_example_retained.sql", contents);
    metadataQueries.clear();
    std::vector<boost::filesystem::path> appliedFiles;
    UpdateFetcher reenabledRun(temp.GetPath(), { module },
        [&](std::string const& query) { metadataQueries.push_back(query); },
        [&](boost::filesystem::path const& path) { appliedFiles.push_back(path); },
        [] { return UpdateFetcher::DirectoryStorage{}; },
        [&] {
            return UpdateFetcher::AppliedFileList{
                { "mod_example_retained.sql", Hash(contents), UpdateFetcher::MODULE, 0 } };
        });

    REQUIRE(reenabledRun.Update(true, true, false, 3).updated == 0);
    REQUIRE(appliedFiles.empty());
    REQUIRE(metadataQueries.empty());
}

TEST_CASE("Retained module hashes cannot rename unrelated migrations", "[UpdateFetcher]")
{
    TemporaryDirectory temp;
    boost::filesystem::path module = temp.CreateDirectory("module");
    std::string const contents = "DO 1;\n";
    temp.Write(module, "mod_second_same_hash.sql", contents);

    std::vector<std::string> metadataQueries;
    std::vector<boost::filesystem::path> appliedFiles;
    UpdateFetcher fetcher(temp.GetPath(), { module },
        [&](std::string const& query) { metadataQueries.push_back(query); },
        [&](boost::filesystem::path const& path) { appliedFiles.push_back(path); },
        [] { return UpdateFetcher::DirectoryStorage{}; },
        [&] {
            return UpdateFetcher::AppliedFileList{
                { "mod_removed_same_hash.sql", Hash(contents), UpdateFetcher::MODULE, 0 } };
        });

    REQUIRE(fetcher.Update(true, true, false, 3).updated == 1);
    REQUIRE(appliedFiles.size() == 1);
    REQUIRE(metadataQueries.size() == 1);
    REQUIRE(metadataQueries.front().find("REPLACE INTO `updates`") != std::string::npos);
    REQUIRE(metadataQueries.front().find("mod_second_same_hash.sql") != std::string::npos);
}

TEST_CASE("Module and core history cannot share a basename", "[UpdateFetcher]")
{
    TemporaryDirectory temp;
    boost::filesystem::path module = temp.CreateDirectory("module");
    temp.Write(module, "shared_name.sql", "DO 1;\n");

    std::vector<std::string> metadataQueries;
    std::vector<boost::filesystem::path> appliedFiles;
    UpdateFetcher fetcher(temp.GetPath(), { module },
        [&](std::string const& query) { metadataQueries.push_back(query); },
        [&](boost::filesystem::path const& path) { appliedFiles.push_back(path); },
        [] { return UpdateFetcher::DirectoryStorage{}; },
        [] {
            return UpdateFetcher::AppliedFileList{
                { "SHARED_NAME.sql", "core-hash", UpdateFetcher::RELEASED, 0 } };
        });

    REQUIRE_THROWS_AS(fetcher.Update(true, true, false, 3), UpdateException);
    REQUIRE(appliedFiles.empty());
    REQUIRE(metadataQueries.empty());
}

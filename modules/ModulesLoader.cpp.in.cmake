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

// This file was created automatically from your module configuration!
// Use CMake to reconfigure this file, never change it on your own!

#cmakedefine TRINITY_IS_DYNAMIC_MODULELOADER

#include "Define.h"

@TRINITY_MODULES_FORWARD_DECL@
#ifdef TRINITY_IS_DYNAMIC_MODULELOADER
#  include "revision_data.h"
#  define TC_MODULE_API TC_API_EXPORT
extern "C" {

/// Exposed in module libraries to return the module revision hash.
TC_MODULE_API char const* GetScriptModuleRevisionHash()
{
    return TRINITY_GIT_COMMIT_HASH;
}

/// Exposed in module libraries to return the name of the module
/// contained in this shared library.
TC_MODULE_API char const* GetScriptModule()
{
    return "@TRINITY_CURRENT_MODULE_PROJECT@";
}

#else
#  include "ModulesScriptLoader.h"
#  include <array>
#  define TC_MODULE_API
#endif

/// Exposed in module libraries to register all module scripts to the ScriptMgr.
TC_MODULE_API void AddModulesScripts()
{
@TRINITY_MODULES_INVOKE@}

#ifndef TRINITY_IS_DYNAMIC_MODULELOADER
std::span<std::string_view const> GetStaticModuleNames()
{
    static constexpr std::array<std::string_view, @TRINITY_MODULE_COUNT@> moduleNames =
    {
@TRINITY_MODULE_NAMES@    };
    return moduleNames;
}
#endif

#ifdef TRINITY_IS_DYNAMIC_MODULELOADER
/// Exposed in dynamic module libraries to get the build directive of the module.
TC_MODULE_API char const* GetBuildDirective()
{
    return TRINITY_BUILD_TYPE;
}

} // extern "C"
#endif

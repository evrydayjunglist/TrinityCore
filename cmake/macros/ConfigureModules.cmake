# This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

function(GetModulesBasePath variable)
  set(${variable} "${CMAKE_SOURCE_DIR}/modules" PARENT_SCOPE)
endfunction()

function(GetPathToModule module variable)
  GetModulesBasePath(MODULES_BASE_PATH)
  set(${variable} "${MODULES_BASE_PATH}/${module}" PARENT_SCOPE)
endfunction()

function(GetModuleSourcePath module variable)
  GetPathToModule(${module} MODULE_PATH)
  set(${variable} "${MODULE_PATH}/src" PARENT_SCOPE)
endfunction()

function(GetProjectNameOfModule module variable)
  string(MAKE_C_IDENTIFIER "${module}" MODULE_PROJECT_SUFFIX)
  string(TOLOWER "modules_${MODULE_PROJECT_SUFFIX}" GENERATED_NAME)
  set(${variable} "${GENERATED_NAME}" PARENT_SCOPE)
endfunction()

function(GetModuleList variable)
  GetModulesBasePath(BASE_PATH)
  set(${variable})

  if(EXISTS "${BASE_PATH}")
    file(GLOB LOCALE_MODULE_LIST RELATIVE
      "${BASE_PATH}"
      "${BASE_PATH}/*")

    foreach(MODULE ${LOCALE_MODULE_LIST})
      GetModuleSourcePath(${MODULE} MODULE_SOURCE_PATH)
      if(IS_DIRECTORY "${MODULE_SOURCE_PATH}")
        list(APPEND ${variable} ${MODULE})
      endif()
    endforeach()
  endif()

  set(${variable} ${${variable}} PARENT_SCOPE)
endfunction()

function(ModuleNameToVariable module variable)
  string(MAKE_C_IDENTIFIER "${module}" MODULE_IDENTIFIER)
  string(TOUPPER "${MODULE_IDENTIFIER}" MODULE_IDENTIFIER)
  set(${variable} "MODULE_${MODULE_IDENTIFIER}" PARENT_SCOPE)
endfunction()

function(IsModulesDynamicLinkingRequired variable)
  if(MODULES MATCHES "dynamic")
    set(IS_DEFAULT_VALUE_DYNAMIC ON)
  endif()

  GetModuleList(MODULE_LIST)
  set(IS_REQUIRED OFF)
  foreach(MODULE ${MODULE_LIST})
    ModuleNameToVariable(${MODULE} MODULE_VARIABLE)
    if((${${MODULE_VARIABLE}} STREQUAL "dynamic") OR
        (${${MODULE_VARIABLE}} STREQUAL "default" AND IS_DEFAULT_VALUE_DYNAMIC))
      set(IS_REQUIRED ON)
      break()
    endif()
  endforeach()
  set(${variable} ${IS_REQUIRED} PARENT_SCOPE)
endfunction()

function(GetModulesInstallOffset variable)
  if(WIN32)
    set(${variable} "${CMAKE_INSTALL_PREFIX}/modules" PARENT_SCOPE)
  else()
    set(${variable} "${CMAKE_INSTALL_PREFIX}/bin/modules" PARENT_SCOPE)
  endif()
endfunction()

# This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

message("")

GetModuleList(MODULE_LIST)
GetModulesInstallOffset(INSTALL_OFFSET)

if(MODULES MATCHES "dynamic")
  set(MODULES_DEFAULT_LINKAGE "dynamic")
elseif(MODULES MATCHES "static")
  set(MODULES_DEFAULT_LINKAGE "static")
else()
  set(MODULES_DEFAULT_LINKAGE "disabled")
endif()

foreach(MODULE ${MODULE_LIST})
  ModuleNameToVariable(${MODULE} MODULE_VARIABLE)
  if(${MODULE_VARIABLE} STREQUAL "default")
    set(${MODULE_VARIABLE} ${MODULES_DEFAULT_LINKAGE})
  endif()

  if(${MODULE_VARIABLE} MATCHES "dynamic")
    GetProjectNameOfModule(${MODULE} MODULE_PROJECT_NAME)
    GetNativeSharedLibraryName(${MODULE_PROJECT_NAME} MODULE_PROJECT_LIBRARY)
    list(APPEND MODULE_GRAPH_KEYS ${MODULE_PROJECT_NAME})
    set(MODULE_GRAPH_VALUE_DISPLAY_${MODULE_PROJECT_NAME} ${MODULE_PROJECT_LIBRARY})
    list(APPEND MODULE_GRAPH_VALUE_CONTAINS_MODULES_${MODULE_PROJECT_NAME} ${MODULE})
  elseif(${MODULE_VARIABLE} MATCHES "static")
    list(APPEND MODULE_GRAPH_KEYS worldserver)
    set(MODULE_GRAPH_VALUE_DISPLAY_worldserver worldserver)
    list(APPEND MODULE_GRAPH_VALUE_CONTAINS_MODULES_worldserver ${MODULE})
  else()
    list(APPEND MODULE_GRAPH_KEYS disabled)
    set(MODULE_GRAPH_VALUE_DISPLAY_disabled disabled)
    list(APPEND MODULE_GRAPH_VALUE_CONTAINS_MODULES_disabled ${MODULE})
  endif()
endforeach()

list(SORT MODULE_GRAPH_KEYS)
list(REMOVE_DUPLICATES MODULE_GRAPH_KEYS)

message("* Module configuration (${MODULES}):
    |")

if(MODULE_GRAPH_KEYS)
  foreach(MODULE_GRAPH_KEY ${MODULE_GRAPH_KEYS})
    if(NOT MODULE_GRAPH_KEY STREQUAL "disabled")
      message("    +- ${MODULE_GRAPH_VALUE_DISPLAY_${MODULE_GRAPH_KEY}}")
    else()
      message("    |  ${MODULE_GRAPH_VALUE_DISPLAY_${MODULE_GRAPH_KEY}}")
    endif()
    foreach(MODULE_GRAPH_PROJECT_ENTRY ${MODULE_GRAPH_VALUE_CONTAINS_MODULES_${MODULE_GRAPH_KEY}})
      message("    |   +- ${MODULE_GRAPH_PROJECT_ENTRY}")
    endforeach()
    message("    |")
  endforeach()
else()
  message("    |  no modules discovered")
  message("    |")
endif()

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_library(modules STATIC)

function(ConfigureModuleLoader MODULELOADER_NAME LOADER_OUT IS_DYNAMIC_MODULELOADER)
  set(TRINITY_IS_DYNAMIC_MODULELOADER ${IS_DYNAMIC_MODULELOADER})
  unset(TRINITY_MODULES_FORWARD_DECL)
  unset(TRINITY_MODULES_INVOKE)
  set(TRINITY_CURRENT_MODULE_PROJECT ${MODULELOADER_NAME})

  foreach(LOCALE_MODULE ${ARGN})
    string(MAKE_C_IDENTIFIER "${LOCALE_MODULE}" _module_loader_id)
    string(CONCAT LOADER_FUNCTION "Add" "${_module_loader_id}" "Scripts()")
    set(TRINITY_MODULES_FORWARD_DECL
      "${TRINITY_MODULES_FORWARD_DECL}void ${LOADER_FUNCTION};\n")
    set(TRINITY_MODULES_INVOKE
      "${TRINITY_MODULES_INVOKE}    ${LOADER_FUNCTION};\n")
  endforeach()

  set(GENERATED_LOADER ${CMAKE_CURRENT_BINARY_DIR}/gen_moduleloader/${MODULELOADER_NAME}/ModulesLoader.cpp)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ModulesLoader.cpp.in.cmake ${GENERATED_LOADER})
  set(${LOADER_OUT} ${GENERATED_LOADER} PARENT_SCOPE)
endfunction()

set(ENABLED_MODULE_CONFIG_FILES)

foreach(MODULE ${MODULE_LIST})
  GetPathToModule(${MODULE} MODULE_PATH)
  GetModuleSourcePath(${MODULE} MODULE_SOURCE_PATH)
  ModuleNameToVariable(${MODULE} MODULE_VARIABLE)

  if((${MODULE_VARIABLE} STREQUAL "disabled") OR
      (${MODULE_VARIABLE} STREQUAL "static"))
    GetProjectNameOfModule(${MODULE} MODULE_PROJECT_NAME)
    GetNativeSharedLibraryName(${MODULE_PROJECT_NAME} MODULE_OUTPUT_NAME)
    list(APPEND DISABLED_MODULE_PROJECTS ${INSTALL_OFFSET}/${MODULE_OUTPUT_NAME})

    if(${MODULE_VARIABLE} STREQUAL "static")
      list(APPEND STATIC_MODULES ${MODULE})
      CollectAndAddSourceFiles(modules ${MODULE_SOURCE_PATH} HEADER_VISIBILITY PRIVATE)

      if(COPY_CONF)
        file(GLOB MODULE_CONFIG_FILES "${MODULE_PATH}/conf/*.conf.dist")
        list(APPEND ENABLED_MODULE_CONFIG_FILES ${MODULE_CONFIG_FILES})
      endif()
    endif()
  elseif(${MODULE_VARIABLE} STREQUAL "dynamic")
    ConfigureModuleLoader(${MODULE} MODULE_PRIVATE_LOADER ON ${MODULE})
    GetProjectNameOfModule(${MODULE} MODULE_PROJECT_NAME)
    list(APPEND DYNAMIC_MODULE_PROJECTS ${MODULE_PROJECT_NAME})

    add_library(${MODULE_PROJECT_NAME} SHARED)
    CollectAndAddSourceFiles(${MODULE_PROJECT_NAME} ${MODULE_SOURCE_PATH} HEADER_VISIBILITY PRIVATE)
    target_sources(${MODULE_PROJECT_NAME}
      PRIVATE
        ${MODULE_PRIVATE_LOADER})

    target_link_libraries(${MODULE_PROJECT_NAME}
      PRIVATE
        trinity-core-interface
      PUBLIC
        game)

    set_target_properties(${MODULE_PROJECT_NAME}
      PROPERTIES
        COMPILE_WARNING_AS_ERROR ${WITH_WARNINGS_AS_ERRORS}
        FOLDER "modules")

    if(COPY_CONF)
      file(GLOB MODULE_CONFIG_FILES "${MODULE_PATH}/conf/*.conf.dist")
      list(APPEND ENABLED_MODULE_CONFIG_FILES ${MODULE_CONFIG_FILES})
    endif()

    if(UNIX)
      install(TARGETS ${MODULE_PROJECT_NAME}
        DESTINATION ${INSTALL_OFFSET}
        COMPONENT ${MODULE_PROJECT_NAME})
    elseif(WIN32)
      install(TARGETS ${MODULE_PROJECT_NAME}
        RUNTIME DESTINATION ${INSTALL_OFFSET}
        COMPONENT ${MODULE_PROJECT_NAME})
      if(MSVC)
        set_target_properties(${MODULE_PROJECT_NAME} PROPERTIES
          RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug/modules
          RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release/modules
          RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo/modules
          RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/MinSizeRel/modules)
      endif()
    endif()
  else()
    message(FATAL_ERROR "Unknown value \"${${MODULE_VARIABLE}}\" for ${MODULE_VARIABLE}!")
  endif()
endforeach()

set(WORLDSERVER_DYNAMIC_MODULE_MODULES_DEPENDENCIES ${DYNAMIC_MODULE_PROJECTS} PARENT_SCOPE)

ConfigureModuleLoader("static" MODULE_STATIC_LOADER OFF ${STATIC_MODULES})

target_sources(modules
  PRIVATE
    ModulesScriptLoader.h
    ${MODULE_STATIC_LOADER})

target_link_libraries(modules
  PRIVATE
    trinity-core-interface
  PUBLIC
    game-interface)

target_include_directories(modules
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(modules
  PROPERTIES
    COMPILE_WARNING_AS_ERROR ${WITH_WARNINGS_AS_ERRORS}
    FOLDER "modules")

if(COPY_CONF)
  foreach(MODULE_CONFIG_FILE ${ENABLED_MODULE_CONFIG_FILES})
    get_filename_component(MODULE_CONFIG_FILENAME ${MODULE_CONFIG_FILE} NAME)
    if(WIN32 AND MSVC)
      add_custom_command(TARGET modules
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/modules"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MODULE_CONFIG_FILE}" "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/modules/${MODULE_CONFIG_FILENAME}")
    elseif(WIN32)
      add_custom_command(TARGET modules
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/modules"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MODULE_CONFIG_FILE}" "${CMAKE_BINARY_DIR}/bin/modules/${MODULE_CONFIG_FILENAME}")
    elseif(UNIX)
      install(FILES ${MODULE_CONFIG_FILE} DESTINATION "${CONF_DIR}/modules")
    endif()
  endforeach()
endif()

if(DISABLED_MODULE_PROJECTS)
  install(CODE "
    foreach(MODULE_TO_UNINSTALL ${DISABLED_MODULE_PROJECTS})
      if(EXISTS \"\${MODULE_TO_UNINSTALL}\")
        message(STATUS \"Uninstalling: \${MODULE_TO_UNINSTALL}\")
        file(REMOVE \"\${MODULE_TO_UNINSTALL}\")
      endif()
    endforeach()
  ")
endif()

ModuleNameToVariable(mod-playerbots MODULE_MOD_PLAYERBOTS_VARIABLE)
    if(${MODULE_MOD_PLAYERBOTS_VARIABLE} STREQUAL "static")
  target_compile_definitions(database PUBLIC WITH_PLAYERBOTS)
  target_link_libraries(modules PRIVATE database)
endif()

message("")

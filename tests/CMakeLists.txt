# This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(tests)

CollectAndAddSourceFiles(
  tests
  ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(tests
  PRIVATE
    trinity-core-interface
    game
    Catch2::Catch2)

target_include_directories(tests
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    # Layer-3 fixtures for mod-playerbots gated payload parse (Write()-mirrored readers).
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src)

target_sources(tests
  PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotPartyInvitePacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotGuildInvitePacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotResurrectRequestPacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotPetitionShowSignaturesPacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotBuyFailedPacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotGroupNewLeaderPacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotMoveSetRunSpeedPacket.cpp)

catch_discover_tests(tests)

set_target_properties(tests
    PROPERTIES
      FOLDER
        "tests")

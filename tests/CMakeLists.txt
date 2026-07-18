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
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotMoveSetRunSpeedPacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotInventoryChangeFailurePacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotTradeStatusPacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotTradeUpdatedPacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotLootResponsePacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotLootItemData.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotItemPushResultPacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotLootRollWonPacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotStartLootRollPacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotPartyCommandResultPacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotLevelUpInfoPacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotLogXPGainPacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotCastFailedPacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotSTextEmotePacket.cpp
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src/Bot/Packet/BotEmotePacket.cpp)

catch_discover_tests(tests)

set_target_properties(tests
    PROPERTIES
      FOLDER
        "tests")

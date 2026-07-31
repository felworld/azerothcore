#
# This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
CollectSourceFiles(
        ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE_SOURCES
)

# unit_tests links game but not the scripts library, so script files whose
# handlers are exercised by tests are compiled into the test binary directly.
list(APPEND PRIVATE_SOURCES
        ${CMAKE_SOURCE_DIR}/src/server/scripts/Commands/cs_pause.cpp
)

include_directories(
        "${CMAKE_CURRENT_SOURCE_DIR}/mocks"
)

add_executable(
        unit_tests
        ${PRIVATE_SOURCES}
)

# Static-archive link order must match the worldserver's (modules, scripts,
# game): the linker resolves archives left to right, and only game-interface
# (headers) is declared as a dependency of modules/scripts, so CMake won't
# reorder or repeat game for us. Module code may reference script-library
# types (e.g. the playerbots raid logic dynamic_casts creature AIs to boss
# classes), and both pull most of their symbols from game, so scripts follows
# modules and game follows both.
if(TARGET modules)
    target_link_libraries(unit_tests modules)
endif()

if(TARGET scripts)
    target_link_libraries(unit_tests scripts)
endif()

target_link_libraries(
        unit_tests
        game
        gtest_main
        gmock_main
        game-interface
)

# Add module test sources if any modules registered tests
get_property(MODULE_TEST_SOURCES GLOBAL PROPERTY ACORE_MODULE_TEST_SOURCES)
get_property(MODULE_TEST_INCLUDES GLOBAL PROPERTY ACORE_MODULE_TEST_INCLUDES)

if(MODULE_TEST_SOURCES)
    target_sources(unit_tests PRIVATE ${MODULE_TEST_SOURCES})
    message(STATUS "Added module tests to unit_tests target")
endif()

if(MODULE_TEST_INCLUDES)
    list(REMOVE_DUPLICATES MODULE_TEST_INCLUDES)
    target_include_directories(unit_tests PRIVATE ${MODULE_TEST_INCLUDES})
    message(STATUS "Added module test includes to unit_tests target")
endif()

add_test(
        NAME
        unit
        COMMAND
        ${CMAKE_BINARY_DIR}/src/test/unit_tests
)

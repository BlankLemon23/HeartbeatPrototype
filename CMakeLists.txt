cmake_minimum_required(VERSION 3.20)
project(HeartbeatPrototype LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
)
FetchContent_MakeAvailable(raylib)

add_library(game_core STATIC game_core/Map.cpp game_core/Position.cpp game_core/Player.cpp game_core/ScanController.cpp game_core/Enemy.cpp "game_core/Weapons/Molotov.cpp")
target_include_directories(game_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/game_core")
target_compile_features(game_core PUBLIC cxx_std_17)
if(MSVC)
    target_compile_options(game_core PUBLIC /utf-8)
endif()

add_executable(game_desktop game_desktop/main.cpp)
target_link_libraries(game_desktop PRIVATE game_core raylib)

add_executable(game_tests game_tests/MapTests.cpp)
target_link_libraries(game_tests PRIVATE game_core)

add_executable(enemy_tests game_tests/EnemyTests.cpp)
target_link_libraries(enemy_tests PRIVATE game_core)

# Keep assertions active in the test executable in every build configuration.
if(MSVC)
    target_compile_options(game_tests PRIVATE /UNDEBUG)
    target_compile_options(enemy_tests PRIVATE /UNDEBUG)
else()
    target_compile_options(game_tests PRIVATE -UNDEBUG)
    target_compile_options(enemy_tests PRIVATE -UNDEBUG)
endif()

enable_testing()
add_test(NAME map_rules COMMAND game_tests)
add_test(NAME enemy_behavior COMMAND enemy_tests)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT game_desktop)

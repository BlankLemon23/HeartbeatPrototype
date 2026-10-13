cmake_minimum_required(VERSION 3.20)
project(HeartbeatPrototype LANGUAGES CXX)

add_library(game_core STATIC game_core/Map.cpp)
target_include_directories(game_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/game_core")
target_compile_features(game_core PUBLIC cxx_std_17)

add_executable(game_tests game_tests/MapTests.cpp)
target_link_libraries(game_tests PRIVATE game_core)

# Keep assertions active in the test executable in every build configuration.
if(MSVC)
    target_compile_options(game_tests PRIVATE /UNDEBUG)
else()
    target_compile_options(game_tests PRIVATE -UNDEBUG)
endif()

enable_testing()
add_test(NAME map_rules COMMAND game_tests)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT game_tests)

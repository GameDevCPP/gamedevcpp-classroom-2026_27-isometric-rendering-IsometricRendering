cmake_minimum_required(VERSION 3.21)
project(CMakeSFMLProject LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.x)
FetchContent_MakeAvailable(SFML)

# Dear ImGui + ImGui-SFML, optional, wired into the engine loop already.
# Pinned to v1.89: the version imgui-sfml's 2.6.x branch is tested against.
# Newer ImGui changed the texture-ID API and won't compile against it.
FetchContent_Declare(imgui
    URL "https://github.com/ocornut/imgui/archive/v1.89.zip")
FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
option(IMGUI_SFML_FIND_SFML "Use find_package to find SFML" OFF)
option(IMGUI_SFML_IMGUI_DEMO "Build imgui_demo.cpp" ON)
add_compile_definitions(IMGUI_SFML_DISABLE_OBSOLETE_FUNCTIONS)
FetchContent_Declare(imgui-sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG 2.6.x
    PATCH_COMMAND git apply --ignore-whitespace ${CMAKE_CURRENT_SOURCE_DIR}/patches/fix-clipboard-u8string.patch || true)
FetchContent_MakeAvailable(imgui-sfml)

# B2D - Box2D physics library (unused by this practical, but the engine's
# system_physics links against it either way)
add_subdirectory("lib/b2d/Box2D")
set(B2D_INCS "lib/b2d/Box2D/")

file(GLOB_RECURSE RESOURCE_FILES "res/*.*")
file(GLOB_RECURSE SOURCE_FILES engine/*.cpp engine/*.h)
add_library(lib_engine STATIC ${SOURCE_FILES} ${RESOURCE_FILES})
target_include_directories(lib_engine INTERFACE "${CMAKE_SOURCE_DIR}/engine/" PUBLIC SYSTEM ${B2D_INCS})
target_link_libraries(lib_engine lib_tile_level_loader lib_maths lib_ecm Box2D sfml-graphics sfml-audio ImGui-SFML::ImGui-SFML)

file(GLOB_RECURSE SOURCE_FILES lib_tile_level_loader/*.cpp lib_tile_level_loader/*.h)
add_library(lib_tile_level_loader STATIC ${SOURCE_FILES})
target_include_directories(lib_tile_level_loader INTERFACE "${CMAKE_SOURCE_DIR}/lib_tile_level_loader/" )
target_link_libraries(lib_tile_level_loader lib_maths sfml-graphics)

add_library(lib_maths INTERFACE)
target_sources(lib_maths INTERFACE "${CMAKE_SOURCE_DIR}/lib_maths/maths.h")
target_include_directories(lib_maths INTERFACE "${CMAKE_SOURCE_DIR}/lib_maths")
target_link_libraries(lib_maths INTERFACE sfml-system)

file(GLOB_RECURSE SOURCE_FILES lib_ecm/*.cpp lib_ecm/*.h)
add_library(lib_ecm STATIC ${SOURCE_FILES})
target_include_directories(lib_ecm INTERFACE "${CMAKE_SOURCE_DIR}/lib_ecm" )
target_link_libraries(lib_ecm PRIVATE lib_maths)

file(GLOB_RECURSE GAME_SOURCES src/*.cpp src/*.h)
add_executable(CMakeSFMLProject ${GAME_SOURCES})
target_link_libraries(CMakeSFMLProject PRIVATE sfml-graphics lib_engine)
target_compile_features(CMakeSFMLProject PRIVATE cxx_std_17)
if (WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET CMakeSFMLProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:CMakeSFMLProject> $<TARGET_FILE_DIR:CMakeSFMLProject> COMMAND_EXPAND_LISTS)
endif()

add_custom_target(copy_resources ALL COMMAND ${CMAKE_COMMAND}
        -E copy_directory
        "${PROJECT_SOURCE_DIR}/res"
        "${CMAKE_BINARY_DIR}/res"
)
add_dependencies(CMakeSFMLProject copy_resources)

install(TARGETS CMakeSFMLProject)

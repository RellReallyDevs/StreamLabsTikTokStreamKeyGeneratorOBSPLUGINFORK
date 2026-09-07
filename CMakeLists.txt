cmake_minimum_required(VERSION 3.16...3.28)

project(tiktok-live-dock VERSION 1.0.0 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# This CMakeLists builds the native OBS 32+ plugin found in src/.
# It expects to be built either:
#   1) Inside an obs-studio source tree (as a UI/frontend-plugins subfolder), or
#   2) Standalone against an OBS "dev" package that exports libobs/obs-frontend-api
#      CMake config files (as produced by obs-studio's own build + install step).
#
# The legacy standalone PySide6 app (Stream.py, TokenRetriever.py,
# StreamLabsTikTokStreamKeyGenerator.py, Updater.py) is kept at the repo root
# for reference; it is not required to build this plugin.
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(POLICY CMP0100)
  cmake_policy(SET CMP0100 NEW)
endif()

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_library(tiktok-live-dock MODULE)

target_sources(tiktok-live-dock PRIVATE
    src/plugin-main.cpp
    src/TikTokDock.cpp
    src/TikTokDock.h
    src/StreamClient.cpp
    src/StreamClient.h
)

target_include_directories(tiktok-live-dock PRIVATE src)

target_link_libraries(tiktok-live-dock PRIVATE
    OBS::libobs
    OBS::obs-frontend-api
    Qt6::Widgets
    Qt6::Network
)

set_target_properties(tiktok-live-dock PROPERTIES
    PREFIX ""
    OUTPUT_NAME "tiktok-live-dock"
)

if(OS_WINDOWS OR WIN32)
    target_compile_definitions(tiktok-live-dock PRIVATE UNICODE _UNICODE)
endif()

# --- Install layout matching a normal OBS plugin package ---------------------
# <plugin>/bin/64bit/tiktok-live-dock.[dll|so|dylib]
# <plugin>/data/locale/en-US.ini
install(TARGETS tiktok-live-dock
    LIBRARY DESTINATION "obs-plugins/64bit"
    RUNTIME DESTINATION "obs-plugins/64bit"
)

install(DIRECTORY data/
    DESTINATION "data/obs-plugins/tiktok-live-dock"
)

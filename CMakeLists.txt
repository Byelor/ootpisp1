cmake_minimum_required(VERSION 3.20)
project(GraphEditor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(SFML_STATIC_LIBRARIES ON CACHE BOOL "Build SFML as static libraries" FORCE)

include(FetchContent)

# Fetch SFML 2.6.2
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.2
)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SFML)

# Fetch ImGui-SFML (which will fetch ImGui internally usually, or we need to provide it)
FetchContent_Declare(ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.4-docking
)
FetchContent_MakeAvailable(ImGui)

FetchContent_Declare(ImGui-SFML
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG v2.6.1 # v2.6.1 works with SFML 2.6
)
# We need to tell ImGui-SFML where ImGui is
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)

# Add aliases for ImGui-SFML to find SFML targets
if (NOT TARGET SFML::Graphics)
    add_library(SFML::System ALIAS sfml-system)
    add_library(SFML::Window ALIAS sfml-window)
    add_library(SFML::Graphics ALIAS sfml-graphics)
endif()

FetchContent_MakeAvailable(ImGui-SFML)

# Create executable
add_executable(GraphEditor
    src/main.cpp
    src/core/Figure.cpp
    src/core/PolylineFigure.cpp
    src/core/Figures.cpp
    src/core/CompositeFigure.cpp
    src/core/Scene.cpp
    src/core/SceneArray.cpp
    src/core/SceneSerializer.cpp
    src/ui/Toolbar.cpp
    src/ui/PropertiesPanel.cpp
    src/ui/CreateFigureModal.cpp
    src/ui/LayerPanel.cpp
    src/utils/GeometryUtils.cpp
)

target_include_directories(GraphEditor PRIVATE src)

# Add optimization for Linux (Ubuntu)
if(UNIX AND NOT APPLE)
    # Enable O3 optimization and native architecture tuning for best performance on Linux
    target_compile_options(GraphEditor PRIVATE -O3 -march=native)
endif()


# Link
target_link_libraries(GraphEditor PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    ImGui-SFML::ImGui-SFML
)

if(WIN32)
    target_link_libraries(GraphEditor PRIVATE
        gdi32  # для графики
        opengl32
    )
endif()

# On Mac we may need to link some frameworks if statically linking, but FetchContent usually handles it
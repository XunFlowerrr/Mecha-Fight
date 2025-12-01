cmake_minimum_required (VERSION 3.8)
cmake_policy(VERSION 3.8)

project(LearnOpenGL)

set(CMAKE_CXX_STANDARD 17) # this does nothing for MSVC, use target_compile_options below
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Option to create a distributable build with bundled resources
option(BUILD_DISTRIBUTABLE "Copy resources to output directory for distribution" OFF)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

if(WIN32)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
endif(WIN32)

link_directories(${CMAKE_SOURCE_DIR}/lib)

# find the required packages
find_package(GLM REQUIRED)
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")
find_package(GLFW3 REQUIRED)
message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIR}")
find_package(ASSIMP REQUIRED)
message(STATUS "Found ASSIMP in ${ASSIMP_INCLUDE_DIR}")
# find_package(SOIL REQUIRED)
# message(STATUS "Found SOIL in ${SOIL_INCLUDE_DIR}")
# find_package(GLEW REQUIRED)
# message(STATUS "Found GLEW in ${GLEW_INCLUDE_DIR}")
if(APPLE)
  find_package(Freetype REQUIRED)
  message(STATUS "Found Freetype in ${FREETYPE_INCLUDE_DIRS}")
endif(APPLE)

if(WIN32)
  set(LIBS glfw3 opengl32 assimp freetype irrKlang)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
elseif(UNIX AND NOT APPLE)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
  find_package(OpenGL REQUIRED)
  add_definitions(${OPENGL_DEFINITIONS})
  find_package(X11 REQUIRED)
  # note that the order is important for setting the libs
  # use pkg-config --libs $(pkg-config --print-requires --print-requires-private glfw3) in a terminal to confirm
  set(LIBS ${GLFW3_LIBRARY} X11 Xrandr Xinerama Xi Xxf86vm Xcursor GL dl pthread freetype ${ASSIMP_LIBRARY})
  set (CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")
elseif(APPLE)
  INCLUDE_DIRECTORIES(/System/Library/Frameworks)
  FIND_LIBRARY(COCOA_LIBRARY Cocoa)
  FIND_LIBRARY(OpenGL_LIBRARY OpenGL)
  FIND_LIBRARY(IOKit_LIBRARY IOKit)
  FIND_LIBRARY(CoreVideo_LIBRARY CoreVideo)
  MARK_AS_ADVANCED(COCOA_LIBRARY OpenGL_LIBRARY)
  SET(APPLE_LIBS ${COCOA_LIBRARY} ${IOKit_LIBRARY} ${OpenGL_LIBRARY} ${CoreVideo_LIBRARY})
  SET(APPLE_LIBS ${APPLE_LIBS} ${GLFW3_LIBRARY} ${ASSIMP_LIBRARY} ${FREETYPE_LIBRARIES})
  set(LIBS ${LIBS} ${APPLE_LIBS})
else()
  set(LIBS )
endif(WIN32)

configure_file(configuration/root_directory.h.in configuration/root_directory.h)
include_directories(${CMAKE_BINARY_DIR}/configuration)

# first create relevant static libraries required for other projects
add_library(STB_IMAGE "src/stb_image.cpp")
set(LIBS ${LIBS} STB_IMAGE)

add_library(GLAD "src/glad.c")
set(LIBS ${LIBS} GLAD)

add_library(TINYGLTF "src/tiny_gltf_impl.cpp")
set(LIBS ${LIBS} TINYGLTF)

macro(makeLink src dest target)
  add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${src} ${dest} COMMENT "mklink ${src} -> ${dest}")
endmacro()

include_directories(${CMAKE_SOURCE_DIR}/includes)

# Automatically collect mecha_fight source files
file(GLOB_RECURSE MECHA_FIGHT_SOURCES
  "src/mecha_fight/*.cpp"
  "src/mecha_fight/*.h"
)

add_executable(mecha_fight ${MECHA_FIGHT_SOURCES})
target_link_libraries(mecha_fight ${LIBS})

if(MSVC)
    target_compile_options(mecha_fight PRIVATE /std:c++17 /MP)
    target_link_options(mecha_fight PUBLIC /ignore:4099)
endif(MSVC)

set(MECHA_FIGHT_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/mecha_fight")

if(WIN32)
    set_target_properties(mecha_fight PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${MECHA_FIGHT_OUTPUT_DIR}")
    set_target_properties(mecha_fight PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${MECHA_FIGHT_OUTPUT_DIR}/Debug")
elseif(UNIX AND NOT APPLE)
    set_target_properties(mecha_fight PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${MECHA_FIGHT_OUTPUT_DIR}")
elseif(APPLE)
    set_target_properties(mecha_fight PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${MECHA_FIGHT_OUTPUT_DIR}")
    set_target_properties(mecha_fight PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${MECHA_FIGHT_OUTPUT_DIR}")
    set_target_properties(mecha_fight PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${MECHA_FIGHT_OUTPUT_DIR}")
endif()

file(GLOB MECHA_FIGHT_SHADERS
  "src/mecha_fight/shaders/*.vs"
  "src/mecha_fight/shaders/*.fs"
  "src/mecha_fight/shaders/*.tcs"
  "src/mecha_fight/shaders/*.tes"
  "src/mecha_fight/shaders/*.gs"
  "src/mecha_fight/shaders/*.cs"
)
file(GLOB MECHA_DLLS "dlls/*.dll")

if(WIN32)
    add_custom_command(TARGET mecha_fight PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory ${MECHA_FIGHT_OUTPUT_DIR})
    add_custom_command(TARGET mecha_fight PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MECHA_FIGHT_SHADERS} $<TARGET_FILE_DIR:mecha_fight>)
    add_custom_command(TARGET mecha_fight PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MECHA_DLLS} $<TARGET_FILE_DIR:mecha_fight>)
elseif(UNIX AND NOT APPLE)
    if(MECHA_FIGHT_SHADERS)
        file(COPY ${MECHA_FIGHT_SHADERS} DESTINATION ${MECHA_FIGHT_OUTPUT_DIR})
    endif()
elseif(APPLE)
    foreach(SHADER ${MECHA_FIGHT_SHADERS})
        get_filename_component(SHADERNAME ${SHADER} NAME)
        makeLink(${SHADER} ${MECHA_FIGHT_OUTPUT_DIR}/${SHADERNAME} mecha_fight)
    endforeach()
endif()

# ==========================================
# DISTRIBUTABLE BUILD - Bundle resources
# ==========================================
# To create a distributable build, run:
#   cmake -DBUILD_DISTRIBUTABLE=ON ..
#   make mecha_fight
#   make copy_resources
#
# This will copy all resources to bin/mecha_fight/resources/
# making the executable fully portable.

if(BUILD_DISTRIBUTABLE)
    # Create a custom target to copy resources
    add_custom_target(copy_resources
        COMMAND ${CMAKE_COMMAND} -E echo "Copying resources to ${MECHA_FIGHT_OUTPUT_DIR}/resources..."
        COMMAND ${CMAKE_COMMAND} -E make_directory "${MECHA_FIGHT_OUTPUT_DIR}/resources"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/resources" "${MECHA_FIGHT_OUTPUT_DIR}/resources"
        COMMENT "Bundling resources for distribution"
    )

    # Also copy shaders directly (not as symlinks) for distribution
    add_custom_target(copy_shaders_dist
        COMMAND ${CMAKE_COMMAND} -E echo "Copying shaders to ${MECHA_FIGHT_OUTPUT_DIR}..."
        COMMAND ${CMAKE_COMMAND} -E copy ${MECHA_FIGHT_SHADERS} "${MECHA_FIGHT_OUTPUT_DIR}/"
        COMMENT "Copying shaders for distribution"
    )

    # Create a combined distribution target
    add_custom_target(dist
        DEPENDS mecha_fight copy_resources copy_shaders_dist
        COMMENT "Creating distributable build"
    )

    # On Windows, also copy DLLs
    if(WIN32)
        add_custom_target(copy_dlls_dist
            COMMAND ${CMAKE_COMMAND} -E copy ${MECHA_DLLS} "${MECHA_FIGHT_OUTPUT_DIR}/"
            COMMENT "Copying DLLs for distribution"
        )
        add_dependencies(dist copy_dlls_dist)
    endif()
endif()

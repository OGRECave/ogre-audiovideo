cmake_minimum_required(VERSION 3.18)

# Build the OGRE development prefix used to compile the OggSound wheel.
# Only OgreMain and its headers (including Ogre.i) are needed; OGRE's Python
# bindings and all optional components/plugins/render systems are disabled.
# OGRE builds SDL2 for mojoAL as part of its dependency bootstrap.
#
# Required environment variables:
#   OGRE_TAG        upstream OGRE tag, e.g. v14.6.2
#   OGRE_PREFIX     installation prefix for OGRE
#   OGRE_BUILD_DIR  directory for the source and build tree
# Optional:
#   JOBS            parallel build jobs (default: 4)

foreach(var IN ITEMS OGRE_TAG OGRE_PREFIX OGRE_BUILD_DIR)
    if(NOT DEFINED ENV{${var}} OR "$ENV{${var}}" STREQUAL "")
        message(FATAL_ERROR "${var} must be set")
    endif()
    set(${var} "$ENV{${var}}")
endforeach()

if(DEFINED ENV{JOBS} AND NOT "$ENV{JOBS}" STREQUAL "")
    set(JOBS "$ENV{JOBS}")
else()
    set(JOBS 4)
endif()

string(REGEX REPLACE "^v" "" OGRE_VERSION "${OGRE_TAG}")
set(source_dir "${OGRE_BUILD_DIR}/ogre-${OGRE_VERSION}")
set(build_dir "${OGRE_BUILD_DIR}/build")

if(NOT EXISTS "${source_dir}/CMakeLists.txt")
    file(MAKE_DIRECTORY "${OGRE_BUILD_DIR}")
    set(archive "${OGRE_BUILD_DIR}/ogre-${OGRE_VERSION}.tar.gz")
    if(NOT EXISTS "${archive}")
        message(STATUS "Downloading OGRE ${OGRE_TAG}")
        file(DOWNLOAD
            "https://github.com/OGRECave/ogre/archive/refs/tags/${OGRE_TAG}.tar.gz"
            "${archive}"
            SHOW_PROGRESS
            TLS_VERIFY ON)
    endif()
    message(STATUS "Extracting OGRE ${OGRE_TAG}")
    file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${OGRE_BUILD_DIR}")
endif()

set(OGRE_OPTIONS
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX=${OGRE_PREFIX}
    -DOGRE_DEPENDENCIES_DIR=${OGRE_PREFIX}/deps
    -DSKBUILD=ON
    -DOGRE_LIB_DIRECTORY=lib
    # OGRE defaults this to "CMake" on Windows and macOS, but to
    # lib/OGRE/cmake elsewhere. Pin it so the prefix layout is the same
    # everywhere and can be found through CMAKE_PREFIX_PATH.
    -DOGRE_CMAKE_DIR=lib/OGRE/cmake
    -DOGRE_NODELESS_POSITIONING=OFF
    -DOGRE_BUILD_DEPENDENCIES=ON
    -DOGRE_INSTALL_DEPENDENCIES=OFF

    -DOGRE_BUILD_COMPONENT_PYTHON=OFF
    -DSWIG_EXECUTABLE=none
    -DOGRE_BUILD_COMPONENT_OVERLAY=OFF
    -DOGRE_BUILD_COMPONENT_OVERLAY_IMGUI=OFF
    -DOGRE_BUILD_COMPONENT_BITES=OFF
    -DOGRE_BUILD_COMPONENT_BULLET=OFF
    -DOGRE_BUILD_COMPONENT_PAGING=OFF
    -DOGRE_BUILD_COMPONENT_MESHLODGENERATOR=OFF
    -DOGRE_BUILD_COMPONENT_TERRAIN=OFF
    -DOGRE_BUILD_COMPONENT_VOLUME=OFF
    -DOGRE_BUILD_COMPONENT_PROPERTY=OFF
    -DOGRE_BUILD_COMPONENT_RTSHADERSYSTEM=OFF
    -DOGRE_BUILD_RTSHADERSYSTEM_SHADERS=OFF
    -DOGRE_BUILD_COMPONENT_JAVA=OFF
    -DOGRE_BUILD_COMPONENT_CSHARP=OFF

    -DOGRE_BUILD_PLUGIN_ASSIMP=OFF
    -DOGRE_BUILD_PLUGIN_BSP=OFF
    -DOGRE_BUILD_PLUGIN_OCTREE=OFF
    -DOGRE_BUILD_PLUGIN_PFX=OFF
    -DOGRE_BUILD_PLUGIN_DOT_SCENE=OFF
    -DOGRE_BUILD_PLUGIN_PCZ=OFF
    -DOGRE_BUILD_PLUGIN_CG=OFF
    -DOGRE_BUILD_PLUGIN_FREEIMAGE=OFF
    -DOGRE_BUILD_PLUGIN_EXRCODEC=OFF
    -DOGRE_BUILD_PLUGIN_STBI=OFF
    -DOGRE_BUILD_PLUGIN_RSIMAGE=OFF
    -DOGRE_BUILD_PLUGIN_GLSLANG=OFF

    -DOGRE_BUILD_RENDERSYSTEM_GL=OFF
    -DOGRE_BUILD_RENDERSYSTEM_GL3PLUS=OFF
    -DOGRE_BUILD_RENDERSYSTEM_GLES2=OFF
    -DOGRE_BUILD_RENDERSYSTEM_TINY=OFF
    -DOGRE_BUILD_RENDERSYSTEM_VULKAN=OFF
    -DOGRE_BUILD_RENDERSYSTEM_D3D9=OFF
    -DOGRE_BUILD_RENDERSYSTEM_D3D11=OFF
    -DOGRE_BUILD_RENDERSYSTEM_METAL=OFF

    -DOGRE_BUILD_SAMPLES=OFF
    -DOGRE_BUILD_TOOLS=OFF
    -DOGRE_BUILD_TESTS=OFF
    -DOGRE_BUILD_XSIEXPORTER=OFF
    -DOGRE_BUILD_FUZZERS=OFF
)

if(APPLE)
    list(APPEND OGRE_OPTIONS
        -DCMAKE_OSX_ARCHITECTURES=arm64
        -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0
        -DOGRE_BUILD_LIBS_AS_FRAMEWORKS=OFF
    )
endif()

message(STATUS "Configuring OGRE ${OGRE_TAG}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${source_dir}" -B "${build_dir}" ${OGRE_OPTIONS}
    RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "OGRE configure failed (${result})")
endif()

message(STATUS "Building OGRE ${OGRE_TAG}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${build_dir}" --target install
            --config Release -j "${JOBS}"
    RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "OGRE build failed (${result})")
endif()

file(GLOB SDL2_CONFIG "${OGRE_PREFIX}/deps/*/cmake/SDL2/SDL2Config.cmake"
                      "${OGRE_PREFIX}/deps/cmake/SDL2Config.cmake")
set(OGRE_CONFIG "${OGRE_PREFIX}/lib/OGRE/cmake/OGREConfig.cmake")
if(NOT EXISTS "${OGRE_CONFIG}" OR NOT SDL2_CONFIG)
    message(FATAL_ERROR
        "OGRE prefix at ${OGRE_PREFIX} is incomplete: ${OGRE_CONFIG} / ${SDL2_CONFIG}")
endif()
list(GET SDL2_CONFIG 0 SDL2_CONFIG)

get_filename_component(SDL2_CONFIG_DIR "${SDL2_CONFIG}" DIRECTORY)
message(STATUS "OGRE installed to ${OGRE_PREFIX}")
message(STATUS "Set CMAKE_PREFIX_PATH=\"${OGRE_PREFIX};${OGRE_PREFIX}/deps\"")
message(STATUS "  OGRE config: ${OGRE_CONFIG}")
message(STATUS "  SDL2 config: ${SDL2_CONFIG_DIR}")

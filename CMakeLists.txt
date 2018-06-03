cmake_minimum_required (VERSION 2.8)
project(OgreVideo)

find_package(OGRE 1.11 REQUIRED COMPONENTS Bites)

include(GenerateExportHeader)

file(GLOB PLAYER_SRC theoraplayer/src/*cpp)
add_library(theoraplayer STATIC ${PLAYER_SRC})
target_include_directories(theoraplayer PUBLIC ${CMAKE_BINARY_DIR}/include/ theoraplayer/include)
target_link_libraries(theoraplayer PRIVATE theoradec)
set_property(TARGET theoraplayer PROPERTY POSITION_INDEPENDENT_CODE ON)
generate_export_header(theoraplayer 
    EXPORT_MACRO_NAME TheoraPlayerExport
    EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/include/TheoraExport.h)

add_library(Plugin_TheoraVideoSystem SHARED
    src/OgreTheoraDataStream.cpp
    src/OgreVideoDLLmain.cpp
    src/OgreVideoManager.cpp)
target_include_directories(Plugin_TheoraVideoSystem PUBLIC include/)
target_link_libraries(Plugin_TheoraVideoSystem PUBLIC theoraplayer OgreMain)
set_target_properties(Plugin_TheoraVideoSystem PROPERTIES PREFIX "")

add_executable(player demos/player/player.cpp)
target_link_libraries(player OgreBites Plugin_TheoraVideoSystem)

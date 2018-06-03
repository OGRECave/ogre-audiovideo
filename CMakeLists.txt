cmake_minimum_required (VERSION 2.8)
project(OgreVideo)

find_package(OGRE 1.11 REQUIRED COMPONENTS Bites)

add_library(Plugin_TheoraVideoSystem SHARED
    src/OgreTheoraDataStream.cpp
    src/OgreVideoDLLmain.cpp
    src/OgreVideoManager.cpp)
target_include_directories(Plugin_TheoraVideoSystem PUBLIC include/)
target_link_libraries(Plugin_TheoraVideoSystem PUBLIC OgreMain)
set_target_properties(Plugin_TheoraVideoSystem PROPERTIES PREFIX "")

add_executable(Player demos/player/player.cpp)
target_link_libraries(Player Plugin_TheoraVideoSystem ${OGRE_LIBRARIES})
target_include_directories(Player PRIVATE demos/include/)

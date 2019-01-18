cmake_minimum_required (VERSION 2.8)
project(OgreVideo)

find_package(OGRE 1.11 REQUIRED OPTIONAL_COMPONENTS Bites)

option(BUILD_DEMOS "Build demos (required Ogre Bites)" ON)
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
target_include_directories(Plugin_TheoraVideoSystem PUBLIC include/ ${OGRE_INCLUDE_DIRS})
target_link_libraries(Plugin_TheoraVideoSystem PUBLIC theoraplayer OgreMain)
set_target_properties(Plugin_TheoraVideoSystem PROPERTIES PREFIX "")

if (NOT OGRE_Bites_FOUND)
	if (BUILD_DEMOS)
		message(WARNING "NOT found OGRE Bites - disable build demos")
	endif (BUILD_DEMOS)
	set(BUILD_DEMOS OFF)
endif (NOT OGRE_Bites_FOUND)

if (BUILD_DEMOS)
	add_executable(player demos/player/player.cpp)
	target_link_libraries(player OgreBites Plugin_TheoraVideoSystem)
	
	file(COPY demos/resources.cfg DESTINATION ${CMAKE_BINARY_DIR})
endif (BUILD_DEMOS)

# doxygen stuff
find_package(Doxygen)
if (DOXYGEN_FOUND)
  # prepare doxygen configuration file
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  add_custom_target( docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating documentation with Doxygen."
    VERBATIM
  )

  # cleanup $build/api-doc on "make clean"
  set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES docs)
endif (DOXYGEN_FOUND)

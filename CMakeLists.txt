cmake_minimum_required (VERSION 3.10)
project(OgreVideoAudio)

# OGRE 14 requires C++14. Do not rely on the compiler default: AppleClang
# defaults to a pre-C++11 dialect, which cannot even parse the OGRE headers.
if(NOT DEFINED CMAKE_CXX_STANDARD)
	set(CMAKE_CXX_STANDARD 14)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

find_package(OGRE REQUIRED OPTIONAL_COMPONENTS Bites)
include_directories(${OGRE_INCLUDE_DIRS})

option(BUILD_DEMOS "Build demo (requires Ogre Bites and both Audio and Video)" ON)
option(BUILD_VIDEOPLUGIN "Build the Theora Video plugin" ON)
option(BUILD_AUDIOPLUGIN "Build the OggSound Audio component" ON)
include(GenerateExportHeader)

SET(CMAKE_DEBUG_POSTFIX "_d")

if(BUILD_VIDEOPLUGIN)
	add_subdirectory(theoravideo)
endif()

if(BUILD_AUDIOPLUGIN)
	add_subdirectory(oggsound)
endif()

if (NOT OGRE_Bites_FOUND)
	if (BUILD_DEMOS)
		message(WARNING "NOT found OGRE Bites - disable build demos")
	endif (BUILD_DEMOS)
	set(BUILD_DEMOS OFF)
endif (NOT OGRE_Bites_FOUND)

if (NOT BUILD_VIDEOPLUGIN OR NOT BUILD_AUDIOPLUGIN)
	if (BUILD_DEMOS)
		message(WARNING "Building Demos require both audio and video plugins - disable build demos")
	endif (BUILD_DEMOS)
	set(BUILD_DEMOS OFF)
endif (NOT BUILD_VIDEOPLUGIN OR NOT BUILD_AUDIOPLUGIN)

if (BUILD_DEMOS)
	add_executable(player demos/player/player.cpp)
	target_link_libraries(player OgreBites Plugin_TheoraVideoSystem OgreOggSound)

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

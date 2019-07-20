cmake_minimum_required (VERSION 2.8)
project(OgreVideoAudio)

find_package(OGRE 1.11 REQUIRED OPTIONAL_COMPONENTS Bites)

option(BUILD_DEMOS "Build demo (requires Ogre Bites and both Audio and Video)" ON)
option(BUILD_VIDEOPLUGIN "Build the Theora Video plugin" ON)
option(BUILD_AUDIOPLUGIN "Build the OggSound Audio plugin" ON)
include(GenerateExportHeader)

SET(CMAKE_DEBUG_POSTFIX "_d")
set(CMAKE_INSTALL_PREFIX ${OGRE_PREFIX_DIR} CACHE PATH "plugin must end up in OGRE plugin path" FORCE)

if(BUILD_VIDEOPLUGIN)
  find_path(THEORA_INCLUDE_DIRS NAMES theora\theoradec.h HINTS ${PC_THEORA_INCLUDE_DIRS} PATH_SUFFIXES theora)
  find_library(THEORADEC_LIBRARIES NAMES theoradec HINTS ${PC_THEORA_LIBRARY_DIRS})
  find_library(VORBIS_LIBRARIES NAMES vorbis HINTS ${PC_VORBIS_LIBRARY_DIRS})

  file(GLOB PLAYER_SRC theoraplayer/src/*cpp)
  file(GLOB PLAYER_H theoraplayer/include/*h)
  add_library(theoraplayer SHARED ${PLAYER_SRC} ${PLAYER_H})
  target_include_directories(theoraplayer PUBLIC ${CMAKE_BINARY_DIR}/include/ theoraplayer/include ${OGG_INCLUDE_DIRS} ${VORBIS_INCLUDE_DIRS} ${THEORA_INCLUDE_DIRS})
  target_link_libraries(theoraplayer PRIVATE ${OGG_LIBRARIES} ${VORBIS_LIBRARIES} ${VORBISFILE_LIBRARIES} ${THEORADEC_LIBRARIES})
  set_property(TARGET theoraplayer PROPERTY POSITION_INDEPENDENT_CODE ON)
  generate_export_header(theoraplayer
      EXPORT_MACRO_NAME TheoraPlayerExport
      EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/include/TheoraExport.h)

  set (PLUGIN_H
       include/OgreTheoraDataStream.h
       include/OgreVideoExport.h
       include/OgreVideoManager.h
  )
  set (PLUGIN_SRC
       src/OgreTheoraDataStream.cpp
       src/OgreVideoDLLmain.cpp
       src/OgreVideoManager.cpp
  )
  add_library(Plugin_TheoraVideoSystem SHARED ${PLUGIN_SRC} ${PLUGIN_H})
  target_compile_definitions(Plugin_TheoraVideoSystem PRIVATE THEORAVIDEO_PLUGIN_EXPORTS)
  target_include_directories(Plugin_TheoraVideoSystem PUBLIC include/ ${OGRE_INCLUDE_DIRS})
  target_link_libraries(Plugin_TheoraVideoSystem PUBLIC theoraplayer OgreMain)
  set_target_properties(Plugin_TheoraVideoSystem PROPERTIES PREFIX "")

  install(FILES ${PLUGIN_H} ${PLAYER_H} ${CMAKE_BINARY_DIR}/include/TheoraExport.h DESTINATION include/OGRE/Plugins/Theora)
  install(TARGETS theoraplayer RUNTIME DESTINATION bin/)
  install(TARGETS Plugin_TheoraVideoSystem RUNTIME DESTINATION bin/ LIBRARY DESTINATION lib/OGRE/ ARCHIVE DESTINATION lib/OGRE/)
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

if (BUILD_DEMOS)
	add_executable(player demos/player/player.cpp)
  target_link_libraries(player OgreBites Plugin_TheoraVideoSystem Plugin_OggSound)
	target_include_directories(player PUBLIC ${OPENAL_INCLUDE_DIRS})

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

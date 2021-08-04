find_path(THEORA_INCLUDE_DIRS NAMES theora/theoradec.h HINTS ${PC_THEORA_INCLUDE_DIRS} PATH_SUFFIXES theora)
find_library(THEORADEC_LIBRARIES NAMES theoradec HINTS ${PC_THEORA_LIBRARY_DIRS})
find_library(VORBIS_LIBRARIES NAMES vorbis HINTS ${PC_VORBIS_LIBRARY_DIRS})

file(GLOB PLAYER_SRC theoraplayer/src/*cpp)
file(GLOB PLAYER_H theoraplayer/include/*h)

# Configure library theoraplayer
if(OGRE_STATIC)
	add_library(theoraplayer STATIC ${PLAYER_SRC} ${PLAYER_H})
else()
	add_library(theoraplayer SHARED ${PLAYER_SRC} ${PLAYER_H})
endif()
target_include_directories(theoraplayer PUBLIC ${CMAKE_BINARY_DIR}/include/ theoraplayer/include ${OGG_INCLUDE_DIRS} ${VORBIS_INCLUDE_DIRS} ${THEORA_INCLUDE_DIRS})
target_link_libraries(theoraplayer PRIVATE ${OGG_LIBRARIES} ${VORBIS_LIBRARIES} ${VORBISFILE_LIBRARIES} ${THEORADEC_LIBRARIES})

# Add suffix with OGRE version
set_target_properties(theoraplayer PROPERTIES VERSION ${OGRE_VERSION} SOVERSION ${OGRE_VERSION})

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

# Configure Plugin TheoraVideoSystem
if(OGRE_STATIC)
	add_library(Plugin_TheoraVideoSystem STATIC ${PLUGIN_SRC} ${PLUGIN_H})
else()
	add_library(Plugin_TheoraVideoSystem SHARED ${PLUGIN_SRC} ${PLUGIN_H})
endif()
target_compile_definitions(Plugin_TheoraVideoSystem PRIVATE THEORAVIDEO_PLUGIN_EXPORTS)
target_include_directories(Plugin_TheoraVideoSystem PUBLIC include/ ${OGRE_INCLUDE_DIRS})
target_link_libraries(Plugin_TheoraVideoSystem PUBLIC theoraplayer OgreMain)

# Disable "lib" prefix and add a suffix with OGRE version
set_target_properties(Plugin_TheoraVideoSystem PROPERTIES PREFIX "" VERSION ${OGRE_VERSION} SOVERSION ${OGRE_VERSION})

install(FILES ${PLUGIN_H} ${PLAYER_H} ${CMAKE_BINARY_DIR}/include/TheoraExport.h DESTINATION include/OGRE/Plugins/Theora)
install(TARGETS theoraplayer RUNTIME DESTINATION bin/ LIBRARY DESTINATION lib/OGRE/ ARCHIVE DESTINATION lib/OGRE/)
install(TARGETS Plugin_TheoraVideoSystem RUNTIME DESTINATION bin/ LIBRARY DESTINATION lib/OGRE/ ARCHIVE DESTINATION lib/OGRE/)

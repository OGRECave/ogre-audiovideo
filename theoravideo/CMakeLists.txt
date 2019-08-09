find_path(THEORA_INCLUDE_DIRS NAMES theora/theoradec.h HINTS ${PC_THEORA_INCLUDE_DIRS} PATH_SUFFIXES theora)
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
install(TARGETS theoraplayer RUNTIME DESTINATION bin/ LIBRARY DESTINATION lib/OGRE/ ARCHIVE DESTINATION lib/OGRE/)
install(TARGETS Plugin_TheoraVideoSystem RUNTIME DESTINATION bin/ LIBRARY DESTINATION lib/OGRE/ ARCHIVE DESTINATION lib/OGRE/)

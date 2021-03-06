# define header and source files for the library
SET (HEADER_FILES
	include/LocklessQueue.h
    include/OgreOggISound.h
    include/OgreOggListener.h
    include/OgreOggSoundCallback.h
    include/OgreOggSoundFactory.h
    include/OgreOggSound.h
    include/OgreOggSoundManager.h
    include/OgreOggSoundPlugin.h
    include/OgreOggSoundPrereqs.h
    include/OgreOggSoundRecord.h
    include/OgreOggStaticSound.h
    include/OgreOggStaticWavSound.h
    include/OgreOggStreamBufferSound.h
    include/OgreOggStreamSound.h
    include/OgreOggStreamWavSound.h
)

SET (SOURCE_FILES
	src/OgreOggISound.cpp
    src/OgreOggListener.cpp
    src/OgreOggSoundFactory.cpp
    src/OgreOggSoundManager.cpp
    src/OgreOggSoundPlugin.cpp
    src/OgreOggSoundPluginDllStart.cpp
    src/OgreOggSoundRecord.cpp
    src/OgreOggStaticSound.cpp
    src/OgreOggStaticWavSound.cpp
    src/OgreOggStreamBufferSound.cpp
    src/OgreOggStreamSound.cpp
    src/OgreOggStreamWavSound.cpp
)
  
# Option.
option(OGGSOUND_THREADED "Enable multi-threaded streamed sounds" OFF)
option(USE_EFX "Use EFX? Disable this if you are using OpenAl Soft" OFF)

IF(OGGSOUND_THREADED)
    ADD_DEFINITIONS(-DOGGSOUND_THREADED=1)
ELSE()
    ADD_DEFINITIONS(-DOGGSOUND_THREADED=0)
ENDIF()

IF(USE_EFX)
	ADD_DEFINITIONS(-DHAVE_EFX=1)
ENDIF()

FIND_PACKAGE(PkgConfig QUIET)

PKG_CHECK_MODULES(PC_OPENAL QUIET openal)
FIND_PATH(OPENAL_INCLUDE_DIRS NAMES al.h HINTS ${PC_OPENAL_INCLUDE_DIRS} PATH_SUFFIXES openal)
FIND_LIBRARY(OPENAL_LIBRARIES NAMES openal HINTS ${PC_OPENAL_LIBRARY_DIRS})

PKG_CHECK_MODULES(PC_OGG QUIET ogg)
FIND_PATH(OGG_INCLUDE_DIRS NAMES ogg/ogg.h HINTS ${PC_OGG_INCLUDE_DIRS} PATH_SUFFIXES ogg)
FIND_LIBRARY(OGG_LIBRARIES NAMES ogg HINTS ${PC_OGG_LIBRARY_DIRS})

PKG_CHECK_MODULES(PC_VORBISFILE QUIET vorbisfile)
FIND_PATH(VORBIS_INCLUDE_DIRS NAMES vorbis/vorbisfile.h HINTS ${PC_VORBISFILE_INCLUDE_DIRS} PATH_SUFFIXES vorbisfile)
FIND_LIBRARY(VORBISFILE_LIBRARIES NAMES vorbisfile HINTS ${PC_VORBISFILE_LIBRARY_DIRS})

INCLUDE_DIRECTORIES(include
                    ${OGRE_INCLUDE_DIRS}
                    ${OPENAL_INCLUDE_DIRS}
                    ${OGG_INCLUDE_DIRS}
                    ${VORBIS_INCLUDE_DIRS})

LINK_DIRECTORIES(${OGRE_LIBRARY_DIRS}
                 ${OPENAL_LIBRARY_DIRS}
                 ${OGG_LIBRARY_DIRS}
                 ${VORBISFILE_LIBRARIES})

LINK_LIBRARIES(${OGRE_LIBRARIES}
               ${OPENAL_LIBRARIES}
               ${OGG_LIBRARIES}
               ${VORBISFILE_LIBRARIES})

ADD_LIBRARY(Plugin_OggSound SHARED ${SOURCE_FILES} ${HEADER_FILES})
TARGET_INCLUDE_DIRECTORIES(Plugin_OggSound PUBLIC include)
# disable "lib" prefix
SET_TARGET_PROPERTIES(Plugin_OggSound PROPERTIES PREFIX "")
TARGET_COMPILE_DEFINITIONS(Plugin_OggSound PRIVATE OGGSOUND_EXPORT)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/OGRE/Plugins/OggSound)
INSTALL(TARGETS Plugin_OggSound RUNTIME DESTINATION bin/ LIBRARY DESTINATION lib/OGRE/ ARCHIVE DESTINATION lib/OGRE/)
SET_PROPERTY(TARGET Plugin_OggSound PROPERTY INSTALL_RPATH ${OGRE_LIBRARY_DIRS})

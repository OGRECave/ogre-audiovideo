# define header and source files for the library
set (HEADER_FILES
	include/LocklessQueue.h
    include/OgreOggISound.h
    include/OgreOggListener.h
    include/OgreOggSoundCallback.h
    include/OgreOggSoundFactory.h
    include/OgreOggSound.h
    include/OgreOggSoundManager.h
    include/OgreOggSoundRoot.h
    include/OgreOggSoundRecord.h
    include/OgreOggStaticSound.h
    include/OgreOggStaticWavSound.h
    include/OgreOggStreamBufferSound.h
    include/OgreOggStreamSound.h
    include/OgreOggStreamWavSound.h
)
set (SOURCE_FILES
	src/OgreOggISound.cpp
    src/OgreOggListener.cpp
    src/OgreOggSoundFactory.cpp
    src/OgreOggSoundManager.cpp
    src/OgreOggSoundRoot.cpp
    src/OgreOggSoundRecord.cpp
    src/OgreOggStaticSound.cpp
    src/OgreOggStaticWavSound.cpp
    src/OgreOggStreamBufferSound.cpp
    src/OgreOggStreamSound.cpp
    src/OgreOggStreamWavSound.cpp
)
  
# Threading support
option(OGGSOUND_THREADED "Enable multi-threaded streamed sounds" OFF)

option(OGGSOUND_PYTHON "Build python bindings for oggsound" ON)

# OpenAL Soft Extensions
INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES (alext.h HAVE_ALEXT)

# Type of EFX backend support
set(USE_EFX 0 CACHE STRING
       "Specifies whether EFX enhancements are supported
       0 - EFX not supported
       1 - Enable EFX support with Creative OpenAL SDK 1.1
       2 - Enable EFX support with OpenAL Soft SDK"
)
set_property(CACHE USE_EFX PROPERTY STRINGS "0" "1" "2")

configure_file(include/OgreOggSoundPrereqs.h.in ${PROJECT_BINARY_DIR}/include/OgreOggSoundPrereqs.h @ONLY)
include_directories(${PROJECT_BINARY_DIR}/include)

find_package(OpenAL REQUIRED)

find_package(PkgConfig QUIET)
pkg_check_modules(PC_OGG QUIET ogg)
find_path(OGG_INCLUDE_DIRS NAMES ogg/ogg.h HINTS ${PC_OGG_INCLUDE_DIRS} PATH_SUFFIXES ogg)
find_library(OGG_LIBRARIES NAMES ogg HINTS ${PC_OGG_LIBRARY_DIRS})

pkg_check_modules(PC_VORBISFILE QUIET vorbisfile)
find_path(VORBIS_INCLUDE_DIRS NAMES vorbis/vorbisfile.h HINTS ${PC_VORBISFILE_INCLUDE_DIRS} PATH_SUFFIXES vorbisfile)
find_library(VORBISFILE_LIBRARIES NAMES vorbisfile HINTS ${PC_VORBISFILE_LIBRARY_DIRS})

# Configure library
if(OGRE_STATIC)
    add_library(OgreOggSound STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
    add_library(OgreOggSound SHARED ${SOURCE_FILES} ${HEADER_FILES})
endif()
target_include_directories(OgreOggSound PUBLIC include)
target_compile_definitions(OgreOggSound PRIVATE OGGSOUND_EXPORT)
target_include_directories(OgreOggSound PUBLIC include ${OPENAL_INCLUDE_DIR} PRIVATE ${OGG_INCLUDE_DIRS} ${VORBIS_INCLUDE_DIRS})
target_link_libraries(OgreOggSound PUBLIC OgreMain ${OPENAL_LIBRARY} PRIVATE ${OGG_LIBRARIES} ${VORBISFILE_LIBRARIES})

install(FILES ${HEADER_FILES} DESTINATION include/OGRE/OggSound)
install(FILES ${PROJECT_BINARY_DIR}/include/OgreOggSoundPrereqs.h DESTINATION include/OGRE/OggSound)
install(TARGETS OgreOggSound RUNTIME DESTINATION bin/ LIBRARY DESTINATION lib/ ARCHIVE DESTINATION lib/)
set_property(TARGET OgreOggSound PROPERTY INSTALL_RPATH ${OGRE_LIBRARY_DIRS})

if(OGGSOUND_PYTHON)
    add_subdirectory(python)
endif()

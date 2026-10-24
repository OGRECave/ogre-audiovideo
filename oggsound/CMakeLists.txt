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

option(OGGSOUND_STATIC "Build OgreOggSound as a static library (used by the python wheel)" OFF)

option(OGGSOUND_INSTALL_DEVEL "Install the C++ headers and library" ON)

option(OGGSOUND_USE_MOJOAL "Use the bundled mojoAL instead of the system OpenAL" ON)

# Type of EFX backend support
set(USE_EFX 0 CACHE STRING
       "Specifies whether EFX enhancements are supported
       0 - EFX not supported
       1 - Enable EFX support with Creative OpenAL SDK 1.1
       2 - Enable EFX support with OpenAL Soft SDK"
)
set_property(CACHE USE_EFX PROPERTY STRINGS "0" "1" "2")

if(OGGSOUND_USE_MOJOAL)
    # mojoAL is a single-file OpenAL 1.1 implementation built on SDL2.
    find_package(SDL2 REQUIRED)
    set(MOJOAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/mojoAL)
    add_library(mojoal STATIC ${MOJOAL_DIR}/mojoal.c)
    target_include_directories(mojoal PUBLIC ${MOJOAL_DIR}/AL)
    target_include_directories(mojoal PRIVATE ${SDL2_INCLUDE_DIRS})
    if(TARGET SDL2::SDL2)
        target_link_libraries(mojoal PUBLIC SDL2::SDL2)
    else()
        target_link_libraries(mojoal PUBLIC ${SDL2_LIBRARIES})
    endif()
    set_target_properties(mojoal PROPERTIES POSITION_INDEPENDENT_CODE ON)

    set(OGGSOUND_OPENAL_LIBRARIES mojoal)
    set(OGGSOUND_OPENAL_INCLUDE_DIRS "")
    # mojoAL ships no alext.h and has no EFX support.
    set(HAVE_ALEXT 0)
    if(USE_EFX)
        message(WARNING "mojoAL does not support EFX - forcing USE_EFX=0")
        set(USE_EFX 0 CACHE STRING "" FORCE)
    endif()
else()
    find_package(OpenAL REQUIRED)

    # OpenAL Soft Extensions
    include(CheckIncludeFiles)
    set(CMAKE_REQUIRED_INCLUDES ${OPENAL_INCLUDE_DIR})
    check_include_files(alext.h HAVE_ALEXT)
    unset(CMAKE_REQUIRED_INCLUDES)

    set(OGGSOUND_OPENAL_LIBRARIES ${OPENAL_LIBRARY})
    set(OGGSOUND_OPENAL_INCLUDE_DIRS ${OPENAL_INCLUDE_DIR})
endif()

configure_file(include/OgreOggSoundPrereqs.h.in ${PROJECT_BINARY_DIR}/include/OgreOggSoundPrereqs.h @ONLY)
include_directories(${PROJECT_BINARY_DIR}/include)

# stb_vorbis (Ogg Vorbis decoding)
set(STB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/stb)
add_library(stb_vorbis STATIC ${STB_DIR}/stb_vorbis.c)
target_include_directories(stb_vorbis PUBLIC ${STB_DIR})
target_compile_definitions(stb_vorbis PUBLIC STB_VORBIS_NO_STDIO STB_VORBIS_NO_PUSHDATA_API)
set_target_properties(stb_vorbis PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Configure library
if(OGGSOUND_STATIC OR OGRE_STATIC)
    add_library(OgreOggSound STATIC ${SOURCE_FILES} ${HEADER_FILES})
    set_target_properties(OgreOggSound PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
    add_library(OgreOggSound SHARED ${SOURCE_FILES} ${HEADER_FILES})
    target_compile_definitions(OgreOggSound PRIVATE OGGSOUND_EXPORT)
endif()
target_include_directories(OgreOggSound PUBLIC include)
target_include_directories(OgreOggSound PUBLIC include ${OGGSOUND_OPENAL_INCLUDE_DIRS})
target_link_libraries(OgreOggSound PUBLIC OgreMain ${OGGSOUND_OPENAL_LIBRARIES} PRIVATE stb_vorbis)

if(OGGSOUND_INSTALL_DEVEL)
    install(FILES ${HEADER_FILES} DESTINATION include/OGRE/OggSound)
    install(FILES ${PROJECT_BINARY_DIR}/include/OgreOggSoundPrereqs.h DESTINATION include/OGRE/OggSound)
    install(TARGETS OgreOggSound RUNTIME DESTINATION bin/ LIBRARY DESTINATION lib/ ARCHIVE DESTINATION lib/)
    set_property(TARGET OgreOggSound PROPERTY INSTALL_RPATH ${OGRE_LIBRARY_DIRS})
endif()

if(OGGSOUND_PYTHON)
    add_subdirectory(python)
endif()

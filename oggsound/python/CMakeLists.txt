find_package(Python3 COMPONENTS Interpreter Development.Module)
find_package(SWIG 4.1 COMPONENTS python)

if(NOT Python3_FOUND OR NOT SWIG_FOUND)
    if(SKBUILD)
        message(FATAL_ERROR "Python3 development files and SWIG 4.1+ are required for the wheel")
    endif()
    return()
endif()

include(${SWIG_USE_FILE})
include_directories(
    ${Python3_INCLUDE_DIRS}
    ${OGRE_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

set(CMAKE_SWIG_FLAGS -w401,314 -builtin)

# newer SWIG versions use the target name directly (CMP0078)
if(POLICY CMP0078)
    cmake_policy(SET CMP0078 NEW)
endif()

set(OGGSOUND_SWIG_INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../include/OgreOggSound.i)
set_source_files_properties(${OGGSOUND_SWIG_INTERFACE} PROPERTIES CPLUSPLUS ON)

if(MSVC)
    add_definitions(/wd4101 /wd4102)
else()
    add_definitions(-Wno-cast-qual -Wno-shadow -Wno-maybe-uninitialized -Wno-unused-label -Wno-unused-but-set-variable -Wno-missing-declarations)
endif()

swig_add_library(OggSound TYPE MODULE LANGUAGE python
    SOURCES ${OGGSOUND_SWIG_INTERFACE})
target_link_libraries(${SWIG_MODULE_OggSound_REAL_NAME} PRIVATE OgreOggSound OgreMain Python3::Module)
set_target_properties(${SWIG_MODULE_OggSound_REAL_NAME} PROPERTIES DEBUG_POSTFIX "")

# The bindings are part of the Ogre python package and expect the Ogre
# libraries (and the SDL2 used by mojoAL) next to them.
if(APPLE)
    set_target_properties(${SWIG_MODULE_OggSound_REAL_NAME} PROPERTIES INSTALL_RPATH "@loader_path")
elseif(UNIX)
    set_target_properties(${SWIG_MODULE_OggSound_REAL_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()

# when built by scikit-build install into the Ogre package inside the wheel
if(SKBUILD)
    set(OGGSOUND_PYTHON_INSTALL_DIR Ogre)
else()
    set(OGGSOUND_PYTHON_INSTALL_DIR lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages/Ogre)
endif()

install(TARGETS ${SWIG_MODULE_OggSound_REAL_NAME}
    LIBRARY DESTINATION ${OGGSOUND_PYTHON_INSTALL_DIR}
    RUNTIME DESTINATION ${OGGSOUND_PYTHON_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OggSound.py DESTINATION ${OGGSOUND_PYTHON_INSTALL_DIR})

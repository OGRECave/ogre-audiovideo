find_package(SWIG 3.0.8)
find_package(PythonInterp)
find_package(PythonLibs)

if(NOT PYTHONLIBS_FOUND OR NOT SWIG_FOUND)
    return()
endif()

include(${SWIG_USE_FILE})
include_directories(${PYTHON_INCLUDE_PATH} ${OGRE_INCLUDE_DIRS})

set(CMAKE_SWIG_FLAGS -w401,314 -builtin)

if(SWIG_VERSION VERSION_LESS "4.1")
    list(APPEND CMAKE_SWIG_FLAGS -Dfinal)
endif()

set_source_files_properties(${CMAKE_SOURCE_DIR}/oggsound/include/OgreOggSound.i PROPERTIES CPLUSPLUS ON)

if(MSVC)
	add_definitions(/wd4101 /wd4102)
else()
    add_definitions(-Wno-cast-qual -Wno-shadow -Wno-maybe-uninitialized -Wno-unused-label -Wno-unused-but-set-variable -Wno-missing-declarations)
endif()

swig_add_module(OgreOggSound python ${CMAKE_SOURCE_DIR}/oggsound/include/OgreOggSound.i)
swig_link_libraries(OgreOggSound OgreOggSound OgreMain ${PYTHON_LIBRARIES})
set_target_properties(${SWIG_MODULE_OgreOggSound_REAL_NAME} PROPERTIES DEBUG_POSTFIX "" INSTALL_RPATH ${OGRE_LIBRARY_DIRS})

# install
set(PYTHON_SITE_PACKAGES lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/dist-packages/)
install(TARGETS ${SWIG_MODULE_OgreOggSound_REAL_NAME} LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES})
install(FILES ${PROJECT_BINARY_DIR}/oggsound/python/OgreOggSound.py DESTINATION ${PYTHON_SITE_PACKAGES})
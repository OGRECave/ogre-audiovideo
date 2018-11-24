#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

##################################################################
# Provides some common functionality for the FindPackage modules
##################################################################


# Begin processing of package
macro(findpkg_begin PREFIX)
	if (NOT ${PREFIX}_FIND_QUIETLY)
		message(STATUS "Looking for ${PREFIX}...")
	endif ()
	
	unset(${PREFIX}_INCLUDE_DIRS)
	unset(${PREFIX}_LIBRARIES)
	set(${PREFIX}_EXTRALIB_MISSING 0)
	
	find_package(PkgConfig)
	if (PKG_CONFIG_FOUND)
		pkg_check_modules("${PREFIX}_PKGCM" ${PREFIX})
		if (${PREFIX}_FOUND)
			set(${PREFIX}_INCLUDE_DIR ${${PREFIX}_INCLUDE_DIRS} CACHE STRING)
			set(${PREFIX}_LIBRARY ${${PREFIX}_LIBRARIES} CACHE STRING)
			if (NOT ${PREFIX}_FIND_QUIETLY)
				message(STATUS "Found ${PREFIX}: ${${PREFIX}_LIBRARIES}")
			endif ()
		endif ()
	endif ()
endmacro(findpkg_begin)


# Display a status message unless FIND_QUIETLY is set
macro(pkg_message PREFIX)
	if (NOT ${PREFIX}_FIND_QUIETLY)
		message(STATUS ${ARGN})
	endif ()
endmacro(pkg_message)


# Add the parent dir from DIR to VAR 
macro(add_parent_dir VAR DIR)
	get_filename_component(${DIR}_TEMP "${${DIR}}/.." ABSOLUTE)
	set(${VAR} ${${VAR}} ${${DIR}_TEMP})
endmacro(add_parent_dir)


# Do the final processing for the package find.
macro(findpkg_finish PREFIX)
	# skip if already processed during this run
	if (NOT ${PREFIX}_FOUND)
		if (${PREFIX}_INCLUDE_DIR AND ${PREFIX}_LIBRARY AND NOT ${PREFIX}_EXTRALIB_MISSING)
			set(${PREFIX}_FOUND TRUE)
			set(${PREFIX}_INCLUDE_DIRS ${${PREFIX}_INCLUDE_DIRS} ${${PREFIX}_INCLUDE_DIR})
			set(${PREFIX}_LIBRARIES ${${PREFIX}_LIBRARIES} ${${PREFIX}_LIBRARY})
			if (NOT ${PREFIX}_FIND_QUIETLY)
				message(STATUS "Found ${PREFIX}")
			endif ()
		elseif (${PREFIX}_EXTRALIB_MISSING AND ${PREFIX}_FIND_REQUIRED)
			message(FATAL_ERROR "\nRequired extra library for ${PREFIX} not found! Install the full library (including dev packages) and try again.\nIf the library is already installed, set the missing variables (${PREFIX}_INCLUDE_DIR and ${PREFIX}_LIBRARY) manually in cmake (via 'CMakeCache.txt' file, `cmake -D command .` or `ccmake .` command).\n")
		else ()
			if (NOT ${PREFIX}_FIND_QUIETLY)
				message(STATUS "Could not locate ${PREFIX}")
			endif ()
			if (${PREFIX}_FIND_REQUIRED)
				message(FATAL_ERROR "\nRequired library ${PREFIX} not found! Install the library (including dev packages) and try again.\nIf the library is already installed, set the missing variables (${PREFIX}_INCLUDE_DIR and ${PREFIX}_LIBRARY) manually in cmake (via 'CMakeCache.txt' file, `cmake -D command .` or `ccmake .` command).\n")
			endif ()
		endif ()
		
		mark_as_advanced(${PREFIX}_INCLUDE_DIR ${PREFIX}_LIBRARY)
	endif ()
endmacro(findpkg_finish)


# Add addional library file with checkin search results
macro(findpkg_addlib PREFIX EXTRALIB)
	if (${PKG_NAME}_${EXTRALIB}_LIBRARY)
		set(${PKG_NAME}_LIBRARIES ${${PKG_NAME}_LIBRARIES} ${${PKG_NAME}_${EXTRALIB}_LIBRARY})
		mark_as_advanced(${PKG_NAME}_${EXTRALIB}_LIBRARY)
	else ()
		set(${PREFIX}_EXTRALIB_MISSING 1)
		if (NOT ${PREFIX}_FIND_QUIETLY)
			message(STATUS "Could not locate extra lib ${EXTRALIB} for ${PREFIX}")
		endif ()
	endif ()
endmacro(findpkg_addlib)

# Add addional library include dirs with checkin search results
macro(findpkg_addincdir PREFIX EXTRALIB)
	if (${PKG_NAME}_${EXTRALIB}_INCLUDE_DIR)
		set(${PKG_NAME}_INCLUDE_DIRS ${${PKG_NAME}_INCLUDE_DIRS} ${${PKG_NAME}_${EXTRALIB}_INCLUDE_DIR})
		mark_as_advanced(${PKG_NAME}_${EXTRALIB}_INCLUDE_DIR)
	else ()
		set(${PREFIX}_EXTRALIB_MISSING 1)
		if (NOT ${PREFIX}_FIND_QUIETLY)
			message(STATUS "Could not locate extra include dir ${EXTRALIB} for ${PREFIX}")
		endif ()
	endif ()
endmacro(findpkg_addincdir)

# this is only (modify) subset of orginal file ...

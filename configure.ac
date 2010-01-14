AC_PREREQ(2.61)
AC_INIT([theoravideoplugin],[1.1],[kreso@cateia.com])
AC_CONFIG_SRCDIR([src/OgreTheoraDataStream.cpp])
AM_INIT_AUTOMAKE([foreign])

AC_REVISION($Revision: 1.0 $)

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_LANG(C++)


# Check for Darwin.
AC_CANONICAL_HOST
AM_CONDITIONAL([DARWIN], [echo "$host_os" | grep '^darwin'])

# Add extra configure options.
#AC_ARG_ENABLE(debug, [  --enable-debug          enable debug], [
#	DEBUG_FLAGS=-DDEBUGLEVEL=5
#	AC_SUBST(DEBUG_FLAGS)
#],[
#	DEBUG_FLAGS=-DDEBUGLEVEL=0
#	AC_SUBST(DEBUG_FLAGS)
#])

# Find ogre
#PKG_CHECK_MODULES(OGRE, OGRE)
#AC_SUBST(OGRE_CFLAGS)
#AC_SUBST(OGRE_LIBS)
#AC_SUBST(OGRE_PLUGINDIR, `pkg-config --variable=plugindir OGRE`)

# Ok... couldn't get this to work, besides, I don't have Ogre system-wide here.
# So we'll instead check for OGRE and tell user to specify manually in cflags.
# *sigh*

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])
# Check for functions
#AC_FUNC_MALLOC
#AC_FUNC_REALLOC
# Check for functions directly
#AC_CHECK_FUNCS([memset])
#AC_CHECK_FUNCS([getenv])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_SIZE_T


# Check for libtheoraplayer
AC_CHECK_HEADERS([TheoraVideoManager.h], ,[AC_MSG_ERROR("*** libtheoraplayer headers not found.")])
#AC_CHECK_LIB(libtheoraplayer, addsomechecksymbolhere, ,[AC_MSG_ERROR("*** libtheoraplayer library not found or wrong version.")])

# Check for Ogre
AC_CHECK_HEADERS([OgrePrerequisites.h], ,[AC_MSG_ERROR("*** Ogre headers not found. You may want to specify something like this in CXXFLAGS: -I/usr/include/OGRE depending where you have Ogre headers. ")])
#AC_CHECK_LIB(ogre, , ,[AC_MSG_ERROR("*** Ogre library not found or wrong version.")])

# Output files
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	include/Makefile
])

AC_OUTPUT

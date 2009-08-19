AC_PREREQ(2.61)
AC_INIT([theoravideoplugin],[1.0],[kreso@cateia.com])
AC_CONFIG_SRCDIR([src/TheoraFrameQueue.cpp])
AM_INIT_AUTOMAKE([foreign])

AC_REVISION($Revision: 1.0 $)

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_LANG(C++)

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


# Check for ptypes' pasync
AC_CHECK_HEADERS([ptypes/pasync.h], ,[AC_MSG_ERROR("*** ptypes headers not found.")])
AC_CHECK_LIB(ptypes, __ptypes_version, ,[AC_MSG_ERROR("*** ptypes library not found or wrong version.")])

# Check for ogg
AC_CHECK_HEADERS([ogg/ogg.h], ,[AC_MSG_ERROR("*** Ogg headers not found.")])
AC_CHECK_LIB(ogg, ogg_sync_pageseek, ,[AC_MSG_ERROR("*** Ogg library not found or wrong version.")])

# Check for theora
AC_CHECK_HEADERS([theora/theoradec.h], ,[AC_MSG_ERROR("*** Theora headers not found.")])
AC_CHECK_LIB(theora, th_decode_free, ,[AC_MSG_ERROR("*** Theora library not found or wrong version.")])

# Check for vorbis
AC_CHECK_HEADERS([vorbis/codec.h], ,[AC_MSG_ERROR("*** Vorbis headers not found.")])
AC_CHECK_LIB(vorbis, vorbis_comment_init, ,[AC_MSG_ERROR("*** Vorbis library not found or wrong version.")])

# Check for Ogre
AC_CHECK_HEADERS([OgrePrerequisites.h], ,[AC_MSG_ERROR("*** Ogre headers not found. You may want to specify something like this in CPPFLAGS: -I/usr/include/OGRE depending where you have Ogre headers. ")])
#AC_CHECK_LIB(ogre, , ,[AC_MSG_ERROR("*** Ogre library not found or wrong version.")])

# Output files
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	include/Makefile
])

AC_OUTPUT

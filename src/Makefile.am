AM_CXXFLAGS=-I../include/

lib_LTLIBRARIES = Plugin_TheoraVideoSystem.la
Plugin_TheoraVideoSystem_la_SOURCES = \
	OgreTheoraDataStream.cpp OgreVideoDLLmain.cpp     OgreVideoManager.cpp

Plugin_TheoraVideoSystem_la_LDFLAGS=-module -avoid-version
#if DARWIN
#Plugin_TheoraVideoSystem_la_LDFLAGS += -shrext .bundle
#endif DARWIN



AM_CXXFLAGS=-I../include/

lib_LTLIBRARIES = Plugin_TheoraVideoSystem.la
Plugin_TheoraVideoSystem_la_SOURCES = \
	TheoraAudioInterface.cpp	TheoraTimer.cpp			TheoraVideoManager.cpp \
	TheoraFrameQueue.cpp		TheoraVideoClip.cpp		TheoraWorkerThread.cpp \
	TheoraMuteAudioInterface.cpp	TheoraVideoFrame.cpp		theoraDLLmain.cpp 
Plugin_TheoraVideoSystem_la_LDFLAGS=-module -avoid-version
if DARWIN
Plugin_TheoraVideoSystem_la_LDFLAGS += -shrext .bundle
endif DARWIN



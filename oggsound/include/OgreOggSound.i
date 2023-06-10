
%module(directors="1") OgreOggSound
%{
#include "Ogre.h"
#if !OGRE_MIN_VERSION(14, 0, 0)
#include "OgreUnifiedHighLevelGpuProgram.h"
#endif

#include "OgreOggSoundRoot.h"
#include "OgreOggSound.h"
%}

%include std_string.i
%include exception.i
%include stdint.i
%include typemaps.i
%import "Ogre.i"

#define _OGGSOUND_EXPORT

%ignore OgreOggSound::OgreOggISound::_setSharedProperties;
%ignore OgreOggSound::OgreOggISound::_getSharedProperties;

%ignore OgreOggSound::OgreOggSoundManager::createRecorder;
%ignore OgreOggSound::OgreOggSoundManager::getRecorder;

%rename(SoundManager) OgreOggSound::OgreOggSoundManager;

%include "OgreOggSoundRoot.h"
%include "OgreOggListener.h"
%include "OgreOggISound.h"
%include "OgreOggSoundManager.h"
#ifndef _SoundModule_Header_
#define _SoundModule_Header_
/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright © 2000-2003 The OGRE Team
Also see acknowledgements in Readme.html

You may use this sample code for anything you like, it is not covered by the
LGPL like the rest of the engine.
-----------------------------------------------------------------------------
*/

#include "TheoraAudioDriver.h"

namespace CEGUI
{
	class ProgressBar;
}

namespace Ogre
{
	class SoundManager
	{
	protected:
		SoundManager() {}
		virtual ~SoundManager() {}

		//Names of possible Sound Modules
		static const std::string openal;    //OAL_MOD
		static const std::string fmod;      //FMOD_MOD

		typedef SoundManager* (*DLL_GETSOUNDSYSTEM)(void);

		virtual void start() {}
		virtual void stop() {}
	public:
		static std::vector<std::string> loadSoundModules();
		static void unloadSoundModules();

		static SoundManager* startUpSoundManager(const std::string &snd);
		static void stopSoundManager(SoundManager* sndMgr);

		virtual TheoraAudioDriver* createAudio() = 0;
		virtual void destroyAudio( TheoraAudioDriver* audioClip ) = 0;	
	};
}
#endif // _SoundModule_Header_

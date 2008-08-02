#include "SoundManager.h"
#include "OgreDynLibManager.h"
#include "OgreDynLib.h"

using namespace Ogre;

//Names of possible Sound Modules
const std::string SoundManager::fmod = "FMOD_MOD";
const std::string SoundManager::openal = "OAL_MOD";

SoundManager* FmodMgr = 0;
SoundManager* OpenALMgr = 0;

DynLib* l_fmod = 0;
DynLib* l_al = 0;

//---------------------------------------------------------------------------//
std::vector<std::string> SoundManager::loadSoundModules()
{
	DynLibManager* dMgr = Ogre::DynLibManager::getSingletonPtr();
	DLL_GETSOUNDSYSTEM getSoundMgr;
	std::vector<std::string> list;

	//Fmod
	try
	{
		l_fmod = dMgr->load(fmod);
		getSoundMgr = (DLL_GETSOUNDSYSTEM)(l_fmod->getSymbol("getSoundSystem"));
		if( getSoundMgr )
		{
			FmodMgr = getSoundMgr();
			list.push_back(fmod);
		}
	}
	catch(...) {}
	//OpenAL
	try
	{
		l_al = dMgr->load(openal);
		getSoundMgr = (DLL_GETSOUNDSYSTEM)(l_al->getSymbol("getSoundSystem"));
		if( getSoundMgr )
		{
			OpenALMgr = getSoundMgr();
			list.push_back(openal);
		}
	}
	catch(...) {}
		
	return list;
}

//---------------------------------------------------------------------------//
void SoundManager::unloadSoundModules()
{
	//Destroy all the Managers
	delete OpenALMgr;
	delete FmodMgr;

	OpenALMgr = 0;
	FmodMgr = 0;

	//Unload
	DynLibManager* dMgr = Ogre::DynLibManager::getSingletonPtr();
	if(l_fmod) dMgr->unload(l_fmod);
	if(l_al)   dMgr->unload(l_al);

	l_fmod = 0;
	l_al = 0;
}

//---------------------------------------------------------------------------//
SoundManager* SoundManager::startUpSoundManager( const std::string &snd )
{
	SoundManager* mgr = 0;

	if( snd == openal && OpenALMgr )
		mgr = OpenALMgr;
	else if( snd == fmod && FmodMgr )
		mgr = FmodMgr;

	if( mgr )
		mgr->start();

	return mgr;
}

//---------------------------------------------------------------------------//
void SoundManager::stopSoundManager( SoundManager* sndMgr )
{
	sndMgr->stop();
}

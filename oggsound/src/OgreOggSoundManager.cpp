/**
* @author  Ian Stangoe 
*
* LICENSE:
*
* This source file is part of OgreOggSound, an OpenAL wrapper library for
* use with the Ogre Rendering Engine.
*
* Copyright (c) 2017 Ian Stangoe
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE. 
*
*/

#include "OgreOggSoundManager.h"
#include "OgreOggSound.h"

#include <string>

#ifndef AL_EFFECTSLOT_TARGET_SOFT
#define AL_EFFECTSLOT_TARGET_SOFT 0x199C
#endif

#if OGGSOUND_THREADED
	bool OgreOggSound::OgreOggSoundManager::mShuttingDown = false;
#endif

#if OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR <= 7    
	template<> OgreOggSound::OgreOggSoundManager* Ogre::Singleton<OgreOggSound::OgreOggSoundManager>::ms_Singleton = 0;
#else
	template<> OgreOggSound::OgreOggSoundManager* Ogre::Singleton<OgreOggSound::OgreOggSoundManager>::msSingleton = 0;
#endif

namespace OgreOggSound
{
	const Ogre::String OgreOggSoundManager::OGREOGGSOUND_VERSION_STRING = "OgreOggSound v1.29";

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggSoundManager::OgreOggSoundManager() :
		mOrigVolume(1.f)
		,mDevice(0)
		,mContext(0)
		,mListener(0)
#if HAVE_EFX
		,mEAXSupport(false)
		,mEFXSupport(false)
		,mXRamSupport(false)
		,mXRamSize(0)
		,mXRamFree(0)
		,mXRamAuto(0)
		,mXRamHardware(0)
		,mXRamAccessible(0)
		,mCurrentXRamMode(0)
		,mEAXVersion(0)
#endif
		,mRecorder(0)
		,mDeviceStrings(0)
		,mMaxSources(100)
		,mResourceGroupName("")
		,mGlobalPitch(1.f)
		,mSoundsToDestroy(0)
		,mFadeVolume(false)
		,mFadeIn(false)
		,mFadeTime(0.f)
		,mFadeTimer(0.f)
#if OGGSOUND_THREADED
		,mActionsList(0)
		,mForceMutex(false)
#endif
		{
#if HAVE_EFX
			// Effect objects
			alGenEffects = NULL;
			alDeleteEffects = NULL;
			alIsEffect = NULL;
			alEffecti = NULL;
			alEffectiv = NULL;
			alEffectf = NULL;
			alEffectfv = NULL;
			alGetEffecti = NULL;
			alGetEffectiv = NULL;
			alGetEffectf = NULL;
			alGetEffectfv = NULL;

			//Filter objects
			alGenFilters = NULL;
			alDeleteFilters = NULL;
			alIsFilter = NULL;
			alFilteri = NULL;
			alFilteriv = NULL;
			alFilterf = NULL;
			alFilterfv = NULL;
			alGetFilteri = NULL;
			alGetFilteriv = NULL;
			alGetFilterf = NULL;
			alGetFilterfv = NULL;

			// Auxiliary slot object
			alGenAuxiliaryEffectSlots = NULL;
			alDeleteAuxiliaryEffectSlots = NULL;
			alIsAuxiliaryEffectSlot = NULL;
			alAuxiliaryEffectSloti = NULL;
			alAuxiliaryEffectSlotiv = NULL;
			alAuxiliaryEffectSlotf = NULL;
			alAuxiliaryEffectSlotfv = NULL;
			alGetAuxiliaryEffectSloti = NULL;
			alGetAuxiliaryEffectSlotiv = NULL;
			alGetAuxiliaryEffectSlotf = NULL;
			alGetAuxiliaryEffectSlotfv = NULL;

			mNumEffectSlots = 0;
			mNumSendsPerSource = 0;
#endif
		}

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggSoundManager::~OgreOggSoundManager()
	{
#if OGGSOUND_THREADED
		mShuttingDown = true;
		if ( mUpdateThread )
		{
			mUpdateThread->join();
			OGRE_FREE(mUpdateThread, Ogre::MEMCATEGORY_GENERAL);
			mUpdateThread = 0;
			mShuttingDown=false;
		}
		if ( mActionsList )
		{
			if ( !mActionsList->empty() )
			{
				SoundAction obj;
				// Clear out action list
				while (mActionsList->pop(obj))
				{
					// If parameters specified delete structure
					if (obj.mParams)
					{
						switch ( obj.mAction )
						{			
						case LQ_LOAD:
							{
								OGRE_DELETE_T(static_cast<cSound*>(obj.mParams), cSound, Ogre::MEMCATEGORY_GENERAL);
							}
							break;	   
						case LQ_ATTACH_EFX:
						case LQ_DETACH_EFX:
						case LQ_SET_EFX_PROPERTY:
							{
								OGRE_DELETE_T(static_cast<efxProperty*>(obj.mParams), efxProperty, Ogre::MEMCATEGORY_GENERAL);
							}
							break;	 
						default:
							{
								OGRE_FREE(obj.mParams, Ogre::MEMCATEGORY_GENERAL);
							}
							break;
						}
					}
				}
			}
			delete mActionsList;
			mActionsList=0;
		}
#endif
		if ( mSoundsToDestroy )
		{
			delete mSoundsToDestroy;
			mSoundsToDestroy=0;
		}

		_releaseAll();

		if ( mRecorder ) { OGRE_DELETE_T(mRecorder, OgreOggSoundRecord, Ogre::MEMCATEGORY_GENERAL); mRecorder=0; }

		alcMakeContextCurrent(0);
		alcDestroyContext(mContext);
		mContext=0;
		alcCloseDevice(mDevice);
		mDevice=0;

		// trying to make sure the scene manager is still there
		if ( mListener && !Ogre::Root::getSingleton().getSceneManagers().empty())
		{
			Ogre::SceneManager* s = mListener->getSceneManager();
			s->destroyAllMovableObjectsByType("OgreOggISound");
		}
		_destroyListener();
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::init(	const Ogre::String &deviceName,
									unsigned int maxSources,
									unsigned int queueListSize,
									Ogre::SceneManager* scnMgr)
	{
		if (mDevice) return true;

		Ogre::LogManager::getSingleton().logMessage("*******************************************");
		Ogre::LogManager::getSingleton().logMessage("*** --- Starting " + OGREOGGSOUND_VERSION_STRING + " --- ***");
		Ogre::LogManager::getSingleton().logMessage("*******************************************");

		// Set source limit
		mMaxSources = maxSources;

		// Get an internal list of audio device strings
		_enumDevices();

		int majorVersion;
		int minorVersion;
#if OGRE_PLATFORM != OGRE_PLATFORM_WIN32
		ALCdevice* device = alcOpenDevice(NULL);
#endif
		// Version Info
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        ALenum error = 0;
		alcGetError(NULL);
	    alcGetIntegerv(NULL, ALC_MINOR_VERSION, sizeof(minorVersion), &minorVersion);
        if ((error = alcGetError(NULL))!=AL_NO_ERROR)
		{
			switch (error)
			{
			case AL_INVALID_NAME:		{ Ogre::LogManager::getSingleton().logError("Invalid Name when attempting: OpenAL Minor Version number"); } break;
			case AL_INVALID_ENUM:		{ Ogre::LogManager::getSingleton().logError("Invalid Enum when attempting: OpenAL Minor Version number"); } break;
			case AL_INVALID_VALUE:		{ Ogre::LogManager::getSingleton().logError("Invalid Value when attempting: OpenAL Minor Version number"); } break;
			case AL_INVALID_OPERATION:	{ Ogre::LogManager::getSingleton().logError("Invalid Operation when attempting: OpenAL Minor Version number"); } break;
			case AL_OUT_OF_MEMORY:		{ Ogre::LogManager::getSingleton().logError("Out of memory when attempting: OpenAL Minor Version number"); }	break;
			}
			Ogre::LogManager::getSingleton().logError("Unable to get OpenAL Minor Version number", Ogre::LML_CRITICAL);
			return false;
		}
		alcGetError(NULL);
		alcGetIntegerv(NULL, ALC_MAJOR_VERSION, sizeof(majorVersion), &majorVersion);
        if ((error = alcGetError(NULL))!=AL_NO_ERROR)
		{
			switch (error)
			{
			case AL_INVALID_NAME:		{ Ogre::LogManager::getSingleton().logError("Invalid Name when attempting: OpenAL Minor Version number"); } break;
			case AL_INVALID_ENUM:		{ Ogre::LogManager::getSingleton().logError("Invalid Enum when attempting: OpenAL Minor Version number"); } break;
			case AL_INVALID_VALUE:		{ Ogre::LogManager::getSingleton().logError("Invalid Value when attempting: OpenAL Minor Version number"); } break;
			case AL_INVALID_OPERATION:	{ Ogre::LogManager::getSingleton().logError("Invalid Operation when attempting: OpenAL Minor Version number"); } break;
			case AL_OUT_OF_MEMORY:		{ Ogre::LogManager::getSingleton().logError("Out of memory when attempting: OpenAL Minor Version number"); } break;
			}
			Ogre::LogManager::getSingleton().logError("Unable to get OpenAL Major Version number");
			return false;
		}
#else
        alcGetIntegerv(device, ALC_MINOR_VERSION, sizeof(minorVersion), &minorVersion);
        ALCenum error = alcGetError(device);
        if (error != ALC_NO_ERROR)
		{
			Ogre::LogManager::getSingleton().logError("Unable to get OpenAL Minor Version number");
			return false;
		}
		alcGetIntegerv(device, ALC_MAJOR_VERSION, sizeof(majorVersion), &majorVersion);
		error = alcGetError(device);
        if (error != ALC_NO_ERROR)
		{
			Ogre::LogManager::getSingleton().logError("Unable to get OpenAL Major Version number");
			return false;
		}
		alcCloseDevice(device);
#endif
		//Ogre::LogManager::getSingleton().logMessage(Ogre::String("OpenAL version " + Ogre::StringConverter::toString(majorVersion) + "." + Ogre::StringConverter::toString(minorVersion)));

		/*
		** OpenAL versions prior to 1.0 DO NOT support device enumeration, so we
		** need to test the current version and decide if we should try to find
		** an appropriate device or if we should just open the default device.
		*/
		bool deviceInList = false;
		if(majorVersion >= 1 && minorVersion >= 1)
		{
			Ogre::LogManager::getSingleton().logMessage("Playback devices available:");

			// List devices in log and see if the sugested device is in the list
			std::stringstream ss;

			for(auto& device : getDeviceList())
			{
				if(device == deviceName)
					deviceInList = true;

				ss << " * \"" << device << "\"";
				Ogre::LogManager::getSingleton().logMessage(ss.str());
				ss.clear(); ss.str("");
			}
		}

		// If the suggested device is in the list we use it, otherwise select the default device
		mDevice = alcOpenDevice(deviceInList ? deviceName.c_str() : NULL);
		if (!mDevice)
		{
			Ogre::LogManager::getSingletonPtr()->logError("OgreOggSoundManager::init() - Unable to open audio device");
			return false;
		}
		
		if (!deviceInList)
			Ogre::LogManager::getSingleton().logMessage("Choosing: \"" + Ogre::String(mDeviceStrings) + "\" (Default device)");
		else
			Ogre::LogManager::getSingleton().logMessage("Choosing: \"" + Ogre::String(deviceName) + "\"");

#if HAVE_EFX
        ALint attribs[2] = {ALC_MAX_AUXILIARY_SENDS, 4};
#else
        ALint attribs[1] = {4};
#endif

		mContext = alcCreateContext(mDevice, attribs);
		if (!mContext)
		{
			Ogre::LogManager::getSingletonPtr()->logError("OgreOggSoundManager::init() - Unable to create a context");
			return false;
		}

		if (!alcMakeContextCurrent(mContext))
		{
			Ogre::LogManager::getSingletonPtr()->logError("OgreOggSoundManager::init() - Unable to set context");
			return false;
		}

		Ogre::LogManager::getSingleton().logMessage("AL_VENDOR = " + Ogre::String(alGetString(AL_VENDOR)));
		Ogre::LogManager::getSingleton().logMessage("AL_RENDERER = " + Ogre::String(alGetString(AL_RENDERER)));
		Ogre::LogManager::getSingleton().logMessage("AL_VERSION = " + Ogre::String(alGetString(AL_VERSION)));

		_checkExtensionSupport();

		_checkFeatureSupport();

		// If no manager specified - grab first one 
		if ( !scnMgr )
		{
			auto& inst = Ogre::Root::getSingletonPtr()->getSceneManagers();

			if ( !inst.empty() )
				mSceneMgr = inst.begin()->second;
			else
			{
				OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, "No SceneManager's created - a valid SceneManager is required to create sounds", "OgreOggSoundManager::init()");
				return false;
			}
		}
		else
			mSceneMgr = scnMgr;

		if ( !createListener() ) 
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, "Unable to create a listener object", "OgreOggSoundManager::init()");
			return false;
		}

		_createSourcePool();

		Ogre::LogManager::getSingleton().logMessage(Ogre::String("Created " + Ogre::StringConverter::toString(getNumSources()) + " sources for simultaneous sounds"));

		mSoundsToDestroy = new LocklessQueue<OgreOggISound*>(100);
#if OGGSOUND_THREADED
		static_assert(OGRE_THREAD_SUPPORT, "Threading not supported by OGRE");
		if (queueListSize)
		{
			mActionsList = new LocklessQueue<SoundAction>(queueListSize);
		}
		OGRE_THREAD_CREATE(tmp, &OgreOggSoundManager::threadUpdate);
		mUpdateThread = tmp;
		Ogre::LogManager::getSingleton().logMessage("Using threads for streaming", Ogre::LML_NORMAL);
#endif

		// Recording
		if (alcIsExtensionPresent(mDevice, "ALC_EXT_CAPTURE") == AL_FALSE)
			Ogre::LogManager::getSingleton().logMessage("Recording devices NOT detected!", Ogre::LML_NORMAL);
		else
		{
			Ogre::LogManager::getSingleton().logMessage("Recording devices available:", Ogre::LML_NORMAL);

			for(auto recordDevice : getCaptureDeviceList())
				Ogre::LogManager::getSingleton().logMessage(" * '" + recordDevice + "'", Ogre::LML_NORMAL);
		}

		return true;
	}

	/*/////////////////////////////////////////////////////////////////*/
#if HAVE_ALEXT == 1
	void OgreOggSoundManager::pauseOpenALDevice()
	{
		if(mDevice)
		{
			if (alcIsExtensionPresent(mDevice, "ALC_SOFT_pause_device"))
			{
				// Get function pointer
				LPALCDEVICEPAUSESOFT alcDevicePauseSOFT = (LPALCDEVICEPAUSESOFT) alGetProcAddress("alcDevicePauseSOFT");

				alcDevicePauseSOFT(mDevice);
				if ( alGetError() != AL_NO_ERROR )
					Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::pauseOpenalDevice() - Unable to pause device!");
			}
			else
				Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::pauseOpenalDevice() - ALC_SOFT_pause_device extension not found!");
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::resumeOpenALDevice()
	{
		if(mDevice)
		{
			if (alcIsExtensionPresent(mDevice, "ALC_SOFT_pause_device"))
			{
				// Get function pointer
				LPALCDEVICERESUMESOFT alcDeviceResumeSOFT = (LPALCDEVICERESUMESOFT) alGetProcAddress("alcDeviceResumeSOFT");

				alcDeviceResumeSOFT(mDevice);
				if ( alGetError() != AL_NO_ERROR )
					Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::resumeOpenALDevice() - Unable to resume device!");
			}
			else
				Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::pauseOpenalDevice() - ALC_SOFT_pause_device extension not found!");
		}
	}
#endif

	/*/////////////////////////////////////////////////////////////////*/
	const Ogre::StringVector OgreOggSoundManager::getDeviceList() const
	{
		const ALCchar* deviceList = mDeviceStrings;

		Ogre::StringVector deviceVector;
		/*
		** The list returned by the call to alcGetString has the names of the
		** devices seperated by NULL characters and the list is terminated by
		** two NULL characters, so we can cast the list into a string and it
		** will automatically stop at the first NULL that it sees, then we
		** can move the pointer ahead by the lenght of that string + 1 and we
		** will be at the begining of the next string.  Once we hit an empty
		** string we know that we've found the double NULL that terminates the
		** list and we can stop there.
		*/
		while(*deviceList != 0)
		{
			deviceVector.push_back(deviceList);

			deviceList += strlen(deviceList) + 1;
		}

		return deviceVector;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::createListener() 
	{
		if ( mListener ) return true;

		// Create a listener
		return (mListener = _createListener()) != 0;
	}

	/*/////////////////////////////////////////////////////////////////*/
	const Ogre::StringVector OgreOggSoundManager::getSoundList() const
	{
#if OGGSOUND_THREADED
		OGRE_WQ_LOCK_MUTEX(mSoundMutex);
#endif

		Ogre::StringVector list;
		for ( SoundMap::const_iterator iter = mSoundMap.begin(); iter != mSoundMap.end(); ++iter )
			list.push_back((*iter).first);
		return list;
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::setMasterVolume(ALfloat vol)
	{
		if ( (vol>=0.f) && (vol<=1.f) )
			alListenerf(AL_GAIN, vol);
	}		 

	/*/////////////////////////////////////////////////////////////////*/
	ALfloat OgreOggSoundManager::getMasterVolume()
	{
		ALfloat vol=0.0;
		alGetListenerf(AL_GAIN, &vol);
		return vol;
	}

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggISound* OgreOggSoundManager::createSound(const Ogre::String& name,
													const Ogre::String& file,
													bool stream,
													bool loop,
													bool preBuffer,
													Ogre::SceneManager* scnMgr,
													bool immediate)
	{
		Ogre::NameValuePairList params;
		OgreOggISound* sound = 0;

		params["fileName"]	= file;
		params["stream"]	= stream	? "true" : "false";
		params["loop"]		= loop		? "true" : "false";
		params["preBuffer"]	= preBuffer ? "true" : "false";
		params["immediate"]	= immediate ? "true" : "false";
		params["name"]		= name;

		// Get first SceneManager if defined
		if ( !scnMgr )
		{
			if ( mSceneMgr )
				scnMgr = mSceneMgr;
			else
			{
				OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND, "No SceneManager defined!", "OgreOggSoundManager::createSound()");
			}
		}

		// Catch exception when plugin hasn't been registered
		try
		{
			params["sceneManagerName"] = scnMgr->getName();
			sound = static_cast<OgreOggISound*>(
			#if OGRE_VERSION_MAJOR == 2
				scnMgr->createMovableObject( OgreOggSoundFactory::FACTORY_TYPE_NAME, &(scnMgr->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC)), &params )
			#else
				scnMgr->createMovableObject( name, OgreOggSoundFactory::FACTORY_TYPE_NAME, &params )
			#endif
			);
		}
		catch (Ogre::Exception& e)
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::createSound() - There was an error when trying to create sound: " + name);

			// If the sound creation generated an exception, then erase the sound from the soundmap
			SoundMap::iterator i = mSoundMap.find(name);
			mSoundMap.erase(i);

			OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, e.getFullDescription(), "OgreOggSoundManager::createSound()");
		}

		// create Movable Sound
		return sound;
	}

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggISound* OgreOggSoundManager::_createSoundImpl(	Ogre::SceneManager* scnMgr,
															const Ogre::String& name,
															#if OGRE_VERSION_MAJOR == 2
															Ogre::IdType id,
															#endif
															const Ogre::String& file,
															bool stream,
															bool loop,
															bool preBuffer,
															bool immediate)
	{
		OgreOggISound* sound = 0;

		// MUST be unique
		if ( hasSound(name) )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::createSound() - Sound with name: " + name + " already exists!");
			return 0;
		}

		if ( file == "BUFFER" )
		{
			sound = OGRE_NEW_T(OgreOggStreamBufferSound, Ogre::MEMCATEGORY_GENERAL)(
				name, scnMgr
				#if OGRE_VERSION_MAJOR == 2
				, Ogre::Id::generateNewId<Ogre::MovableObject>(), &(scnMgr->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC)), 0
				#endif
			);

			// Here we lock the sound mutex directly instead of using a scoped lock and immediately unlock it after modifying the
			// sound map. The reason for this is that if a scoped lock is used it is possible that update thread will attempt to
			// lock it before _requestSoundAction() is called which can cause the application to hang forever.
			#if OGGSOUND_THREADED
				mSoundMutex.lock();
			#endif

			// Add to list
			mSoundMap[name] = sound;

			#if OGGSOUND_THREADED
				mSoundMutex.unlock();
			#endif

			return sound;
		}
		else if	( file.find(".ogg") != file.npos || file.find(".OGG") != file.npos )
		{
			if(stream)
				sound = OGRE_NEW_T(OgreOggStreamSound, Ogre::MEMCATEGORY_GENERAL)(
					name, scnMgr
					#if OGRE_VERSION_MAJOR == 2
					, Ogre::Id::generateNewId<Ogre::MovableObject>(), &(scnMgr->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC)), 0
					#endif
				);
			else
				sound = OGRE_NEW_T(OgreOggStaticSound, Ogre::MEMCATEGORY_GENERAL)(
					name, scnMgr
					#if OGRE_VERSION_MAJOR == 2
					, Ogre::Id::generateNewId<Ogre::MovableObject>(), &(scnMgr->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC)), 0
					#endif
				);

			// Set loop flag
			sound->loop(loop);

			// Here we lock the sound mutex directly instead of using a scoped lock and immediately unlock it after modifying the
			// sound map. The reason for this is that if a scoped lock is used it is possible that update thread will attempt to
			// lock it before _requestSoundAction() is called which can cause the application to hang forever.
			#if OGGSOUND_THREADED
				mSoundMutex.lock();
			#endif

			// Add to list
			mSoundMap[name] = sound;

			#if OGGSOUND_THREADED
				mSoundMutex.unlock();
			#endif

#if OGGSOUND_THREADED
			SoundAction action;
			cSound* c		= OGRE_NEW_T(cSound, Ogre::MEMCATEGORY_GENERAL);
			c->mFileName	= file;
			c->mPrebuffer	= preBuffer;
			action.mAction	= LQ_LOAD;
			action.mParams	= c;
			action.mImmediately = immediate;
			action.mSound	= sound->getName();
			_requestSoundAction(action);
#else
			// load audio data
			_loadSoundImpl(sound, file, preBuffer);
#endif
			return sound;
		}
		else if	( file.find(".wav") != file.npos || file.find(".WAV") != file.npos )
		{
			if(stream)
				sound = OGRE_NEW_T(OgreOggStreamWavSound, Ogre::MEMCATEGORY_GENERAL)(
					name, scnMgr
					#if OGRE_VERSION_MAJOR == 2
					, Ogre::Id::generateNewId<Ogre::MovableObject>(), &(scnMgr->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC)), 0
					#endif
				);
			else
				sound = OGRE_NEW_T(OgreOggStaticWavSound, Ogre::MEMCATEGORY_GENERAL)(
					name, scnMgr
					#if OGRE_VERSION_MAJOR == 2
					, Ogre::Id::generateNewId<Ogre::MovableObject>(), &(scnMgr->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC)), 0
					#endif
				);

			// Set loop flag
			sound->loop(loop);

			// Here we lock the sound mutex directly instead of using a scoped lock and immediately unlock it after modifying the
			// sound map. The reason for this is that if a scoped lock is used it is possible that update thread will attempt to
			// lock it before _requestSoundAction() is called which can cause the application to hang forever.
			#if OGGSOUND_THREADED
				mSoundMutex.lock();
			#endif

			// Add to list
			mSoundMap[name] = sound;

			#if OGGSOUND_THREADED
				mSoundMutex.unlock();
			#endif

#if OGGSOUND_THREADED
			SoundAction action;
			cSound* c		= OGRE_NEW_T(cSound, Ogre::MEMCATEGORY_GENERAL);
			c->mFileName	= file;
			c->mPrebuffer	= preBuffer; 
			action.mAction	= LQ_LOAD;
			action.mParams	= c;
			action.mImmediately = immediate;
			action.mSound	= sound->getName();
			_requestSoundAction(action);
#else
			// Load audio file
			_loadSoundImpl(sound, file, preBuffer);
#endif
			return sound;
		}
		else
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::createSound() - Sound does not have (.ogg | .wav) extension: " + file);
			return 0;
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggListener* OgreOggSoundManager::_createListener()
	{
		#if OGRE_VERSION_MAJOR == 2
		OgreOggListener* l = OGRE_NEW_T(OgreOggListener, Ogre::MEMCATEGORY_GENERAL)(Ogre::Id::generateNewId<Ogre::MovableObject>(), mSceneMgr, &(mSceneMgr->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC)), 0 );
		#else
		OgreOggListener* l = OGRE_NEW_T(OgreOggListener, Ogre::MEMCATEGORY_GENERAL)(mSceneMgr);
		#endif
		return l;
	}

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggISound* OgreOggSoundManager::getSound(const Ogre::String& name)
	{
#if OGGSOUND_THREADED
		OGRE_WQ_LOCK_MUTEX(mSoundMutex);
#endif

		SoundMap::iterator i = mSoundMap.find(name);
		if(i == mSoundMap.end()) return 0;
#if OGGSOUND_THREADED
		if ( !i->second->_isDestroying() )
			return i->second;
		else
			return 0;
#else
		return i->second;
#endif
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::hasSound(const Ogre::String& name)
	{
#if OGGSOUND_THREADED
		OGRE_WQ_LOCK_MUTEX(mSoundMutex);
#endif

		SoundMap::iterator i = mSoundMap.find(name);
		if(i == mSoundMap.end())
			return false;
#if OGGSOUND_THREADED
		if ( !i->second->_isDestroying() )
			return true;
		else
			return false;
#else
		return true;
#endif
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::destroyAllSounds()
	{
		_destroyAllSoundsImpl();
	}


	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::stopAllSounds()
	{
#if OGGSOUND_THREADED 
		SoundAction action;
		action.mAction	= LQ_STOP_ALL;
		action.mImmediately = false;
		action.mParams	= 0;
		action.mSound	= "";
		_requestSoundAction(action);
#else
		_stopAllSoundsImpl();
#endif
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::setGlobalPitch(float pitch)
	{
		if ( pitch<=0.f ) return;

		mGlobalPitch=pitch;
#if OGGSOUND_THREADED 
		SoundAction action;
		action.mAction	= LQ_GLOBAL_PITCH;
		action.mParams	= 0;
		action.mImmediately = false;
		action.mSound	= "";
		_requestSoundAction(action);
#else
		_setGlobalPitchImpl();
#endif
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::pauseAllSounds()
	{
#if OGGSOUND_THREADED
		SoundAction action;
		action.mAction = LQ_PAUSE_ALL;
		action.mSound = "";
		action.mParams = 0;
		action.mImmediately = false;
		_requestSoundAction(action);
#else
		_pauseAllSoundsImpl();
#endif
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::fadeMasterVolume(float time, bool fadeIn)
	{
		mFadeVolume = true;
		mFadeTime = time;
		mFadeIn = fadeIn;
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::resumeAllPausedSounds()
	{
#if OGGSOUND_THREADED
		SoundAction action;
		action.mAction = LQ_RESUME_ALL;
		action.mSound = "";
		action.mParams = 0;
		action.mImmediately = false;
		_requestSoundAction(action);
#else
		_resumeAllPausedSoundsImpl();
#endif
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::destroySound(const Ogre::String& sName)
	{
		OgreOggISound* sound = 0;

		if ( !(sound = getSound(sName)) )
			return;

		_destroySoundImpl(sound);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::destroySound(OgreOggISound* sound)
	{
		if ( !sound ) return;

		_destroySoundImpl(sound);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::setDistanceModel(ALenum value)
	{
		alDistanceModel(value);
	}

	/*/////////////////////////////////////////////////////////////////*/
	const ALenum OgreOggSoundManager::getDistanceModel() const
	{
		return alGetInteger(AL_DISTANCE_MODEL);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::setSpeedOfSound(float speed)
	{
		alSpeedOfSound(speed);
	}

	/*/////////////////////////////////////////////////////////////////*/
	float OgreOggSoundManager::getSpeedOfSound()
	{
		return alGetFloat(AL_SPEED_OF_SOUND);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::setDopplerFactor(float factor)
	{
		alDopplerFactor(factor);
	}

	/*/////////////////////////////////////////////////////////////////*/
	float OgreOggSoundManager::getDopplerFactor()
	{
		return alGetFloat(AL_DOPPLER_FACTOR);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::update(float fTime)
	{
#if OGGSOUND_THREADED == 0
		static float rTime = 0.f;
	
		if ( !mActiveSounds.empty() )
		{
			// Update ALL active sounds
			ActiveList::const_iterator i=mActiveSounds.begin(); 
			ActiveList::const_iterator end(mActiveSounds.end()); 
			while ( i != end )
			{
				(*i)->update(fTime);
				(*i)->_updateAudioBuffers();
				// Update recorder
				if ( mRecorder ) mRecorder->_updateRecording();
				++i;
			}
		}

		// Update listener
		mListener->update();

		// Limit re-activation
		if ( (rTime += fTime) > 0.05 )
		{
			// try to reactivate any
			_reactivateQueuedSounds();

			// Reset timer
			rTime = 0.f;
		}

#endif
		// Fade volume
		if ( mFadeVolume )
		{
			mFadeTimer+=fTime;

			if ( mFadeTimer > mFadeTime )
			{
				mFadeVolume = false;
				setMasterVolume(mFadeIn ? 1.f : 0.f);
			}
			else
			{
				ALfloat vol = 1.f;
				if ( mFadeIn )
					vol = (mFadeTimer/mFadeTime);
				else
					vol = 1.f - (mFadeTimer/mFadeTime);

				setMasterVolume(vol);
			}
		}

		// Destroy sounds
		if ( mSoundsToDestroy )
		{
			if (!mSoundsToDestroy->empty() )
			{
				OgreOggISound* s = 0;
				int count = 0;
				do
				{
					if ( mSoundsToDestroy->pop(s) )
					{
						_destroySoundImpl(s);
						count++;
					}
				}
				while(!mSoundsToDestroy->empty() && count<5);
			}
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::setResourceGroupName(const Ogre::String& group)
	{
#if OGGSOUND_THREADED
		OGRE_WQ_LOCK_MUTEX(mResourceGroupNameMutex);
#endif

		mResourceGroupName = group;
	}

	/*/////////////////////////////////////////////////////////////////*/
	Ogre::String OgreOggSoundManager::getResourceGroupName() const
	{
#if OGGSOUND_THREADED
		OGRE_WQ_LOCK_MUTEX(mResourceGroupNameMutex);
#endif

		return mResourceGroupName;
	}

	/*/////////////////////////////////////////////////////////////////*/
	struct OgreOggSoundManager::_sortNearToFar
	{
		_sortNearToFar(const Ogre::Vector3 & listenerPos) : mListenerPos(listenerPos) { }

		bool operator()(OgreOggISound*& sound1, OgreOggISound*& sound2)
		{
			if ( !sound1->isMono() ) return false;

			// Check sort order
			if ( !sound1->isMono() && sound2->isMono() ) return false;

			const Ogre::Real d1 = OgreOggSoundManager::_calculateDistanceToListener(sound1, mListenerPos);
			const Ogre::Real d2 = OgreOggSoundManager::_calculateDistanceToListener(sound2, mListenerPos);

			if ( d1<d2 )	return true;
			if ( d1>d2 )	return false;

			// Equal - don't sort
			return false;
		}

		private:
			Ogre::Vector3 mListenerPos;
	};

	/*/////////////////////////////////////////////////////////////////*/
	struct OgreOggSoundManager::_sortFarToNear
	{
		_sortFarToNear(const Ogre::Vector3 & listenerPos) : mListenerPos(listenerPos) { }

		bool operator()(OgreOggISound*& sound1, OgreOggISound*& sound2)
		{
			if ( !sound1->isMono() ) return false;

			// Check sort order
			if ( !sound1->isMono() && sound2->isMono() ) return true;

			const Ogre::Real d1 = OgreOggSoundManager::_calculateDistanceToListener(sound1, mListenerPos);
			const Ogre::Real d2 = OgreOggSoundManager::_calculateDistanceToListener(sound2, mListenerPos);

			if ( d1>d2 )	return true;
			if ( d1<d2 )	return false;

			// Equal - don't sort
			return false;
		}

		private:

			Ogre::Vector3 mListenerPos;
	};

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::_requestSoundSource(OgreOggISound* sound)
	{
		// Does sound need a source?
		if (!sound) return false;

		if (sound->getSource()!=AL_NONE) return true;
		
		ALuint src = AL_NONE;

		// If there are still sources available
		// Pop next available off list
		if ( !mSourcePool.empty() )
		{
			// Get next available source
			src = static_cast<ALuint>(mSourcePool.back());
			// Remove from available list
			mSourcePool.pop_back();
			// Set sounds source
			sound->setSource(src);
			// Remove from reactivate list if reactivating..
			if ( !mSoundsToReactivate.empty() )
			{
				ActiveList::iterator rIter=mSoundsToReactivate.begin(); 
				while ( rIter!=mSoundsToReactivate.end() )
				{
					if ( (*rIter)==sound )
						rIter = mSoundsToReactivate.erase(rIter);
					else
						++rIter;
				}
			}
			// Add new sound to active list
			mActiveSounds.push_back(sound);
			return true;
		}
		// All sources in use
		// Re-use an active source
		// Use either a non-playing source or a lower priority source
		else
		{
			// Get iterator for list
			ActiveList::iterator iter = mActiveSounds.begin();

			// Search for a stopped sound
			while ( iter!=mActiveSounds.end() )
			{
				// Find a stopped sound - reuse its source
				if ( (*iter)->isStopped() )
				{
					ALuint src = (*iter)->getSource();
					ALuint nullSrc = AL_NONE;
					// Remove source
					(*iter)->setSource(nullSrc);
					// Attach source to new sound
					sound->setSource(src);
					// Add new sound to active list
					mActiveSounds.erase(iter);
					// Add new sound to active list
					mActiveSounds.push_back(sound);
					// Return success
					return true;
				}
				else
					++iter;
			}

			// Check priority...
			Ogre::uint8 priority = sound->getPriority();
			iter = mActiveSounds.begin();

			// Search for a lower priority sound
			while ( iter!=mActiveSounds.end() )
			{
				// Find a stopped sound - reuse its source
				if ( (*iter)->getPriority()<sound->getPriority() )
				{
					ALuint src = (*iter)->getSource();
					ALuint nullSrc = AL_NONE;

					if ((*iter)->getState() != SS_DESTROYED)
					{
						// Pause sounds
						(*iter)->pause();
					}

					// Remove source
					(*iter)->setSource(nullSrc);
					// Attach source to new sound
					sound->setSource(src);

					// Add to reactivate list
					if ((*iter)->getState() != SS_DESTROYED)
					{
						mSoundsToReactivate.push_back((*iter));
					}

					// Remove relinquished sound from active list
					mActiveSounds.erase(iter);
					// Add new sound to active list
					mActiveSounds.push_back(sound);
					// Return success
					return true;
				}
				else
					++iter;
			}

			if (mListener)
			{
				const Ogre::Vector3 listenerPos(mListener->getPosition());

				// Sort list by distance
				mActiveSounds.sort(_sortFarToNear(listenerPos));

				// Lists should be sorted:	Active-->furthest to Nearest
				//							Reactivate-->Nearest to furthest
				OgreOggISound* snd1 = mActiveSounds.front();

				// Sort by distance
				const Ogre::Real d1 = _calculateDistanceToListener(snd1, listenerPos);
				const Ogre::Real d2 = _calculateDistanceToListener(sound, listenerPos);

				// Needs swapping?
				if ( d1>d2 )
				{
					ALuint src = snd1->getSource();
					ALuint nullSrc = AL_NONE;

					if (snd1->getState() != SS_DESTROYED)
					{
						// Pause sounds
						snd1->pause();
						snd1->_markPlayPosition();
					}

					// Remove source
					snd1->setSource(nullSrc);
					// Attach source to new sound
					sound->setSource(src);
					sound->_recoverPlayPosition();

					// Add to reactivate list
					if (snd1->getState() != SS_DESTROYED)
					{
						mSoundsToReactivate.push_back(snd1);
					}

					// Remove relinquished sound from active list
					mActiveSounds.erase(mActiveSounds.begin());					 
					// Add new sound to active list
					mActiveSounds.push_back(sound);
					// Return success
					return true;
				}
			}
		}

		// If no opportunity to grab a source add to queue
		if ( !mWaitingSounds.empty() )
		{
			// Check not already in list
			for ( ActiveList::iterator iter=mWaitingSounds.begin(); iter!=mWaitingSounds.end(); ++iter )
				if ( (*iter)==sound )
					return false;
		}

		// Add to list
		mWaitingSounds.push_back(sound);

		// Uh oh - won't be played
		return false;
	}	  

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::_releaseSoundSource(OgreOggISound* sound)
	{
		if (!sound) return false;

		if (sound->getSource() == AL_NONE) return true;

		// Get source
		ALuint src = sound->getSource();

		// Valid source?
		if(src != AL_NONE)
		{
			ALuint source = AL_NONE;

			// Detach source from sound
			sound->setSource(source);

			// Make source available
			mSourcePool.push_back(src);

			// Remove from actives list
			ActiveList::iterator iter = mActiveSounds.begin();
			while ( iter != mActiveSounds.end() )
			{
				// Find sound in actives list
				if ( (*iter) == sound )
					iter = mActiveSounds.erase(iter);
				else
					++iter;
			}
			return true;
		}

		return false;
	}	 	

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggSoundRecord* OgreOggSoundManager::createRecorder()
	{
		if ( mDevice )
		{
			mRecorder = OGRE_NEW_T(OgreOggSoundRecord, Ogre::MEMCATEGORY_GENERAL)(*mDevice);
			return mRecorder;
		}
		else
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::createRecorder() - Cowardly refusing to create a Recorder without a recording device!");
			return 0;
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	const RecordDeviceList& OgreOggSoundManager::getCaptureDeviceList()
	{
		mRecordDeviceList.clear();
		// Get list of available Capture Devices
		const ALchar *pDeviceList = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
		if (pDeviceList)
		{
			while (*pDeviceList)
			{
				mRecordDeviceList.push_back(Ogre::String(pDeviceList));
				pDeviceList += strlen(pDeviceList) + 1;
			}
		}
		return mRecordDeviceList;
	}

#if HAVE_EFX
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::isEffectSupported(ALint effectID)
	{
		if ( mEFXSupportList.find(effectID) != mEFXSupportList.end() )
			return mEFXSupportList[effectID];
		else
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::isEffectSupported() - Invalid effectID!");

		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
#	if HAVE_EFX == 1
	void OgreOggSoundManager::setXRamBuffer(ALsizei numBuffers, ALuint* buffer)
	{
		if ( buffer && mEAXSetBufferMode )
			mEAXSetBufferMode(numBuffers, buffer, mCurrentXRamMode);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::setXRamBufferMode(ALenum mode)
	{
		mCurrentXRamMode = mXRamAuto;
		if		( mode==mXRamAuto ) 	mCurrentXRamMode = mXRamAuto;
		else if ( mode==mXRamHardware ) mCurrentXRamMode = mXRamHardware;
		else if ( mode==mXRamAccessible ) mCurrentXRamMode = mXRamAccessible;
	}
#	endif

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::setEFXEffectParameter(const Ogre::String& eName, ALint effectType, ALenum attrib, ALfloat param)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::setEFXEffectParameter() - No EFX support!");
			return false;
		}

		if ( eName.empty() )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::setEFXEffectParameter() - Empty effect name!");
			return false;
		}

		ALuint effect;

		// Get effect id's
		if ( (effect = _getEFXEffect(eName) ) != AL_EFFECT_NULL )
		{
			alGetError();
			alEffecti(effectType, attrib, static_cast<ALint>(param));
			if ( alGetError()!=AL_NO_ERROR )
			{
				Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::setEFXEffectParameter() - Unable to change effect parameter!");
				return false;
			}
		}

		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::setEFXEffectParameter(const Ogre::String& eName, ALint effectType, ALenum attrib, ALfloat* params)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::setEFXEffectParameter() - No EFX support!");
			return false;
		}

		if ( eName.empty() )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::setEFXEffectParameter() - Empty effect name!");
			return false;
		}

		if ( params == nullptr )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::setEFXEffectParameter() - NULL pointer params!");
			return false;
		}

		ALuint effect;

		// Get effect id's
		if ( (effect = _getEFXEffect(eName) ) != AL_EFFECT_NULL )
		{
			alGetError();

			alEffectfv(effectType, attrib, params);

			if ( alGetError()!=AL_NO_ERROR )
			{
				Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::setEFXEffectParameter() - Unable to change effect parameters!");
				return false;
			}
		}

		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::setEFXEffectParameter(const Ogre::String& eName, ALint effectType, ALenum attrib, ALint param)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::setEFXEffectParameter() - No EFX support!");
			return false;
		}

		if ( eName.empty() )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::setEFXEffectParameter() - Empty effect name!");
			return false;
		}

		ALuint effect;

		// Get effect id's
		if ( (effect = _getEFXEffect(eName) ) != AL_EFFECT_NULL )
		{
			alGetError();

			alEffecti(effectType, attrib, param);

			if ( alGetError()!=AL_NO_ERROR )
			{
				Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::setEFXEffectParameter() - Unable to change effect parameter!");
				return false;
			}
		}

		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::setEFXEffectParameter(const Ogre::String& eName, ALint effectType, ALenum attrib, ALint* params)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::setEFXEffectParameter() - No EFX support!");
			return false;
		}

		if ( eName.empty() )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::setEFXEffectParameter() - Empty effect name!");
			return false;
		}

		if ( !params )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::setEFXEffectParameter() - NULL pointer params!");
			return false;
		}

		ALuint effect;

		// Get effect id's
		if ( (effect = _getEFXEffect(eName) ) != AL_EFFECT_NULL )
		{
			alGetError();

			alEffectiv(effectType, attrib, params);

			if ( alGetError()!=AL_NO_ERROR )
			{
				Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::setEFXEffectParameter() - Unable to change effect parameters!");
				return false;
			}
		}

		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::attachEffectToSound(const Ogre::String& sName, ALuint send, ALuint slotID, const Ogre::String& filterName)
	{
		OgreOggISound* sound=0;

		if ( !(sound = getSound(sName)) )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::attachEffectToSound() - Unable to find sound: " + sName);
			return false;
		}

#if OGGSOUND_THREADED
		SoundAction action;
		efxProperty* e	= OGRE_NEW_T(efxProperty, Ogre::MEMCATEGORY_GENERAL);
		e->mSend		= send;
		e->mSlotID		= slotID;
		e->mFilterName	= filterName;
		action.mAction	= LQ_ATTACH_EFX;
		action.mParams	= e;
		action.mSound	= sound->getName();
		action.mImmediately = false;
		_requestSoundAction(action);
		return true;
#else
		// load audio data
		return _attachEffectToSoundImpl(sound, send, slotID, filterName);
#endif
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::_attachEffectToSoundImpl(OgreOggISound* sound, ALuint send, ALuint slotID, const Ogre::String& filterName)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_attachEffectToSoundImpl() - No EFX support!");
			return false;
		}

		if ( !sound )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::_attachEffectToSoundImpl() - NULL pointer sound!");
			return false;
		}

		if ( send > getMaxAuxiliaryEffectSends() - 1 )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_attachEffectToSoundImpl() - Invalid send number!");
			return false;
		}

		if ( slotID > getNumberOfCreatedEffectSlots() - 1 )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_attachEffectToSoundImpl() - Invalid slotID!");
			return false;
		}

		ALuint slot;
		ALuint filter;

		// Get effect id's
		slot	= _getEFXSlot(slotID);
		filter	= _getEFXFilter(filterName);

		if ( slot == AL_NONE )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_attachEffectToSoundImpl() - Unable to retrieve slotID!");
			return false;
		}

		ALuint src = sound->getSource();

		if ( src != AL_NONE )
		{
			alGetError();

			alSource3i(src, AL_AUXILIARY_SEND_FILTER, slot, send, filter);

			if (alGetError() == AL_NO_ERROR)
			{
				return true;
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_attachEffectToSoundImpl() - Unable to attach effect to source!");
				return false;
			}
		}
		else
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_attachEffectToSoundImpl() - Sound has no source!");
			return false;
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::attachFilterToSound(const Ogre::String& sName, const Ogre::String& filterName)
	{
		OgreOggISound* sound=0;

		if ( !(sound = getSound(sName)) ) return false;

#if OGGSOUND_THREADED
		SoundAction action;
		efxProperty* e	= OGRE_NEW_T(efxProperty, Ogre::MEMCATEGORY_GENERAL);
		e->mEffectName	= "";
		e->mFilterName	= filterName;
		e->mSlotID		= 255;
		action.mAction	= LQ_ATTACH_EFX;
		action.mParams	= e;
		action.mSound	= sound->getName();
		action.mImmediately = false;
		_requestSoundAction(action);
		return true;
#else
		// load audio data
		return _attachFilterToSoundImpl(sound, filterName);
#endif
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::_attachFilterToSoundImpl(OgreOggISound* sound, const Ogre::String& filterName)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_attachFilterToSoundImpl() - No EFX support!");
			return false;
		}

		if ( !sound )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::_attachFilterToSoundImpl() - NULL pointer sound!");
			return false;
		}

		ALuint filter = _getEFXFilter(filterName);

		if ( filter != AL_FILTER_NULL )
		{
			ALuint src = sound->getSource();

			if ( src != AL_NONE )
			{
				alGetError();

				alSourcei(src, AL_DIRECT_FILTER, filter);

				if (alGetError() == AL_NO_ERROR)
				{
					return true;
				}
				else
				{
					Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_attachFilterToSoundImpl() - Unable to attach filter to source!");
					return false;
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_attachFilterToSoundImpl() - Sound has no source!");
				return false;
			}
		}
		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::detachEffectFromSound(const Ogre::String& sName, ALuint send)
	{
		OgreOggISound* sound=0;
		if ( !(sound = getSound(sName)) ) return false;

#if OGGSOUND_THREADED
		SoundAction action;
		efxProperty* e	= OGRE_NEW_T(efxProperty, Ogre::MEMCATEGORY_GENERAL);
		e->mEffectName	= "";
		e->mFilterName	= "";
		e->mSend		= send;
		e->mAirAbsorption = 0.f;
		e->mRolloff		= 0.f;
		e->mConeHF		= 0.f;
		action.mAction	= LQ_DETACH_EFX;
		action.mParams	= e;
		action.mSound	= sound->getName();
		action.mImmediately = false;
		_requestSoundAction(action);
		return true;
#else
		// load audio data
		return _detachEffectFromSoundImpl(sound, send);
#endif
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::_detachEffectFromSoundImpl(OgreOggISound* sound, ALuint send)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_detachEffectFromSoundImpl() - No EFX support!");
			return false;
		}

		if ( !sound )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::_detachEffectFromSoundImpl() - NULL pointer sound!");
			return false;
		}

		if ( send > getMaxAuxiliaryEffectSends() - 1 )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_detachEffectFromSoundImpl() - Invalid send number!");
			return false;
		}

		ALuint src = sound->getSource();

		if ( src != AL_NONE )
		{
			alGetError();

			alSource3i(src, AL_AUXILIARY_SEND_FILTER, AL_EFFECT_NULL, send, AL_FILTER_NULL);

			if (alGetError() == AL_NO_ERROR)
			{
				return true;
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_detachEffectFromSoundImpl() - Unable to detach effect from source!");
				return false;
			}
		}
		else
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_detachEffectFromSoundImpl() - Sound has no source!");
			return false;
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::detachFilterFromSound(const Ogre::String& sName)
	{
		OgreOggISound* sound=0;

		if ( !(sound = getSound(sName)) ) return false;

#if OGGSOUND_THREADED
		SoundAction action;
		efxProperty* e	= OGRE_NEW_T(efxProperty, Ogre::MEMCATEGORY_GENERAL);
		e->mEffectName	= "";
		e->mFilterName	= "";
		e->mSlotID		= 255;
		e->mAirAbsorption= 0.f;
		e->mRolloff		= 0.f;
		e->mConeHF		= 0.f;
		action.mAction	= LQ_DETACH_EFX;
		action.mParams	= e;
		action.mSound	= sound->getName();
		action.mImmediately = false;
		_requestSoundAction(action);
		return true;
#else
		// load audio data
		return _detachFilterFromSoundImpl(sound);
#endif
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::_detachFilterFromSoundImpl(OgreOggISound* sound)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_detachFilterFromSoundImpl() - No EFX support!");
			return false;
		}

		if ( !sound )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::_detachFilterFromSoundImpl() - NULL pointer sound!");
			return false;
		}

		ALuint src = sound->getSource();
		if ( src != AL_NONE )
		{
			alSourcei(src, AL_DIRECT_FILTER, AL_FILTER_NULL);
			if (alGetError() != AL_NO_ERROR)
			{
				Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_detachFilterFromSoundImpl() - Unable to detach filter from source!");
				return false;
			}
		}
		else
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_detachFilterFromSoundImpl() - Sound has no source!");
			return false;
		}

		return true;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::createEFXFilter(const Ogre::String& fName, ALint filterType, ALfloat gain, ALfloat hfGain, ALfloat lfGain)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::createEFXFilter() - No EFX support!");
			return false;
		}

		if ( fName.empty() )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::createEFXFilter() - Empty filter name!");
			return false;
		}

		if ( !isEffectSupported(filterType) )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::createEFXFilter() - Unsupported filter!");
			return false;
		}

		if ( mFilterList.find(fName) != mFilterList.end() )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::createEFXFilter() - Filter with name \"" + fName + "\" already exists!");
			return false;
		}

		ALuint filter;

		alGenFilters(1, &filter);
		if (alGetError() != AL_NO_ERROR)
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::createEFXFilter() - Cannot create EFX Filter!");
			return false;
		}

		if (alIsFilter(filter))
		{
			alGetError();

			alFilteri(filter, AL_FILTER_TYPE, filterType);

			if (alGetError() != AL_NO_ERROR)
			{
				Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::createEFXFilter() - Unable to set filter type!");
				// Destroy invalid filter we just created
				alDeleteFilters( 1, &filter);
				return false;
			}
			else
			{
				switch(filterType)
				{
					case AL_FILTER_LOWPASS:
						// Set lowpass filter properties
						alFilterf(filter, AL_LOWPASS_GAIN, gain);
						alFilterf(filter, AL_LOWPASS_GAINHF, hfGain);
						mFilterList[fName]=filter;
					break;
					case AL_FILTER_HIGHPASS:
						// Set highpass filter properties
						alFilterf(filter, AL_LOWPASS_GAIN, gain);
						alFilterf(filter, AL_HIGHPASS_GAINLF, lfGain);
						mFilterList[fName]=filter;
					break;
					case AL_FILTER_BANDPASS:
						// Set bandpass filter properties
						alFilterf(filter, AL_LOWPASS_GAIN, gain);
						alFilterf(filter, AL_BANDPASS_GAINLF, lfGain);
						alFilterf(filter, AL_LOWPASS_GAINHF, hfGain);
						mFilterList[fName]=filter;
					break;
					default:
						Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::createEFXFilter() - Unknown Filter Type");
					break;
				}
			}
		}
		else
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::createEFXFilter() - Created filter is not valid!");
			// Destroy invalid filter we just created
			alDeleteFilters( 1, &filter);

			return false;
		}
		return true;
	}

	/*/////////////////////////////////////////////////////////////////*/
#	if HAVE_EFX == 1
	bool OgreOggSoundManager::createEFXEffect(const Ogre::String& eName, ALint effectType, EAXREVERBPROPERTIES* props)
#	elif HAVE_EFX == 2
	bool OgreOggSoundManager::createEFXEffect(const Ogre::String& eName, ALint effectType, EFXEAXREVERBPROPERTIES* props)
#	endif
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::createEFXEffect() - No EFX support!");
			return false;
		}

		if ( eName.empty() )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::createEFXEffect() - Empty effect name!");
			return false;
		}

		if ( !isEffectSupported(effectType) )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::createEFXEffect() - Unsupported effect!");
			return false;
		}

		if ( mEffectList.find(eName) != mEffectList.end() )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::createEFXEffect() - Effect with name \"" + eName + "\" already exists!");
			return false;
		}

		ALuint effect;

		alGenEffects(1, &effect);
		if (alGetError() != AL_NO_ERROR)
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::createEFXEffect() - Cannot create EFX effect!");
			return false;
		}

		if (alIsEffect(effect))
		{
			alEffecti(effect, AL_EFFECT_TYPE, effectType);
			if (alGetError() != AL_NO_ERROR)
			{
				Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::createEFXEffect() - Effect not supported!");
				// Destroy invalid effect we just created
				alDeleteEffects(1, &effect);

				return false;
			}
			else
			{
				// Apply some preset reverb properties
				if ( effectType==AL_EFFECT_EAXREVERB && props )
				{
#	if HAVE_EFX == 1
					EFXEAXREVERBPROPERTIES eaxProps;
					ConvertReverbParameters(props, &eaxProps);
					_setEAXReverbProperties(&eaxProps, effect);
#	elif HAVE_EFX == 2
					_setEAXReverbProperties(props, effect);
#	endif
				}

				// Add to list
				mEffectList[eName]=effect;
			}
		}
		return true;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::_setEAXReverbProperties(EFXEAXREVERBPROPERTIES *pEFXEAXReverb, ALuint uiEffect)
	{
		if (pEFXEAXReverb)
		{
			// Clear AL Error code
			alGetError();

			// Determine type of 'Reverb' effect and apply correct settings
			ALint type;
			alGetEffecti(uiEffect, AL_EFFECT_TYPE, &type);

			// Apply selected presets to normal reverb
			if ( type==AL_EFFECT_REVERB )
			{
				alEffectf(uiEffect, AL_REVERB_DENSITY, pEFXEAXReverb->flDensity);
				alEffectf(uiEffect, AL_REVERB_DIFFUSION, pEFXEAXReverb->flDiffusion);
				alEffectf(uiEffect, AL_REVERB_GAIN, pEFXEAXReverb->flGain);
				alEffectf(uiEffect, AL_REVERB_GAINHF, pEFXEAXReverb->flGainHF);
				alEffectf(uiEffect, AL_REVERB_DECAY_TIME, pEFXEAXReverb->flDecayTime);
				alEffectf(uiEffect, AL_REVERB_DECAY_HFRATIO, pEFXEAXReverb->flDecayHFRatio);
				alEffectf(uiEffect, AL_REVERB_REFLECTIONS_GAIN, pEFXEAXReverb->flReflectionsGain);
				alEffectf(uiEffect, AL_REVERB_REFLECTIONS_DELAY, pEFXEAXReverb->flReflectionsDelay);
				alEffectf(uiEffect, AL_REVERB_LATE_REVERB_GAIN, pEFXEAXReverb->flLateReverbGain);
				alEffectf(uiEffect, AL_REVERB_LATE_REVERB_DELAY, pEFXEAXReverb->flLateReverbDelay);
				alEffectf(uiEffect, AL_REVERB_AIR_ABSORPTION_GAINHF, pEFXEAXReverb->flAirAbsorptionGainHF);
				alEffectf(uiEffect, AL_REVERB_ROOM_ROLLOFF_FACTOR, pEFXEAXReverb->flRoomRolloffFactor);
				alEffecti(uiEffect, AL_REVERB_DECAY_HFLIMIT, pEFXEAXReverb->iDecayHFLimit);
			}
			// Apply full EAX reverb settings
			else
			{
				alEffectf(uiEffect, AL_EAXREVERB_DENSITY, pEFXEAXReverb->flDensity);
				alEffectf(uiEffect, AL_EAXREVERB_DIFFUSION, pEFXEAXReverb->flDiffusion);
				alEffectf(uiEffect, AL_EAXREVERB_GAIN, pEFXEAXReverb->flGain);
				alEffectf(uiEffect, AL_EAXREVERB_GAINHF, pEFXEAXReverb->flGainHF);
				alEffectf(uiEffect, AL_EAXREVERB_GAINLF, pEFXEAXReverb->flGainLF);
				alEffectf(uiEffect, AL_EAXREVERB_DECAY_TIME, pEFXEAXReverb->flDecayTime);
				alEffectf(uiEffect, AL_EAXREVERB_DECAY_HFRATIO, pEFXEAXReverb->flDecayHFRatio);
				alEffectf(uiEffect, AL_EAXREVERB_DECAY_LFRATIO, pEFXEAXReverb->flDecayLFRatio);
				alEffectf(uiEffect, AL_EAXREVERB_REFLECTIONS_GAIN, pEFXEAXReverb->flReflectionsGain);
				alEffectf(uiEffect, AL_EAXREVERB_REFLECTIONS_DELAY, pEFXEAXReverb->flReflectionsDelay);
				alEffectfv(uiEffect, AL_EAXREVERB_REFLECTIONS_PAN, pEFXEAXReverb->flReflectionsPan);
				alEffectf(uiEffect, AL_EAXREVERB_LATE_REVERB_GAIN, pEFXEAXReverb->flLateReverbGain);
				alEffectf(uiEffect, AL_EAXREVERB_LATE_REVERB_DELAY, pEFXEAXReverb->flLateReverbDelay);
				alEffectfv(uiEffect, AL_EAXREVERB_LATE_REVERB_PAN, pEFXEAXReverb->flLateReverbPan);
				alEffectf(uiEffect, AL_EAXREVERB_ECHO_TIME, pEFXEAXReverb->flEchoTime);
				alEffectf(uiEffect, AL_EAXREVERB_ECHO_DEPTH, pEFXEAXReverb->flEchoDepth);
				alEffectf(uiEffect, AL_EAXREVERB_MODULATION_TIME, pEFXEAXReverb->flModulationTime);
				alEffectf(uiEffect, AL_EAXREVERB_MODULATION_DEPTH, pEFXEAXReverb->flModulationDepth);
				alEffectf(uiEffect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, pEFXEAXReverb->flAirAbsorptionGainHF);
				alEffectf(uiEffect, AL_EAXREVERB_HFREFERENCE, pEFXEAXReverb->flHFReference);
				alEffectf(uiEffect, AL_EAXREVERB_LFREFERENCE, pEFXEAXReverb->flLFReference);
				alEffectf(uiEffect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, pEFXEAXReverb->flRoomRolloffFactor);
				alEffecti(uiEffect, AL_EAXREVERB_DECAY_HFLIMIT, pEFXEAXReverb->iDecayHFLimit);
			}

			// Check that there weren't any errors
			if (alGetError() == AL_NO_ERROR)
			{
				return true;
			}
			else
			{
				Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::_setEAXReverbProperties() - Failed to set EFXEAXReverb");
				return false;
			}
		}

		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::createEFXSlot()
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::createEFXSlot() - No EFX support!");
			return false;
		}

		ALuint slot;

		alGenAuxiliaryEffectSlots(1, &slot);

		if (alGetError() != AL_NO_ERROR)
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::createEFXSlot() - Cannot create Auxiliary Effect slot!");
			return false;
		}
		else
		{
			mEffectSlotList.push_back(slot);
		}

		return true;
	}

	/*/////////////////////////////////////////////////////////////////*/
	int OgreOggSoundManager::getNumberOfCreatedEffectSlots()
	{
		if ( mEffectSlotList.empty() ) return 0;

		return static_cast<int>(mEffectSlotList.size());
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::setEFXSlotGain(ALuint slotID, float gain)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::setEFXSlotGain() - No EFX support!");
			return false;
		}

		if ( gain < 0 || gain > 1 )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::setEFXSlotGain() - Invalid value for gain!");
			return false;
		}

		if ( slotID > getNumberOfCreatedEffectSlots() - 1 )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::setEFXSlotGain() - Invalid slotID!");
			return false;
		}

		ALuint slot = _getEFXSlot(slotID);

		if ( slot == AL_NONE )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::setEFXSlotGain() - Unable to retrieve slotID!");
			return false;
		}

		alAuxiliaryEffectSlotf(slot, AL_EFFECTSLOT_GAIN, gain);

		if (alGetError() != AL_NO_ERROR)
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::setEFXSlotGain() - Cannot set Auxiliary Effect slot gain!");
			return false;
		}

		return true;
	}

	/*/////////////////////////////////////////////////////////////////*/
	float OgreOggSoundManager::getEFXSlotGain(ALuint slotID)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::getEFXSlotGain() - No EFX support!");
			return -1;
		}

		if ( slotID > getNumberOfCreatedEffectSlots() - 1 )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::getEFXSlotGain() - Invalid slotID!");
			return -1;
		}

		ALuint slot = _getEFXSlot(slotID);

		if ( slot == AL_NONE )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::getEFXSlotGain() - Unable to retrieve slotID!");
			return -1;
		}

		ALfloat gain;

		alGetAuxiliaryEffectSlotf(slot, AL_EFFECTSLOT_GAIN, &gain);

		if (alGetError() != AL_NO_ERROR)
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::getEFXSlotGain() - Cannot retrieve Auxiliary Effect slot gain!");
			return -1;
		}

		return gain;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::attachEffectToSlot(const Ogre::String& effectName, ALuint slotID)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::attachEffectToSlot() - No EFX support!");
			return false;
		}

		if ( slotID > getNumberOfCreatedEffectSlots() - 1 )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::attachEffectToSlot() - Invalid slotID!");
			return false;
		}

		ALuint slot;
		ALuint effect;

		// Get effect id's
		slot	= _getEFXSlot(slotID);
		effect	= _getEFXEffect(effectName);

		if ( slot == AL_NONE )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::attachEffectToSlot() - Unable to retrieve slotID!");
			return false;
		}

		if ( effect == AL_EFFECT_NULL )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::attachEffectToSlot() - Unable to retrieve effect: " + effectName);
			return false;
		}

		/* Attach Effect to Auxiliary Effect Slot */
		/* slot is the ID of an Aux Effect Slot */
		/* effect is the ID of an Effect */
		alAuxiliaryEffectSloti(slot, AL_EFFECTSLOT_EFFECT, effect);

		if (alGetError() != AL_NO_ERROR)
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::attachEffectToSlot() - Cannot attach effect to slot!");
			return false;
		}
		return true;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::detachEffectFromSlot(ALuint slotID)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::detachEffectFromSlot() - No EFX support!");
			return false;
		}

		if ( slotID > getNumberOfCreatedEffectSlots() - 1 )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::detachEffectFromSlot() - Invalid slotID!");
			return false;
		}

		ALuint slot;
		ALuint effect;

		slot = _getEFXSlot(slotID);

		if ( slot == AL_NONE )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::detachEffectFromSlot() - Unable to retrieve slotID!");
			return false;
		}

		/* Remove Effect from an Auxiliary Effect Slot */
		/* slot is the ID of an Aux Effect Slot */
		/* Attaching a NULL effect to an auxiliary effect slot to unload any effect 
		   that might be occupying the slot and free the associated resources. */
		alAuxiliaryEffectSloti(slot, AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);

		if (alGetError() != AL_NO_ERROR)
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::detachEffectFromSlot() - Cannot detach effect from slot!");
			return false;
		}
		return true;
	}

#if HAVE_ALEXT == 1
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::concatenateEFXEffectSlots(ALuint srcSlotID, ALuint dstSlotID)
	{
		if ( mEffectSlotList.empty() ) {
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::concatenateEFXEffectSlots() - No EFX slots created yet!");
			return false;
		}

		if ( srcSlotID == dstSlotID )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::concatenateEFXEffectSlots() - Won't concatenate a slot to itself!");
			return false;
		}

		if (alIsExtensionPresent("AL_SOFT_effect_target") == AL_FALSE)
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::concatenateEFXEffectSlots() - Extension AL_SOFT_effect_target not detected! ***");
			return false;
		}

		ALuint srcSlot;
		ALuint dstSlot;

		// Get effect from their slot IDs
		srcSlot	= _getEFXSlot(srcSlotID);
		dstSlot	= _getEFXSlot(dstSlotID);

		/*
		Docs:
		 - https://openal-soft.org/openal-extensions/SOFT_effect_target.txt
		 - https://github.com/kcat/openal-soft/issues/57

		alAuxiliaryEffectSloti(): The output of srcSlot is added to the input of dstSlot.
		So if srcSlot has an echo effect loaded, and dstSlot has a reverb effect, the echoes will be used as input to the reverb 
		(a source would not need to send to dstSlot explicitly, just to srcSlot is enough).
		Some things to note:
		 - Each effect slot can only have one target (though an effect slot can act as the target for multiple other effect slots and sources).
		 - You can't delete an effect slot that is currently the target of another.
		 - It's an error to create a circular chain.
		*/
		alAuxiliaryEffectSloti(srcSlot, AL_EFFECTSLOT_TARGET_SOFT, dstSlot);

		if (alGetError() != AL_NO_ERROR)
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::concatenateEFXEffectSlots() - Cannot concatenate Auxiliary Effect slots!");
			return false;
		}
		else
		{
			// Due to the fact that you can't delete an effect slot that is currently the target of another, it is necessary to maintain an accounting of the slot concatenations.
			// Otherwise it won't be possible to correctly release the auxiliary slots on termination of the plugin
			mEffectSlotMultiMap.insert(std::pair<ALuint, ALuint>(dstSlot, srcSlot));
		}

		return true;
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_delConcatenatedEFXEffectSlots(ALuint slot, std::multimap<ALuint, ALuint> *effectSlotMultiMap)
	{
		// Go through each of the tree nodes and recursively call this function on the children
		std::pair <std::multimap<ALuint, ALuint>::iterator, std::multimap<ALuint, ALuint>::iterator> ret;
		ret = effectSlotMultiMap->equal_range(slot);
		for (std::multimap<ALuint, ALuint>::iterator it=ret.first; it!=ret.second; ++it)
			_delConcatenatedEFXEffectSlots(it->second, effectSlotMultiMap);

		alDeleteAuxiliaryEffectSlots(1, &slot);

		// Find slot in mEffectSlotList and remove it
		for (EffectSlotList::iterator it = mEffectSlotList.begin(); it != mEffectSlotList.end(); ++it)
		{
			if(*it == slot) {
				mEffectSlotList.erase(it);
				break;
			}
		}
	}
#endif

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::setEFXSoundProperties(const Ogre::String& sName, float airAbsorption, float roomRolloff, float coneOuterHF)
	{
		OgreOggISound* sound=0;

		if ( !(sound = getSound(sName)) )
			return false;

#if OGGSOUND_THREADED
		SoundAction action;
		efxProperty* e	= OGRE_NEW_T(efxProperty, Ogre::MEMCATEGORY_GENERAL);
		e->mEffectName	= "";
		e->mFilterName	= "";
		e->mSlotID		= 255;
		e->mAirAbsorption = airAbsorption;
		e->mRolloff		= roomRolloff;
		e->mConeHF		= coneOuterHF;
		action.mAction	= LQ_SET_EFX_PROPERTY;
		action.mParams	= e;
		action.mSound	= sound->getName();
		action.mImmediately = false;
		_requestSoundAction(action);
		return true;
#else
		// load audio data
		return _setEFXSoundPropertiesImpl(sound);
#endif
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::_setEFXSoundPropertiesImpl(OgreOggISound* sound, float airAbsorption, float roomRolloff, float coneOuterHF)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_setEFXSoundPropertiesImpl() - No EFX support!");
			return false;
		}

		if ( !sound )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::_setEFXSoundPropertiesImpl() - NULL pointer sound!");
			return false;
		}

		ALuint src = sound->getSource();

		if ( src != AL_NONE )
		{
			alGetError();

			alSourcef(src, AL_AIR_ABSORPTION_FACTOR, airAbsorption);
			alSourcef(src, AL_ROOM_ROLLOFF_FACTOR, roomRolloff);
			alSourcef(src, AL_CONE_OUTER_GAINHF, coneOuterHF);

			if ( alGetError() == AL_NO_ERROR )
			{
				return true;
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::setEFXSoundProperties() - Unable to set EFX sound properties!");
				return false;
			}
		}
		else
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::setEFXSoundProperties() - No source attached to sound!");
			return false;
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::setEFXDistanceUnits(float units)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::setEFXDistanceUnits() - No EFX support!");
			return false;
		}

		if ( units <= 0 )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::setEFXDistanceUnits() - Invalid units value: " + Ogre::StringConverter::toString(units));
			return false;
		}

		alGetError();

		alListenerf(AL_METERS_PER_UNIT, units);

		if ( alGetError() == AL_NO_ERROR )
		{
			return true;
		}
		else
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::setEFXDistanceUnits() - Unable to set Listener distance units!");
			return false;
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	ALuint OgreOggSoundManager::_getEFXFilter(const Ogre::String& fName)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_getEFXFilter() - No EFX support!");
			return false;
		}

		if ( fName.empty() )
			return AL_FILTER_NULL;

		if ( mFilterList.empty() )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::_getEFXFilter() - No EFX filters have been created!");
			return AL_FILTER_NULL;
		}

		FilterList::iterator filter=mFilterList.find(fName);
		if ( filter==mFilterList.end() )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::_getEFXFilter() - Filter with name \"" + fName + "\" not found!");
			return AL_FILTER_NULL;
		}
		else
			return filter->second;
	}

	/*/////////////////////////////////////////////////////////////////*/
	ALuint OgreOggSoundManager::_getEFXEffect(const Ogre::String& eName)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_getEFXEffect() - No EFX support!");
			return false;
		}

		if ( eName.empty() )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::_getEFXEffect() - Empty effect name!");
			return AL_EFFECT_NULL;
		}

		if ( mEffectList.empty() )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::_getEFXEffect() - No EFX effects have been created!");
			return AL_EFFECT_NULL;
		}

		EffectList::iterator effect = mEffectList.find(eName);
		if ( effect == mEffectList.end() )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::_getEFXEffect() - Effect with name \"" + eName + "\" not found!");
			return AL_EFFECT_NULL;
		}
		else
			return effect->second;
	}

	/*/////////////////////////////////////////////////////////////////*/
	ALuint OgreOggSoundManager::_getEFXSlot(int slotID)
	{
		if ( !hasEFXSupport() )
		{
			Ogre::LogManager::getSingleton().logMessage("OgreOggSoundManager::_getEFXSlot() - No EFX support!");
			return false;
		}

		if ( mEffectSlotList.empty() || ( slotID >= static_cast<int>(mEffectSlotList.size()) ) )
		{
			Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::_getEFXSlot() - Unable to retrieve slot: " + Ogre::StringConverter::toString(slotID));
			return AL_NONE;
		}

		return static_cast<ALuint>(mEffectSlotList[slotID]);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_determineAuxEffectSlots()
	{
		ALuint		uiEffectSlots[128] = { 0 };
		ALuint		uiEffects[1] = { 0 };
		ALuint		uiFilters[1] = { 0 };

		// To determine how many Auxiliary Effects Slots are available,
		// create as many as possible (up to 128) until the call fails.
		for (mNumEffectSlots = 0; mNumEffectSlots < 128; mNumEffectSlots++)
		{
			alGenAuxiliaryEffectSlots(1, &uiEffectSlots[mNumEffectSlots]);
			if (alGetError() != AL_NO_ERROR)
				break;
		}

		Ogre::LogManager::getSingleton().logMessage(Ogre::StringConverter::toString(mNumEffectSlots) + " Auxiliary Effect Slot(s)");

		// Retrieve the number of Auxiliary Effect Slots Sends available on each Source
		alcGetIntegerv(mDevice, ALC_MAX_AUXILIARY_SENDS, 1, &mNumSendsPerSource);
		Ogre::LogManager::getSingleton().logMessage(Ogre::StringConverter::toString(mNumSendsPerSource) + " Auxiliary Send(s) per Source");

		Ogre::LogManager::getSingleton().logMessage("Effects supported:");
		alGenEffects(1, &uiEffects[0]);
		if (alGetError() == AL_NO_ERROR)
		{
			int effects[] = {AL_EFFECT_REVERB,
							AL_EFFECT_EAXREVERB,
							AL_EFFECT_CHORUS,
							AL_EFFECT_DISTORTION,
							AL_EFFECT_ECHO,
							AL_EFFECT_FLANGER,
							AL_EFFECT_FREQUENCY_SHIFTER,
							AL_EFFECT_VOCAL_MORPHER,
							AL_EFFECT_PITCH_SHIFTER,
							AL_EFFECT_RING_MODULATOR,
							AL_EFFECT_AUTOWAH,
							AL_EFFECT_COMPRESSOR,
							AL_EFFECT_EQUALIZER};
			const char* effVals[] = {"AL_EFFECT_REVERB",
									"AL_EFFECT_EAXREVERB",
									"AL_EFFECT_CHORUS",
									"AL_EFFECT_DISTORTION",
									"AL_EFFECT_ECHO",
									"AL_EFFECT_FLANGER",
									"AL_EFFECT_FREQUENCY_SHIFTER",
									"AL_EFFECT_VOCAL_MORPHER",
									"AL_EFFECT_PITCH_SHIFTER",
									"AL_EFFECT_RING_MODULATOR",
									"AL_EFFECT_AUTOWAH",
									"AL_EFFECT_COMPRESSOR",
									"AL_EFFECT_EQUALIZER"};
			// Try setting Effect Type to known Effects
			for(int i = 0; i < sizeof(effects)/sizeof(int); i++)
			{
				alEffecti(uiEffects[0], AL_EFFECT_TYPE, effects[i]);
				mEFXSupportList[effects[i]] = (alGetError() == AL_NO_ERROR);
				auto strVal = Ogre::StringConverter::toString(mEFXSupportList[AL_EFFECT_REVERB], true);
				Ogre::LogManager::getSingleton().logMessage(" * " + Ogre::String(effVals[i])+": "+strVal);
			}
		}


		// To determine which Filters are supported, generate a Filter Object, and try to set its type to
		// the various Filter enum values
		Ogre::LogManager::getSingleton().logMessage("Filters supported:");

		// Generate a Filter to use to determine what Filter Types are supported
		alGenFilters(1, &uiFilters[0]);
		if (alGetError() == AL_NO_ERROR)
		{
			int filters[] = {AL_FILTER_LOWPASS, AL_FILTER_HIGHPASS, AL_FILTER_BANDPASS};
			const char* filtStrs[] = {"AL_FILTER_LOWPASS", "AL_FILTER_HIGHPASS", "AL_FILTER_BANDPASS"};

			// Try setting the Filter type to known Filters
			for(int i = 0; i < sizeof(filters)/sizeof(int); i++)
			{
				alFilteri(uiFilters[0], AL_FILTER_TYPE, filters[i]);
				mEFXSupportList[filters[i]] = (alGetError() == AL_NO_ERROR);
				auto strVal = Ogre::StringConverter::toString(mEFXSupportList[AL_EFFECT_REVERB], true);
				Ogre::LogManager::getSingleton().logMessage(" * " + Ogre::String(filtStrs[i])+": "+strVal);
			}
		}

		// Delete Filter
		alDeleteFilters(1, &uiFilters[0]);

		// Delete Effect
		alDeleteEffects(1, &uiEffects[0]);

		// Delete Auxiliary Effect Slots
		alDeleteAuxiliaryEffectSlots(mNumEffectSlots, uiEffectSlots);
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::_checkEFXSupport()
	{
		if (alcIsExtensionPresent(mDevice, "ALC_EXT_EFX"))
		{
			// Get function pointers
			alGenEffects = (LPALGENEFFECTS)alGetProcAddress("alGenEffects");
			alDeleteEffects = (LPALDELETEEFFECTS )alGetProcAddress("alDeleteEffects");
			alIsEffect = (LPALISEFFECT )alGetProcAddress("alIsEffect");
			alEffecti = (LPALEFFECTI)alGetProcAddress("alEffecti");
			alEffectiv = (LPALEFFECTIV)alGetProcAddress("alEffectiv");
			alEffectf = (LPALEFFECTF)alGetProcAddress("alEffectf");
			alEffectfv = (LPALEFFECTFV)alGetProcAddress("alEffectfv");
			alGetEffecti = (LPALGETEFFECTI)alGetProcAddress("alGetEffecti");
			alGetEffectiv = (LPALGETEFFECTIV)alGetProcAddress("alGetEffectiv");
			alGetEffectf = (LPALGETEFFECTF)alGetProcAddress("alGetEffectf");
			alGetEffectfv = (LPALGETEFFECTFV)alGetProcAddress("alGetEffectfv");
			alGenFilters = (LPALGENFILTERS)alGetProcAddress("alGenFilters");
			alDeleteFilters = (LPALDELETEFILTERS)alGetProcAddress("alDeleteFilters");
			alIsFilter = (LPALISFILTER)alGetProcAddress("alIsFilter");
			alFilteri = (LPALFILTERI)alGetProcAddress("alFilteri");
			alFilteriv = (LPALFILTERIV)alGetProcAddress("alFilteriv");
			alFilterf = (LPALFILTERF)alGetProcAddress("alFilterf");
			alFilterfv = (LPALFILTERFV)alGetProcAddress("alFilterfv");
			alGetFilteri = (LPALGETFILTERI )alGetProcAddress("alGetFilteri");
			alGetFilteriv= (LPALGETFILTERIV )alGetProcAddress("alGetFilteriv");
			alGetFilterf = (LPALGETFILTERF )alGetProcAddress("alGetFilterf");
			alGetFilterfv= (LPALGETFILTERFV )alGetProcAddress("alGetFilterfv");
			alGenAuxiliaryEffectSlots = (LPALGENAUXILIARYEFFECTSLOTS)alGetProcAddress("alGenAuxiliaryEffectSlots");
			alDeleteAuxiliaryEffectSlots = (LPALDELETEAUXILIARYEFFECTSLOTS)alGetProcAddress("alDeleteAuxiliaryEffectSlots");
			alIsAuxiliaryEffectSlot = (LPALISAUXILIARYEFFECTSLOT)alGetProcAddress("alIsAuxiliaryEffectSlot");
			alAuxiliaryEffectSloti = (LPALAUXILIARYEFFECTSLOTI)alGetProcAddress("alAuxiliaryEffectSloti");
			alAuxiliaryEffectSlotiv = (LPALAUXILIARYEFFECTSLOTIV)alGetProcAddress("alAuxiliaryEffectSlotiv");
			alAuxiliaryEffectSlotf = (LPALAUXILIARYEFFECTSLOTF)alGetProcAddress("alAuxiliaryEffectSlotf");
			alAuxiliaryEffectSlotfv = (LPALAUXILIARYEFFECTSLOTFV)alGetProcAddress("alAuxiliaryEffectSlotfv");
			alGetAuxiliaryEffectSloti = (LPALGETAUXILIARYEFFECTSLOTI)alGetProcAddress("alGetAuxiliaryEffectSloti");
			alGetAuxiliaryEffectSlotiv = (LPALGETAUXILIARYEFFECTSLOTIV)alGetProcAddress("alGetAuxiliaryEffectSlotiv");
			alGetAuxiliaryEffectSlotf = (LPALGETAUXILIARYEFFECTSLOTF)alGetProcAddress("alGetAuxiliaryEffectSlotf");
			alGetAuxiliaryEffectSlotfv = (LPALGETAUXILIARYEFFECTSLOTFV)alGetProcAddress("alGetAuxiliaryEffectSlotfv");

			if (alGenEffects &&	alDeleteEffects && alIsEffect && alEffecti && alEffectiv &&	alEffectf &&
				alEffectfv && alGetEffecti && alGetEffectiv && alGetEffectf && alGetEffectfv &&	alGenFilters &&
				alDeleteFilters && alIsFilter && alFilteri && alFilteriv &&	alFilterf && alFilterfv &&
				alGetFilteri &&	alGetFilteriv && alGetFilterf && alGetFilterfv && alGenAuxiliaryEffectSlots &&
				alDeleteAuxiliaryEffectSlots &&	alIsAuxiliaryEffectSlot && alAuxiliaryEffectSloti &&
				alAuxiliaryEffectSlotiv && alAuxiliaryEffectSlotf && alAuxiliaryEffectSlotfv &&
				alGetAuxiliaryEffectSloti && alGetAuxiliaryEffectSlotiv && alGetAuxiliaryEffectSlotf &&
				alGetAuxiliaryEffectSlotfv)
				return true;
		}

		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
#	if HAVE_EFX == 1
	bool OgreOggSoundManager::_checkXRAMSupport()
	{
		// Check for X-RAM extension
		if(alIsExtensionPresent("EAX-RAM") == AL_TRUE)
		{
			// Get X-RAM Function pointers
			mEAXSetBufferMode = (EAXSetBufferMode)alGetProcAddress("EAXSetBufferMode");
			mEAXGetBufferMode = (EAXGetBufferMode)alGetProcAddress("EAXGetBufferMode");

			if (mEAXSetBufferMode && mEAXGetBufferMode)
			{
				mXRamSize = alGetEnumValue("AL_EAX_RAM_SIZE");
				mXRamFree = alGetEnumValue("AL_EAX_RAM_FREE");
				mXRamAuto = alGetEnumValue("AL_STORAGE_AUTOMATIC");
				mXRamHardware = alGetEnumValue("AL_STORAGE_HARDWARE");
				mXRamAccessible = alGetEnumValue("AL_STORAGE_ACCESSIBLE");

				if (mXRamSize && mXRamFree && mXRamAuto && mXRamHardware && mXRamAccessible)
				{
					// Support available
					mXRamSizeMB = alGetInteger(mXRamSize) / (1024*1024);
					mXRamFreeMB = alGetInteger(mXRamFree) / (1024*1024);
					return true;
				}
			}
		}
		return false;
	}
#	endif

#endif	// HAVE_EFX

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_destroyTemporarySound(OgreOggISound* sound)
	{
		if (!sound) return;

		mSoundsToDestroy->push(sound);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_destroyAllSoundsImpl()
	{
#if OGGSOUND_THREADED
		/** Mutex lock to avoid potential thread crashes. 
		*/
		OGRE_WQ_LOCK_MUTEX(mMutex);
#endif

		// Destroy all sounds
		Ogre::StringVector soundList;

#if OGGSOUND_THREADED
		OGRE_WQ_LOCK_MUTEX(mSoundMutex);
#endif

		// Get a list of all sound names
		for ( SoundMap::iterator i = mSoundMap.begin(); i != mSoundMap.end(); ++i )
			soundList.push_back(i->first);

		// Destroy individually outside mSoundMap iteration
		for ( Ogre::StringVector::iterator i = soundList.begin(); i != soundList.end(); ++i )
		{
			OgreOggISound* sound=0;
			if ( sound = getSound((*i)) )
				_destroySoundImpl(sound);
		}
		soundList.clear();

		// Shared buffers
		SharedBufferList::iterator b = mSharedBuffers.begin();
		while (b != mSharedBuffers.end())
		{
			if ( b->second->mRefCount>0 )
				alDeleteBuffers(1, &b->second->mAudioBuffer);
			OGRE_FREE(b->second, Ogre::MEMCATEGORY_GENERAL);
			++b;
		}

		mSharedBuffers.clear();

		// Clear queues
		mActiveSounds.clear();
		mPausedSounds.clear();
		mSoundsToReactivate.clear();
		mWaitingSounds.clear();
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_stopAllSoundsImpl()
	{
		if (mActiveSounds.empty()) return;

		for (ActiveList::const_iterator iter=mActiveSounds.begin(); iter!=mActiveSounds.end(); ++iter)
		{
			// If the sound was destroyed then we're not allowed to modify its state so don't bother trying to stop it.
			if ((*iter)->getState() != SS_DESTROYED)
			{
				(*iter)->_stopImpl();
			}
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_setGlobalPitchImpl()
	{
#if OGGSOUND_THREADED
		OGRE_WQ_LOCK_MUTEX(mSoundMutex);
#endif

		if ( mSoundMap.empty() ) return;

		// Affect all sounds
		for (SoundMap::const_iterator iter = mSoundMap.begin(); iter != mSoundMap.end(); ++iter)
			iter->second->setPitch(mGlobalPitch);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_pauseAllSoundsImpl()
	{
		if (mActiveSounds.empty()) return;

		for (ActiveList::const_iterator iter=mActiveSounds.begin(); iter!=mActiveSounds.end(); ++iter)
		{
			if ( (*iter)->isPlaying() && !(*iter)->isPaused() )
			{
				// Pause sound
				(*iter)->_pauseImpl();

				// Add to list to allow resuming
				mPausedSounds.push_back((*iter));
			}
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::addSoundToResume(OgreOggISound* sound)
	{
		// Add to list to allow resuming
		mPausedSounds.push_back(sound);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_resumeAllPausedSoundsImpl()
	{
		if (mPausedSounds.empty()) return;

		for (ActiveList::const_iterator iter=mPausedSounds.begin(); iter!=mPausedSounds.end(); ++iter)
			(*iter)->_playImpl();

		mPausedSounds.clear();
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_loadSoundImpl(	OgreOggISound* sound,
												const Ogre::String& file,
												bool prebuffer)
	{
		if ( !sound ) return;

		sharedAudioBuffer* buffer=0;

		if ( !sound->mStream )
			// Is there a shared buffer?
			buffer = _getSharedBuffer(file);

		if (!buffer)
		{
		    Ogre::DataStreamPtr stream = _openStream(file);

			// Load audio file
			sound->_openImpl(stream);
		}
		else
		{
			// Use shared buffer if available
			sound->_openImpl(file, buffer);

			// Increment the reference count since this buffer is now being used for another sound.
			++buffer->mRefCount;
		}

		// If requested to preBuffer - grab free source and init
		if (prebuffer)
		{
			if ( !_requestSoundSource(sound) )
			{
				Ogre::LogManager::getSingleton().logError("OgreOggSoundManager::_loadSoundImpl() - Failed to preBuffer sound: " + sound->getName());
			}
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_removeFromLists(OgreOggSound::OgreOggISound *sound)
	{
		// Remove from reactivate list
		if ( !mSoundsToReactivate.empty() )
		{
			// Remove ALL referneces to this sound..
			ActiveList::iterator iter=mSoundsToReactivate.begin(); 
			while ( iter!=mSoundsToReactivate.end() )
			{
				if ( sound==(*iter) )
					iter=mSoundsToReactivate.erase(iter);
				else
					++iter;
			}
		}

		/** Paused sound list - created by a call to pauseAllSounds()
		*/
		if ( !mPausedSounds.empty() )
		{
			// Remove ALL referneces to this sound..
			ActiveList::iterator iter=mPausedSounds.begin(); 
			while ( iter!=mPausedSounds.end() )
			{
				if ( sound==(*iter) )
					iter=mPausedSounds.erase(iter);
				else
					++iter;
			}
		}
		/** Waiting sound list
		*/
		if ( !mWaitingSounds.empty() )
		{
			// Remove ALL referneces to this sound..
			ActiveList::iterator iter=mWaitingSounds.begin(); 
			while ( iter!=mWaitingSounds.end() )
			{
				if ( sound==(*iter) )
					iter=mWaitingSounds.erase(iter);
				else
					++iter;
			}
		}
		/** Active sound list
		*/
		if ( !mActiveSounds.empty() )
		{
			// Remove ALL references to this sound..
			ActiveList::iterator iter=mActiveSounds.begin(); 
			while ( iter!=mActiveSounds.end() )
			{
				if ( sound==(*iter) )
					iter=mActiveSounds.erase(iter);
				else
					++iter;
			}
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_releaseSoundImpl(OgreOggISound* sound)
	{
		if (!sound) return;

#if OGGSOUND_THREADED
		OGRE_WQ_LOCK_MUTEX(mMutex);
#endif

		// Delete sound buffer
		ALuint src = sound->getSource();
		if ( src != AL_NONE ) _releaseSoundSource(sound);

		// Remove references from lists
		_removeFromLists(sound);

#if OGGSOUND_THREADED
		OGRE_WQ_LOCK_MUTEX(mSoundMutex);
#endif

		// Find sound in map and remove it
		SoundMap::iterator i = mSoundMap.find(sound->getName());
		mSoundMap.erase(i);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_destroySoundImpl(OgreOggISound* sound)
	{
		if ( !sound ) return;

		// Get SceneManager
		Ogre::SceneManager* s = sound->getSceneManager();
		s->destroyMovableObject(sound);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_destroyListener()
	{
		if ( !mListener ) return;

#if OGGSOUND_THREADED
		/** Dumb check to catch external destruction of sounds to avoid potential
			thread crashes. (manager issued destruction sets this flag)
		*/
		OGRE_WQ_LOCK_MUTEX(mMutex);
#endif

		OGRE_DELETE_T(mListener, OgreOggListener, Ogre::MEMCATEGORY_GENERAL);
		mListener = 0;
	}

	/*/////////////////////////////////////////////////////////////////*/
	Ogre::Real OgreOggSoundManager::_calculateDistanceToListener(OgreOggISound * sound, const Ogre::Vector3 & listenerPos)
	{
		return sound->isRelativeToListener() ? sound->getPosition().length() : sound->getPosition().distance(listenerPos);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_checkFeatureSupport()
	{
		// Supported Formats Info
		Ogre::LogManager::getSingleton().logMessage("Supported formats:");
		ALenum eBufferFormat = 0;

		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_MONO16");
		if(eBufferFormat)
			Ogre::LogManager::getSingleton().logMessage(" * AL_FORMAT_MONO16 -- Monophonic Sound");

		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_STEREO16");
		if(eBufferFormat)
			Ogre::LogManager::getSingleton().logMessage(" * AL_FORMAT_STEREO16 -- Stereo Sound");

		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_QUAD16");
		if(eBufferFormat)
			Ogre::LogManager::getSingleton().logMessage(" * AL_FORMAT_QUAD16 -- 4 Channel Sound");

		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_51CHN16");
		if(eBufferFormat)
			Ogre::LogManager::getSingleton().logMessage(" * AL_FORMAT_51CHN16 -- 5.1 Surround Sound");

		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_61CHN16");
		if(eBufferFormat)
			Ogre::LogManager::getSingleton().logMessage(" * AL_FORMAT_61CHN16 -- 6.1 Surround Sound");

		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_71CHN16");
		if(eBufferFormat)
			Ogre::LogManager::getSingleton().logMessage(" * AL_FORMAT_71CHN16 -- 7.1 Surround Sound");

#if HAVE_EFX
		// EFX
		mEFXSupport = _checkEFXSupport();
		if (mEFXSupport)
		{
			Ogre::LogManager::getSingleton().logMessage("EFX Detected");
			_determineAuxEffectSlots();
		}
		else
			Ogre::LogManager::getSingleton().logMessage("EFX NOT Detected");

#	if HAVE_EFX == 1
		// XRAM
		mXRamSupport = _checkXRAMSupport();
		if (mXRamSupport)
		{
			// Log message
			Ogre::LogManager::getSingleton().logMessage("X-RAM Detected");
			Ogre::LogManager::getSingleton().logMessage("X-RAM Size(MB): " + Ogre::StringConverter::toString(mXRamSizeMB) +
				" Free(MB):" + Ogre::StringConverter::toString(mXRamFreeMB));
		}
		else
			Ogre::LogManager::getSingleton().logMessage("XRAM NOT Detected");
#	endif

		// EAX
		for(int version = 5; version >= 2; version--)
		{
			Ogre::String eaxName="EAX"+Ogre::StringConverter::toString(version) + ".0";
			if(alIsExtensionPresent(eaxName.c_str()) == AL_TRUE)
			{
				mEAXSupport = true;
				mEAXVersion = version;
				eaxName="EAX "+Ogre::StringConverter::toString(version) + ".0 Detected";
				Ogre::LogManager::getSingleton().logMessage(eaxName);
				break;
			}
		}
#endif
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_checkExtensionSupport()
	{
		const ALCchar* extensionList = alcGetString(mDevice, ALC_EXTENSIONS);

		// Unofficial OpenAL extension repository: https://github.com/openalext/openalext
		// OpenAL Soft extensions: https://openal-soft.org/openal-extensions/

		// List of extensions in log
		std::stringstream ss;

		while(*extensionList != 0)
		{
			ss << extensionList;

			extensionList += strlen(extensionList) + 1;
		}

		// Supported Extensions Info
		Ogre::LogManager::getSingleton().logMessage("ALC_EXTENSIONS = " + ss.str());
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_enumDevices()
	{
		// If possible use the Enumerate All Extension to get the extended device list
		if(alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT") == AL_TRUE)
			mDeviceStrings = const_cast<ALCchar*>(alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER));
		// Otherwise use the more basic Enumeration Extension
		else if (alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT") == AL_TRUE)
			mDeviceStrings = const_cast<ALCchar*>(alcGetString(NULL, ALC_DEVICE_SPECIFIER));
		// If the OpenAL version is prior to OpenAL 1.1, then there are no Enumeration Extensions
		else
			mDeviceStrings = NULL;
	}					  

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_releaseAll()
	{
		// Clear all of the various containers of sounds. This will make releasing MUCH faster since each sound won't have to
		// bother searching for and manually removing themselves from the lists, which really doesn't matter when everything is
		// being destroyed.
		mSoundsToReactivate.clear();
		mPausedSounds.clear();
		mWaitingSounds.clear();
		mActiveSounds.clear();
		mSourcePool.clear();

		stopAllSounds();
		_destroyAllSoundsImpl();

		// Delete sources
		SourceList::iterator it = mSources.begin();
		while (it != mSources.end())
		{
#if HAVE_EFX
			if ( hasEFXSupport() )
			{
				// Remove filters/effects
				alSourcei(static_cast<ALuint>((*it)), AL_DIRECT_FILTER, AL_FILTER_NULL);
				alSource3i(static_cast<ALuint>((*it)), AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
 			}
#endif
			alDeleteSources(1, &(*it));
			++it;
		}

		mSources.clear();

#if HAVE_EFX
		// clear EFX effect lists
		if ( !mFilterList.empty() )
		{
			FilterList::iterator iter=mFilterList.begin();
			for ( ; iter!=mFilterList.end(); ++iter )
			    alDeleteFilters( 1, &iter->second);
			mFilterList.clear();
		}

		if ( !mEffectList.empty() )
		{
			EffectList::iterator iter=mEffectList.begin();
			for ( ; iter!=mEffectList.end(); ++iter )
			    alDeleteEffects( 1, &iter->second);
			mEffectList.clear();
		}

#if HAVE_ALEXT == 1
		// In order to properly release the concatenated effect slots it is neccesary to delete them in order

		// Obtain the roots of the trees registered in the mEffectSlotMultiMap
		// Create a set with all the slots that are parents of someone and another set with all the slots that are children
		std::set<ALuint> sources, dests;
		for (std::multimap<ALuint, ALuint>::iterator it=mEffectSlotMultiMap.begin(); it!=mEffectSlotMultiMap.end(); ++it)
		{
			sources.insert(it->first);
			dests.insert(it->second);
		}

		// Get all the elements that are parents and take out all the elements that are children (that should leave only the roots)
		std::vector<ALuint> roots;
		roots.resize(sources.size());
		std::vector<ALuint>::iterator diff = std::set_difference (sources.begin(), sources.end(), dests.begin(), dests.end(), roots.begin());
		roots.resize(diff-roots.begin());

		for(auto root : roots)
			_delConcatenatedEFXEffectSlots(root, &mEffectSlotMultiMap);

		mEffectSlotMultiMap.clear();
#endif

		if ( !mEffectSlotList.empty() )
		{
			EffectSlotList::iterator iter=mEffectSlotList.begin();
			for ( ; iter!=mEffectSlotList.end(); ++iter )
			    alDeleteAuxiliaryEffectSlots( 1, &(*iter));
			mEffectSlotList.clear();
		}
#endif
	}

	/*/////////////////////////////////////////////////////////////////*/
	int OgreOggSoundManager::_createSourcePool()
	{
		ALuint source;
		ALenum error;
		unsigned int numSources = 0;

		while(numSources < mMaxSources)
		{
			alGenSources(1, &source);

			error = alGetError();

			if(error == AL_NO_ERROR)
			{
				mSourcePool.push_back(source);
				mSources.push_back(source);
				numSources++;
			}
			else
			{
				switch (error)
				{
					case AL_INVALID_VALUE:		{ Ogre::LogManager::getSingleton().logMessage("Invalid Value when attempting to create more OpenAL sources", Ogre::LML_NORMAL);} 		break;
					case AL_INVALID_OPERATION:	{ Ogre::LogManager::getSingleton().logMessage("Invalid Operation when attempting to create more OpenAL sources", Ogre::LML_NORMAL);} 	break;
					case AL_OUT_OF_MEMORY:		{ Ogre::LogManager::getSingleton().logMessage("Out of memory when attempting to create more OpenAL sources", Ogre::LML_NORMAL);} 		break;
				}

				break;
			}
		}

		return static_cast<int>(mSourcePool.size());
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_reactivateQueuedSoundsImpl()
	{
		// Pump waiting sounds first..
		if (!mWaitingSounds.empty())
		{
			OgreOggISound* sound = mWaitingSounds.front();

			// Grab a source
			if ( _requestSoundSource(sound) )
			{
				// Play
				sound->_playImpl();

				// Remove
				mWaitingSounds.erase(mWaitingSounds.begin());

				return;
			}
			else
				// Non available - quit
				return;
		}

		// Any sounds to re-activate?
		if (mSoundsToReactivate.empty()) return;

		if (mListener)
		{
			const Ogre::Vector3 listenerPos(mListener->getPosition());

			// Sort list by distance
			mActiveSounds.sort(_sortNearToFar(listenerPos));

			// Get sound object from front of list
			OgreOggISound* snd = mSoundsToReactivate.front();

			// Check sound hasn't been stopped or destroyed whilst in list
			if ( !snd->isPlaying() && snd->getState() != SS_DESTROYED)
			{
				// Try to request a source for sound
				if (_requestSoundSource(snd))
				{
					// play sound
					snd->_playImpl();
				}
			}
			// Else - kick off list
			else
			{
				mSoundsToReactivate.erase(mSoundsToReactivate.begin());
			}
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_reactivateQueuedSounds()
	{
		if ( mWaitingSounds.empty() && mSoundsToReactivate.empty() ) return;

#if OGGSOUND_THREADED
		SoundAction action;
		action.mAction	= LQ_REACTIVATE;
		action.mParams	= 0;
		action.mSound	= "";
		action.mImmediately = false;
		_requestSoundAction(action);
#else
		_reactivateQueuedSoundsImpl();
#endif
	}

	/*/////////////////////////////////////////////////////////////////*/
	sharedAudioBuffer* OgreOggSoundManager::_getSharedBuffer(const Ogre::String& sName)
	{
		if ( sName.empty() ) return AL_NONE;

		SharedBufferList::iterator f;
		if ( ( f = mSharedBuffers.find(sName) ) != mSharedBuffers.end() )
			return f->second;

		return AL_NONE;
	}	 

	/*/////////////////////////////////////////////////////////////////*/
	Ogre::DataStreamPtr OgreOggSoundManager::_openStream(const Ogre::String& file) const
	{
		Ogre::DataStreamPtr result;
		Ogre::ResourceGroupManager* groupManager = 0;
		Ogre::String group;

		try
		{
			if (groupManager = Ogre::ResourceGroupManager::getSingletonPtr())
			{
				// Have to use the getter to retrieve the mResourceGroupName member since this function can be called in a separate
				// thread and the getter has thread safety.
				const Ogre::String resourceGroupName = getResourceGroupName();

				if (!resourceGroupName.empty())
				{
					result = groupManager->openResource(file, resourceGroupName);
				}
				else
				{
					group = groupManager->findGroupContainingResource(file);
					result = groupManager->openResource(file, group);
				}
			}
			else
			{
				OGRE_EXCEPT(Ogre::Exception::ERR_FILE_NOT_FOUND, "Unable to find Ogre::ResourceGroupManager", "OgreOggSoundManager::_openStream()");
				result.reset();
			}
		}
		catch (Ogre::Exception& e)
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_FILE_NOT_FOUND, e.getFullDescription(), "OgreOggSoundManager::_openStream()");
			result.reset();
		}

		return result;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::_releaseSharedBuffer(const Ogre::String& sName, ALuint& buffer)
	{
		if ( sName.empty() ) return false;

		SharedBufferList::iterator f;
		if ( ( f = mSharedBuffers.find(sName) ) != mSharedBuffers.end() )
		{
			// Is it sharing buffer?
			if ( buffer == f->second->mAudioBuffer )
			{
				// Decrement
				f->second->mRefCount--;
				if ( f->second->mRefCount==0 )
				{
					// Delete buffer object
					alDeleteBuffers(1, &f->second->mAudioBuffer);

					// Delete struct
					OGRE_DELETE_T(f->second, sharedAudioBuffer, Ogre::MEMCATEGORY_GENERAL);

					// Remove from list
					mSharedBuffers.erase(f);
				}
				return true;
			}
		}
		return false;
	}	

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::_registerSharedBuffer(const Ogre::String& sName, ALuint& buffer, OgreOggISound* parent)
	{
		if ( sName.empty() ) return false;

		SharedBufferList::iterator f;
		if ( ( f = mSharedBuffers.find(sName) ) == mSharedBuffers.end() )
		{
			// Create struct
			sharedAudioBuffer* buf = OGRE_NEW_T(sharedAudioBuffer, Ogre::MEMCATEGORY_GENERAL);

			// Set buffer
			buf->mAudioBuffer = buffer;

			// Set ref count
			buf->mRefCount = 1;

			// Copy the shared information into the buffer so it can be passed around to every other sound that needs it.
			parent->_getSharedProperties(buf->mBuffers, buf->mPlayTime, buf->mFormat);

			// Add to list
			mSharedBuffers[sName] = buf;
		}
		return true;
	}

#if OGGSOUND_THREADED
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_updateBuffers()
	{
		static Ogre::uint32 cTime;
		static Ogre::uint32 pTime=0;
		static Ogre::Timer timer;
		static float rTime=0.f;

		// Get frame time
		cTime = timer.getMillisecondsCPU();
		float fTime = (cTime-pTime) * 0.001f;

		// update Listener
		if ( mListener ) 
			mListener->update();

		// Loop all active sounds
		ActiveList::const_iterator i = mActiveSounds.begin();
		while( i != mActiveSounds.end())
		{
			// update pos/fade
			(*i)->update(fTime);

			// Update buffers
			(*i)->_updateAudioBuffers();

			// Update recorder
			if ( mRecorder ) mRecorder->_updateRecording();

			// Next..
			++i;
		}

		// Reactivate 10fps
		if ( (rTime+=fTime) > 0.1f )
		{
			_reactivateQueuedSoundsImpl();
			rTime=0.f;
		}

		// Reset timer
		pTime=cTime;
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_performAction(const SoundAction& act)
	{
		switch (act.mAction)
		{
		case LQ_PLAY:			
			{ 
				if ( hasSound(act.mSound) )
					getSound(act.mSound)->_playImpl(); 
			} 
			break;
		case LQ_PAUSE:			
			{ 
				if ( hasSound(act.mSound) )
					getSound(act.mSound)->_pauseImpl(); 
			} 
			break;
		case LQ_STOP:			
			{ 
				if ( hasSound(act.mSound) )
					getSound(act.mSound)->_stopImpl(); 
			} 
			break;
		case LQ_REACTIVATE:		
			{ 
				_reactivateQueuedSoundsImpl(); 
			} 
			break;
		case LQ_GLOBAL_PITCH:	
			{ 
				_setGlobalPitchImpl(); 
			} 
			break;
		case LQ_STOP_ALL:		
			{ 
				_stopAllSoundsImpl(); 
			} 
			break;
		case LQ_PAUSE_ALL:		
			{ 
				_pauseAllSoundsImpl(); 
			} 
			break;
		case LQ_RESUME_ALL:		
			{ 
				_resumeAllPausedSoundsImpl(); 
			} 
			break;
		case LQ_LOAD:
			{
				cSound* c = static_cast<cSound*>(act.mParams);
				if ( hasSound(act.mSound) )
				{
					OgreOggISound* s = getSound(act.mSound); 
					_loadSoundImpl(s, c->mFileName, c->mPrebuffer);
				}

				// Delete
				OGRE_DELETE_T(c, cSound, Ogre::MEMCATEGORY_GENERAL);
			}
			break;
#if HAVE_EFX
		case LQ_ATTACH_EFX:
			{
				efxProperty* e = static_cast<efxProperty*>(act.mParams);
				if ( hasSound(act.mSound) )
				{
					OgreOggISound* s = getSound(act.mSound); 
					if ( !e->mEffectName.empty() && !e->mFilterName.empty() ) 
						_attachEffectToSoundImpl(s, e->mSend, e->mSlotID, e->mFilterName);
					else
						_attachFilterToSoundImpl(s, e->mFilterName);
				}
				// Delete
				OGRE_DELETE_T(e, efxProperty, Ogre::MEMCATEGORY_GENERAL);
			}
			break;
		case LQ_DETACH_EFX:
			{
				efxProperty* e = static_cast<efxProperty*>(act.mParams);
				if ( hasSound(act.mSound) )
				{
					OgreOggISound* s = getSound(act.mSound); 
					if ( e->mSlotID != 255 )
						_detachEffectFromSoundImpl(s, e->mSlotID);
					else
						_detachFilterFromSoundImpl(s);
				}
				// Delete
				OGRE_DELETE_T(e, efxProperty, Ogre::MEMCATEGORY_GENERAL);
			}
			break;
		case LQ_SET_EFX_PROPERTY:
			{
				efxProperty* e = static_cast<efxProperty*>(act.mParams);
				if ( hasSound(act.mSound) )
				{
					OgreOggISound* s = getSound(act.mSound); 
					_setEFXSoundPropertiesImpl(s, e->mAirAbsorption, e->mRolloff, e->mConeHF);
				}
				// Delete
				OGRE_DELETE_T(e, efxProperty, Ogre::MEMCATEGORY_GENERAL);
			}
			break;
#endif
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_requestSoundAction(const SoundAction& action)
	{
		// If user has requested a mutex be used for every action,
		// action is performed immediately and blocks main thread.
		if ( mForceMutex || action.mImmediately )
		{
			OGRE_WQ_LOCK_MUTEX(mMutex);
			_performAction(action);
			return;
		}

		if ( !mActionsList ) return;

		mActionsList->push(action);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_processQueuedSounds()
	{
		if ( !mActionsList ) return;

		SoundAction act;
		// Perform sound requests 
		while ( mActionsList->pop(act) )
		{
			_performAction(act);
		}
	}
#endif
}

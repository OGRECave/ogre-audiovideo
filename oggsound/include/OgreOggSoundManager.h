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
* DESCRIPTION: Manages the audio library
*/

#pragma once

#include "OgreOggSoundPrereqs.h"
#include "OgreOggSound.h"
#include "OgreOggISound.h"
#include "LocklessQueue.h"

#include <map>

namespace OgreOggSound
{
	typedef std::map<Ogre::String, OgreOggISound*> SoundMap;
	typedef std::map<Ogre::String, ALuint> FilterList;
	typedef std::map<Ogre::String, ALuint> EffectList;
	typedef std::map<ALenum, bool> FeatureList;
	typedef std::list<OgreOggISound*> ActiveList;
	typedef std::deque<ALuint> SourceList;
	typedef std::deque<ALuint> EffectSlotList;
	typedef std::multimap<ALuint, ALuint> SlotMultiMap;
	typedef std::vector<Ogre::String> RecordDeviceList;
	
	class OgreOggISound;


	//! Various sound commands
	enum SOUND_ACTION
	{
		LQ_PLAY,
		LQ_STOP,
		LQ_PAUSE,
		LQ_LOAD,
		LQ_GLOBAL_PITCH,
		LQ_STOP_ALL,
		LQ_PAUSE_ALL,
		LQ_RESUME_ALL,
		LQ_REACTIVATE,
		LQ_DESTROY_TEMPORARY,
		LQ_ATTACH_EFX,
		LQ_DETACH_EFX,
		LQ_SET_EFX_PROPERTY
	};

	//! Holds information about a sound action
	struct SoundAction
	{
		Ogre::String	mSound;
		SOUND_ACTION	mAction;
		bool			mImmediately;
		void*			mParams;
	};

	//! Holds information about a create sound request.
	struct cSound
	{
		bool mPrebuffer;
		Ogre::String mFileName;
		ALuint mBuffer;
	};

	//! Holds information about an EFX effect.
	struct efxProperty
	{
		Ogre::String mEffectName;
		Ogre::String mFilterName;
		float mAirAbsorption;
		float mRolloff;
		float mConeHF;
		ALuint mSend;
		ALuint mSlotID;
	};

	//! Sound Manager: Manages all sounds for an application
	class _OGGSOUND_EXPORT OgreOggSoundManager : public Ogre::Singleton<OgreOggSoundManager>
	{

	public:

		// Version string
		static const Ogre::String OGREOGGSOUND_VERSION_STRING;

		/** Creates a manager for all sounds within the application.
		 */
		OgreOggSoundManager();
		/** Destroys this manager.
		@remarks
			Destroys all sound objects and thread if defined.\n
			Cleans up all OpenAL objects, buffers and devices and closes down the audio device.
		 */
		~OgreOggSoundManager();
		/** Creates a listener object for the system
		@remarks
			Only needed when SceneManager->clearScene() or similar is used, 
			which destroys the listener object automatically without the manager knowing.\n
			You can therefore use this function to recreate a listener object for the system.
		 */
		bool createListener();
		/** Initialises the audio device.
		@remarks
			Attempts to initialise the audio device for sound playback.\n
			Internally some logging is done to list features supported as
			well as creating a pool of sources from which sounds can be
			attached and played.
			@param deviceName
				Audio device string to open, will use default device if not found.
			@param maxSources
				Maximum number of sources to allocate (optional)
			@param queueListSize
				Desired size of queue list (optional | Multi-threaded ONLY)
			@param scnMgr
				SceneManager to use in order create sounds (If no manager specified, uses the first one)
		 */
		bool init(const std::string &deviceName = "", unsigned int maxSources=100, unsigned int queueListSize=100, Ogre::SceneManager* scnMgr=0);
		/** Gets the OpenAL device pointer
		*/
		const ALCdevice* getOpenALDevice() { return mDevice; }
		/** Gets the OpenAL context pointer
		*/
		const ALCcontext* getOpenALContext() { return mContext; }
#if HAVE_ALEXT == 1
		/** Pauses the OpenAL device
		@remarks
			This function allows applications to pause a playback device.\n
			The main purpose of this is to silence output, stop processing, and allow the audio hardware to go into a low-power mode.\n
			On a mobile device, for instance, apps may want to silence output and not waste battery life with unneeded processing when in the background.
		@note
			Uses the ALC_SOFT_pause_device extension which is available in OpenAL Soft (https://openal-soft.org/openal-extensions/SOFT_pause_device.txt)
		*/
		void pauseOpenALDevice();
		/** Resumes previously paused OpenAL device
		@remarks
			This function allows applications to resume a paused playback device. 
		@note
			Uses the ALC_SOFT_pause_device extension which is available in OpenAL Soft (https://openal-soft.org/openal-extensions/SOFT_pause_device.txt)
		*/
		void resumeOpenALDevice();
#endif
		/** Sets the global volume for all sounds
			@param vol 
				Global attenuation for all sounds.
		 */
		void setMasterVolume(ALfloat vol);
		/** Sets the default SceneManager for creation of sound objects
		 */
		void setSceneManager(Ogre::SceneManager* sMan) { mSceneMgr=sMan; }
		/** Gets the default SceneManager for creation of sound objects
		 */
		Ogre::SceneManager* getSceneManager() { return mSceneMgr; }
		/** Gets number of currently created sounds
		 */
		unsigned int getNumSounds() const { return static_cast<unsigned int>(mSoundMap.size()); }
		/** Gets the current global volume for all sounds
		 */
		ALfloat getMasterVolume();
		/** Creates a single sound object.
		@remarks
			Plugin specific version of createSound, uses createMovableObject() to instantiate
			a sound automatically registered with the supplied SceneManager, 
			allows OGRE to automatically cleanup/manage this sound.\n
			Each sound must have a unique name within the manager.
			@param name 
				Unique name of sound
			@param file 
				Audio file path string or "BUFFER" for memory buffer sound (@ref OgreOggStreamBufferSound)
			@param stream 
				Flag indicating if the sound sound be streamed.
			@param loop 
				Flag indicating if the file should loop.
			@param preBuffer 
				Flag indicating if a source should be attached at creation.
			@param scnMgr
				Pointer to SceneManager this sound belongs - 0 defaults to first SceneManager defined.
			@param immediate
				Optional flag to indicate creation should occur immediately and not be passed to background thread for queueing.\n
				Can be used to overcome the random creation time which might not be acceptable (MULTI-THREADED ONLY)
		 */
		OgreOggISound* createSound(const Ogre::String& name,const Ogre::String& file, bool stream=false, bool loop=false, bool preBuffer=false, Ogre::SceneManager* scnMgr=0, bool immediate=false);
		/** Gets a named sound.
		@remarks
			Returns a named sound object if defined, NULL otherwise.
			@param name 
				Sound name.
		 */
		OgreOggISound *getSound(const Ogre::String& name);
		/** Gets list of created sounds.
		@remarks
			Returns a vector of sound name strings.
		 */
		const Ogre::StringVector getSoundList() const;
		/** Returns whether named sound exists.
		@remarks
			Checks sound map for a named sound.
			@param name 
				Sound name.
		 */
		bool hasSound(const Ogre::String& name);
		/** Sets the pitch of all sounds.
		@remarks
			Sets the pitch modifier applied to all sounds.
			@param pitch
				New pitch for all sounds (positive value)
		 */
		void setGlobalPitch(float pitch);
		/** Gets the current global pitch.
		 */
		const float getGlobalPitch() const { return mGlobalPitch; }
		/** Stops all currently playing sounds.
		 */
		void stopAllSounds();
		/** Pauses all currently playing sounds.
		 */
		void pauseAllSounds();
		/** Mutes all sounds.
		 */
		inline void muteAllSounds() { alGetListenerf(AL_GAIN, &mOrigVolume); setMasterVolume(0.f); }
		/** Un mutes all sounds.
		 */
		inline void unmuteAllSounds() { setMasterVolume(mOrigVolume); }
		/** Add single sound to list of sounds resumed on resumeAllPausedSounds call.
		@remarks
			Do not pause sound or check play/pause state.\n
			Only add to list of sounds to resume.
			@param sound 
				Sound pointer.
		 */
		void addSoundToResume(OgreOggISound* sound);
		/** Resumes all previously playing sounds.
		 */
		void resumeAllPausedSounds();
		/** Destroys all sounds within manager.
		 */
		void destroyAllSounds();
		/** Destroys a single sound.
		@remarks
			Destroys a single sound object.
			@param name
				Sound name to destroy.
		 */
		void destroySound(const Ogre::String& name="");
		/** Destroys a single sound.
		@remarks
			Destroys a single sound object.
			@param sound
				Sound to destroy.
		 */
		void destroySound(OgreOggISound* sound);

		/** Destroys a temporary sound.
		@remarks
			Internal use only.
			@param sound
				Sound to destroy.
		 */
		void _destroyTemporarySound(OgreOggISound* sound);
		/** Requests a free source object.
		@remarks
			Retrieves a free source object and attaches it to the specified sound object.\n
			Internally checks for any currently available sources, 
			then checks stopped sounds and finally prioritised sounds.
			@param sound 
				Sound pointer.
		@note
			Internal function - SHOULD NOT BE CALLED BY USER CODE.\n
		 */
		bool _requestSoundSource(OgreOggISound* sound=0);
		/** Release a sounds source.
		@remarks
			Releases a specified sounds source object back to the system,
			allowing it to be re-used by another sound.
			@param sound 
				Sound pointer.
		@note
			Internal function - SHOULD NOT BE CALLED BY USER CODE.\n
		 */
		bool _releaseSoundSource(OgreOggISound* sound=0);
		/** Releases a shared audio buffer
		@remarks
			Each shared audio buffer is reference counted so destruction is handled correctly,
			this function merely decrements the reference count, 
			only destroying when no sounds are referencing buffer.
			@param sName
				Name of audio file
			@param buffer
				Buffer ID
		@note
			Internal function - SHOULD NOT BE CALLED BY USER CODE.\n
		*/
		bool _releaseSharedBuffer(const Ogre::String& sName, ALuint& buffer);
		/** Registers a shared audio buffer
		@remarks
			Its possible to share audio buffer data among many sources so this function registers an audio buffer as 'sharable', 
			meaning if a the same audio file is created more then once, 
			it will simply use the original buffer data instead of creating/loading the same data again.
			@param sName
				Name of audio file
			@param buffer
				OpenAL buffer ID holding audio data
			@param parent
				Sound from where to copy the properties: Buffers, PlayTime, Format
		@note
			Internal function - SHOULD NOT BE CALLED BY USER CODE.\n
		 */
		bool _registerSharedBuffer(const Ogre::String& sName, ALuint& buffer, OgreOggISound* parent=0);
		/** Sets distance model.
		@remarks
			Sets the global distance attenuation algorithm used by all sounds in the system.
			@param value
				ALenum value of distance model.
		 */
		void setDistanceModel(ALenum value);
		/** Gets the distance model.
		@remarks
			Gets the global distance attenuation algorithm used by all sounds in the system.
			@param value
				ALenum value of distance model.
		 */
		const ALenum getDistanceModel() const;
		/** Sets doppler factor.
		@remarks
			Sets the global doppler factor which affects attenuation for all sounds
			@param factor
				Factor scale (>0).
		 */
		void setDopplerFactor(float factor=1.f);
		/** Gets doppler factor.
		@remarks
			Gets the global doppler factor which affects attenuation for all sounds
			@param factor
				Factor scale (>0).
		 */
		float getDopplerFactor();
		/** Sets speed of sound.
		@remarks
			Sets the global speed of sound used in the attenuation algorithm, affects all sounds.
			@param speed 
				Speed (m/s).
		 */
		void setSpeedOfSound(float speed=363.f);
		/** Gets speed of sound.
		@remarks
			Gets the global speed of sound used in the attenuation algorithm which affects all sounds.
			@param speed
				Speed (m/s).
		 */
		float getSpeedOfSound();
		/** Fades master volume in/out
		@remarks
			Allows fading of in/out of alls sounds
		 */
		void fadeMasterVolume(float time, bool fadeIn);
		/** Gets a list of device strings
		@remarks
			Creates a list of available audio device strings
		 */
		const Ogre::StringVector getDeviceList() const;
		/** Returns pointer to listener.
		 */
		OgreOggListener* getListener() { return mListener; }
		/** Returns number of sources created.
		 */
		int getNumSources() const { return mSources.size(); }
		/** Updates system.
		@remarks
			Iterates all sounds and updates them.
			@param fTime 
				Elapsed frametime.
		 */
		void update(float fTime=0.f);
		/** Sets a resource group name to search for all sounds first.
		@remarks
			A speed improvement to skip the cost of searching all resource locations/groups when creating sounds.\n
			Will default to searching all groups if sound is not found.
			@param group
				Name of OGRE ResourceGroup.
		 */
		void setResourceGroupName(const Ogre::String& group);
		/** Returns user defined search group name
		 */
		Ogre::String getResourceGroupName() const;
#if HAVE_EFX
#	if HAVE_EFX == 1
		/** Returns XRAM support status.
		 */
		bool hasXRamSupport() { return mXRamSupport; }
#	endif
		/** Returns EFX support status.
		 */
		bool hasEFXSupport() { return mEFXSupport; }
		/** Returns EAX support status.
		 */
		bool hasEAXSupport() { return mEAXSupport; }
#	if HAVE_EFX == 1
		/** Sets XRam buffers.
		@remarks
			Currently defaults to AL_STORAGE_AUTO.
		 */
		void setXRamBuffer(ALsizei numBuffers, ALuint* buffers);
		/** Sets XRam buffers storage mode.
		@remarks
			Should be called before creating any sounds
			Options: AL_STORAGE_AUTOMATIC | AL_STORAGE_HARDWARE | AL_STORAGE_ACCESSIBLE
		 */
		void setXRamBufferMode(ALenum mode);
#	endif
		/** Sets the distance units of measurement for EFX effects.
		@remarks
			@param unit 
				Units (meters).
		 */
		bool setEFXDistanceUnits(float unit=3.3f);
		/** Creates a specified EFX filter
		@remarks
			Creates a specified EFX filter if hardware supports it.
			@param eName
				name for the filter.
			@param type
				Possible types: AL_FILTER_LOWPASS, AL_FILTER_HIGHPASS, AL_FILTER_BANDPASS.
			@param gain
				Gain of the allowed frequency band. Range: [0.0, 1.0]
			@param hfGain
				Desired gain for filtered high frequencies (only affects lowpass and bandpass filters). Range: [0.0, 1.0]
			@param lfGain
				Desired gain for filtered low frequencies (only affects highpass and bandpass filters). Range: [0.0, 1.0]
		 */
		bool createEFXFilter(const Ogre::String& eName, ALint type, ALfloat gain=1.0, ALfloat hfGain=1.0, ALfloat lfGain=1.0);
		/** Creates a specified EFX effect
		@remarks
			Creates a specified EFX effect if the hardware (or software in the case of OpenAL Soft) supports it.\n
			Optional reverb preset structure can be passed which will be applied to the effect.\n
			See eax-util.h (efx-presets.h for OpenAL Soft) for a list of presets.
			@param eName
				Name for effect.
			@param type
				See OpenAL docs for available effects.
			@param props
				Legacy structure describing a preset reverb effect.
		 */
#	if HAVE_EFX == 1
		bool createEFXEffect(const Ogre::String& eName, ALint type, EAXREVERBPROPERTIES* props=0);
#	elif HAVE_EFX == 2
		bool createEFXEffect(const Ogre::String& eName, ALint type, EFXEAXREVERBPROPERTIES* props=0);
#	endif
		/** Sets extended properties on a specified sounds source
		@remarks
			Tries to set EFX extended source properties.
			@param sName
				Name of sound.
			@param airAbsorption
				Absorption factor for air.
			@param roomRolloff
				Room rolloff factor.
			@param coneOuterHF
				Cone outer gain factor for High frequencies.
		 */
		bool setEFXSoundProperties(const Ogre::String& sName, float airAbsorption=0.f, float roomRolloff=0.f, float coneOuterHF=0.f);
		/** Sets a specified paremeter on an effect
		@remarks
			Tries to set a parameter value on a specified effect.
			Returns true/false.
			@param eName
				Name of effect.
			@param effectType
				See OpenAL docs for available effects.
			@param attrib
				Parameter value to alter.
			@param param
				Float value to set.
		 */
		bool setEFXEffectParameter(const Ogre::String& eName, ALint effectType, ALenum attrib, ALfloat param);
		/** Sets a specified paremeter on an effect
		@remarks
			Tries to set a parameter value on a specified effect.
			Returns true/false.
			@param eName 
				Name of effect.
			@param type 
				See OpenAL docs for available effects.
			@param attrib 
				Parameter value to alter.
			@param params 
				Vector pointer of float values to set.
		 */
		bool setEFXEffectParameter(const Ogre::String& eName, ALint type, ALenum attrib, ALfloat* params=0);
		/** Sets a specified paremeter on an effect
		@remarks
			Tries to set a parameter value on a specified effect.
			Returns true/false.
			@param eName 
				Name of effect.
			@param type 
				See OpenAL docs for available effects.
			@param attrib 
				Parameter value to alter.
			@param param 
				Integer value to set.
		 */
		bool setEFXEffectParameter(const Ogre::String& eName, ALint type, ALenum attrib, ALint param);
		/** Sets a specified paremeter on an effect
		@remarks
			Tries to set a parameter value on a specified effect.
			Returns true/false.
			@param eName 
				Name of effect.
			@param type 
				See OpenAL docs for available effects.
			@param attrib 
				Parameter value to alter.
			@param params 
				Vector pointer of integer values to set.
		 */
		bool setEFXEffectParameter(const Ogre::String& eName, ALint type, ALenum attrib, ALint* params=0);
		/** Gets the maximum number of Auxiliary Effect Slots detected for the OpenAL device on initialization
		@remarks
			Returns how many simultaneous effects can be applied at the same time to the Output Mix.
		 */
		int getMaxAuxiliaryEffectSlots() { return mNumEffectSlots; };
		/** Gets the maximum number of Auxiliary Effect Sends per source
		@remarks
			Returns how many simultaneous effects can be applied to any one source object.
		 */
		int getMaxAuxiliaryEffectSends() { return mNumSendsPerSource; };
		/** Gets the number of currently created Auxiliary Effect slots
		@remarks
			Returns number of slots created and available for effects/filters.
		 */
		int getNumberOfCreatedEffectSlots();
		/** Creates an Auxiliary Effect slot
		@remarks
			Creates an Auxiliary Effect slot if the hardware (or software in the case of OpenAL Soft) supports it.
		 */
		bool createEFXSlot();
		/** Sets the gain of a specific Auxiliary Effect slot
		@remarks
			Allows the application to control the output level of an Auxiliary Effect Slot.
			@param slotID
				ID of the Auxiliary Effect slot
			@param gain
				Output level for the Auxiliary Effect Slot (a value between 0 and 1).
		 */
		bool setEFXSlotGain(ALuint slotID, float gain);
		/** Gets the gain of a specific Auxiliary Effect slot
		@remarks
			Retrieves the value of the gain that was set for the Auxiliary Effect Slot.
			@param slotID
				ID of the Auxiliary Effect slot
		@note
			Returns -1 if there is an error.
		 */
		float getEFXSlotGain(ALuint slotID);
		/** Attaches a created effect to an Auxiliary Effect slot. (Does nothing without EAX or EFX support)
			@param slotID
				ID of the Auxiliary Effect slot to where the effect will be attached
			@param effect
				Name of the effect to attach to the slot
		 */
		bool attachEffectToSlot(const Ogre::String& effect, ALuint slotID);
		/** Detaches an effect from an Auxiliary Effect slot
			@param slotID
				ID of the Auxiliary Effect slot from where the effect should be detached
		 */
		bool detachEffectFromSlot(ALuint slotID);
		/** Attaches an Auxiliary Effect slot to a sound
		@remarks
			Currently sound must have a source attached prior to this call.
			@param sName 
				Name of sound
			@param send
				Number of the Auxiliary Effect send where the Auxiliary Effect slot should be attached to (0 ... Max Sends)
			@param slotID
				ID of the Auxiliary Effect slot to attach to the Auxiliary Effect send
			@param filter
				Name of filter to attach to the send path (wet signal)
		 */
		bool attachEffectToSound(const Ogre::String& sName, ALuint send, ALuint slotID, const Ogre::String& filter="");
		/** Detaches an Auxiliary Effect slot from sound
		@remarks
			Currently sound must have a source attached prior to this call.
			@param sName
				Name of sound
			@param send
				Number of the Auxiliary Effect send where the Auxiliary Effect slot was attached to
		 */
		bool detachEffectFromSound(const Ogre::String& sName, ALuint send);
		/** Attaches a filter to the direct path (dry signal) of a sound
		@remarks
			Currently sound must have a source attached prior to this call.
			@param sName
				Name of sound
			@param filter
				Name of filter as defined when created
		 */
		bool attachFilterToSound(const Ogre::String& sName, const Ogre::String& filter="");
		/** Detaches all filters from the direct path (dry signal) of a sound
		@remarks
			Currently sound must have a source attached prior to this call.
			@param sName
				Name of sound
		 */
		bool detachFilterFromSound(const Ogre::String& sName);
#if HAVE_ALEXT == 1
		/** Concatenates the output of a specified Auxiliary Effect slot to the input of another
		@remarks
			This function provides a method to reroute the output of an auxiliary effect slot to the input of another auxiliary effect slot.\n
			By default, an effect slot's output is added to the main output along side other effect slots and each source's direct path.\n
			This makes it impossible to, for example, apply an equalizer effect to the output of a chorus effect since the chorus and equalizer effects are processed separately.\n
			Retargeting an effect slot's output to another effect slot allows chaining multiple effects to create results that aren't possible with standard EFX.
			@param srcSlotID
				Slot ID of the source Auxiliary Effect effect slot.
			@param dstSlotID
				Slot ID of destination Auxiliary Effect effect slot.
		@note
			This function uses the extension AL_SOFT_effect_target which is only available through OpenAL Soft.\n
			Each effect slot can only have one target (though an effect slot can act as the target for multiple other effect slots and sources).\n
			It's an error to create a circular chain.
		 */
		bool concatenateEFXEffectSlots(ALuint srcSlotID, ALuint dstSlotID);
		/** Helper function used to release Auxiliary Effect Slots
		@remarks
			When two Auxiliary Effect Slots are concatenated by concatenateEFXEffectSlots() the order of slot deletion becomes important.\n
			To solve this a Multi Map registers the concatenated pairs, which end up forming a forest of trees.\n
			The deletion of the Auxiliary Effect Slots should begin with the tree leaves, to delete in that order we use DFS (Depth First Search).\n
			This function implements the algorithm recursively.
			@param slot
				Slot to process
			@param effectSlotMultiMap
				Multi Map that containts the source slot <---> dest slot pairs
		@note
			This function is for internal use only.
		 */
		void _delConcatenatedEFXEffectSlots(ALuint slot, std::multimap<ALuint, ALuint> *effectSlotMultiMap);
#endif
		/** Sets extended properties on a specified sounds source
		@remarks
			Tries to set EFX extended source properties.
			@param sound
				Name of sound.
			@param airAbsorption
				Absorption factor for air.
			@param roomRolloff
				Room rolloff factor.
			@param coneOuterHF
				Cone outer gain factor for High frequencies.
		 */
		bool _setEFXSoundPropertiesImpl(OgreOggISound* sound=0, float airAbsorption=0.f, float roomRolloff=0.f, float coneOuterHF=0.f);
		/** Attaches an effect to a sound
		@remarks
			Currently sound must have a source attached prior to this call.\n
			It is expected that an Effect has already been attached to the Auxiliary Effect slot.
			@param sound
				Sound pointer
			@param send
				Number of the Auxiliary Effect send where the Auxiliary Effect slot should be attached to (0 ... Max Sends)
			@param slotID
				ID of the Auxiliary Effect slot to attach to the Auxiliary Effect send
			@param filter
				Name of filter to attach to the send path (wet signal)
		 */
		bool _attachEffectToSoundImpl(OgreOggISound* sound, ALuint send, ALuint slotID, const Ogre::String& filter);
		/** Detaches all effects from a sound
		@remarks
			Currently sound must have a source attached prior to this call.
			@param sound
				Sound pointer
			@param slotID
				Slot ID
		 */
		bool _detachEffectFromSoundImpl(OgreOggISound* sound=0, ALuint slotID=255);
		/** Attaches a filter to a sound
		@remarks
			Currently sound must have a source attached prior to this call.
			@param sound
				Sound pointer
			@param filter
				Name of filter as defined when created
		 */
		bool _attachFilterToSoundImpl(OgreOggISound* sound=0, const Ogre::String& filter="");
		/** Detaches all filters from a sound
		@remarks
			Currently sound must have a source attached prior to this call.
			@param sound
				Sound pointer
		 */
		bool _detachFilterFromSoundImpl(OgreOggISound* sound=0);
		/** Returns whether a specified effect is supported
			@param effectID
				OpenAL effect/filter id. (AL_EFFECT... | AL_FILTER...)
		 */
		bool isEffectSupported(ALint effectID);
#endif

		/** Creates recording class to manage device recording
		 */
		OgreOggSoundRecord* createRecorder();
		/** Gets recording class created to manage device recording
		 */
		OgreOggSoundRecord* getRecorder() { return mRecorder; }

		/** Gets a list of strings with the names of the available capture devices
		*/
		const RecordDeviceList& getCaptureDeviceList();

		/// @copydoc Ogre::Singleton::getSingleton()
		static OgreOggSoundManager& getSingleton(); 

		/// @copydoc Ogre::Singleton::getSingletonPtr()
		static OgreOggSoundManager* getSingletonPtr();

#if OGGSOUND_THREADED
		OGRE_WQ_MUTEX(mMutex);
		OGRE_WQ_MUTEX(mSoundMutex);
		OGRE_WQ_MUTEX(mResourceGroupNameMutex);

		/** Pushes a sound action request onto the queue
		@remarks
			Sound actions are queued through the manager to be operated on in an efficient and
			non-blocking manner, this function adds a request to the list to be processed.
			@param sound
				Sound object to perform action upon
			@param action
				Action to perform.
		@note
			Internal function - SHOULD NOT BE CALLED BY USER CODE.\n
		*/
		void _requestSoundAction(const SoundAction& action);
		/** Sets the mForceMutex flag for switching between non-blocking/blocking action calls.
		@remarks
			@param on
				Flag indicating status of mForceMutex var.
		*/
		inline void setForceMutex(bool on) { mForceMutex=on; }
#endif
 
	private:
		LocklessQueue<OgreOggISound*>* mSoundsToDestroy;

#if OGGSOUND_THREADED
		/** Processes queued sound actions.
		@remarks
			Presently executes a maximum of 5 actions in a single iteration.
		 */
		void _processQueuedSounds(void);
		/** Updates all sound buffers.
		@remarks
			Iterates all sounds and updates their buffers.
		 */
		void _updateBuffers();

		LocklessQueue<SoundAction>* mActionsList;

		OGRE_THREAD_TYPE* mUpdateThread;
		static bool mShuttingDown;

		/** Flag indicating that a mutex should be used whenever an action is requested.
		@remarks
			In certain instances user may require that an action is performed inline,
			rather then the default: queued and performed asynchronously.\n
			This global flag will affect all subsequent asynchronous calls to execute immediately, 
			with the disadvantage that it will block the main thread. 
		@note
			This doesn't affect buffer updates, which will still be handled asynchronously.
		*/
		bool mForceMutex;			 
									
		/** Performs a requested action.
		@param act
			Action struct describing action to perform
		 */
		void _performAction(const SoundAction& act);

		/** Threaded function for streaming updates
		@remarks
			Optional threading function specified in OgreOggPreReqs.h.\n
			Implemented to handle updating of streamed audio buffers independently of main game thread, 
			unthreaded streaming would be disrupted by any pauses or large frame lags, 
			due to the fact that OpenAL itself runs in its own thread.\n
			If the audio buffers aren't constantly re-filled the sound will be automatically stopped by OpenAL.\n
			Static sounds do not suffer this problem because all the audio data is preloaded into memory.
		 */
		static void threadUpdate()
		{
			while(!mShuttingDown)
			{	
				{
					OGRE_WQ_LOCK_MUTEX(OgreOggSoundManager::getSingletonPtr()->mMutex);
					OgreOggSoundManager::getSingletonPtr()->_updateBuffers();
					OgreOggSoundManager::getSingletonPtr()->_processQueuedSounds();
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
#endif
		/** Creates a single sound object (implementation).
		@remarks
			Creates and inits a single sound object, depending on passed
			parameters this function will create a static/streamed sound.\n
			Each sound must have a unique name within the manager.
			@param name 
				Unique name of sound
			@param file 
				Audio file path string or "BUFFER" for memory buffer sound (@ref OgreOggStreamBufferSound)
			@param stream 
				Flag indicating if the sound sound be streamed.
			@param loop 
				Flag indicating if the file should loop.
			@param preBuffer 
				Flag indicating if a source should be attached at creation.
			@param immediate
				Optional flag to indicate creation should occur immediately and not be passed to background thread for queueing.\n
				Can be used to overcome the random creation time which might not be acceptable (MULTI-THREADED ONLY)
		 */
		
		OgreOggISound* _createSoundImpl(
			const Ogre::String& name,
			#if OGRE_VERSION_MAJOR == 2
			Ogre::IdType id,
			#endif
			const Ogre::String& file,
			bool stream    = false,
			bool loop      = false,
			bool preBuffer = false,
			bool immediate = false
		);
		
		/** Implementation of sound loading
		@param sound
			Sound pointer.
		@param file
			Name of sound file.
		@param prebuffer
			Prebuffer flag.
		*/
		void _loadSoundImpl(OgreOggISound* sound, const Ogre::String& file, bool prebuffer);
		/** Destroys a single sound.
		@remarks
			Destroys a single sound object.
			@param sound
				Sound object to destroy.
		 */
		void _destroySoundImpl(OgreOggISound* sound=0);
		/** Release sound source and remove sound
		@remarks
			Releases sound source and removes sound from OgreOggSoundManager internal sound registry.\n
			So another sound with the same name can be created next time.
			@param sound
				Sound to destroy.
		 */
		void _releaseSoundImpl(OgreOggISound* sound=0);
		/** Removes references of a sound from all possible internal lists.
		@remarks
			Various lists exist to manage numerous states of a sound, this
			function exists to remove a sound object from any/all lists it has
			previously been added to. 
			@param sound
				Sound to destroy.
		 */
		void _removeFromLists(OgreOggISound* sound=0);
		/** Stops all currently playing sounds.
		 */
		void _stopAllSoundsImpl();
		/** Applys global pitch.
		 */
		void _setGlobalPitchImpl();
		/** Pauses all currently playing sounds.
		 */
		void _pauseAllSoundsImpl();
		/** Resumes all previously playing sounds.
		 */
		void _resumeAllPausedSoundsImpl();
		/** Destroys all sounds.
		 */
		void _destroyAllSoundsImpl();
		/** Creates a pool of OpenAL sources for playback.
		@remarks
			Attempts to create a pool of source objects which allow simultaneous audio playback.\n
			The number of sources will be clamped to either the hardware maximum or [mMaxSources], whichever comes first.
		 */
		int _createSourcePool();
		/** Gets a shared audio buffer
		@remarks
			Returns a previously loaded shared buffer reference if available.\n
			@param sName
				Name of audio file
		@note
			Increments a reference count so releaseSharedBuffer() must be called when buffer is no longer used.
		 */
		sharedAudioBuffer* _getSharedBuffer(const Ogre::String& sName);
		/** Opens the specified file as a new data stream.
			@param file
				The path to the resource file to open.
		 */
		Ogre::DataStreamPtr _openStream(const Ogre::String& file) const;
		/** Releases all sounds and buffers
		@remarks
			Release all sounds and their associated OpenAL objects from the system.
		 */
		void _releaseAll();
		/** Checks and Logs a list of supported features
		@remarks
			Queries OpenAL for various supported features and lists them with the LogManager.
		 */
		void _checkFeatureSupport();
		/** Checks and Logs a list of supported extensions
		@remarks
			Queries OpenAL for the supported extentions and lists them with the LogManager.
		 */
		void _checkExtensionSupport();
#if HAVE_EFX
		/** Checks for EFX hardware support
		 */
		bool _checkEFXSupport();
#if HAVE_EFX == 1
		/** Checks for XRAM hardware support
		 */
		bool _checkXRAMSupport();
#endif
		/** Checks for EAX effect support
		 */
		void _determineAuxEffectSlots();
		/** Gets a specified EFX filter
		@param fName 
			Name of filter as defined when created.
		 */
		ALuint _getEFXFilter(const Ogre::String& fName);
		/** Gets a specified EFX Effect
		@param eName 
			Name of effect as defined when created.
		 */
		ALuint _getEFXEffect(const Ogre::String& eName);
		/** Gets a specified EFX Effect slot
		@param slotID 
			Index of auxiliary effect slot
		 */
		ALuint _getEFXSlot(int slotID=0);
		/** Sets EAX reverb properties using a specified present
		@param pEFXEAXReverb 
			Pointer to converted EFXEAXREVERBPROPERTIES structure object
		@param uiEffect 
			Effect ID
		 */
		bool _setEAXReverbProperties(EFXEAXREVERBPROPERTIES *pEFXEAXReverb, ALuint uiEffect);
#endif
		/** Re-activates any sounds which had their source stolen.
		 */
		void _reactivateQueuedSounds();
		/** Re-activates any sounds which had their source stolen, implementation methods.
		@remarks
			When all sources are in use, the sounds begin to give up their source objects to higher priority sounds.\n
			When this happens the lower priority sound is queued ready to re-play when a source becomes available again, 
			this function checks this queue and tries to re-play those sounds.\n
			Only affects sounds which were originally playing when forced to give up their source object.
		 */
		void _reactivateQueuedSoundsImpl();
		/** Enumerates audio devices.
		@remarks
			Gets a list of audio devices available.
		 */
		void _enumDevices();
		/** Creates a listener object.
		 */
		OgreOggListener* _createListener();
		/** Destroys a listener object.
		 */
		void _destroyListener();

		/** Calculates the distance a sound is from the specified listener position.
		 */
		static Ogre::Real _calculateDistanceToListener(OgreOggISound * sound, const Ogre::Vector3 & listenerPos);

		/**
		 * OpenAL device objects
		 */
		ALCdevice* mDevice;						// OpenAL device
		ALCcontext* mContext;					// OpenAL context

		ALfloat	mOrigVolume;					// Used to revert volume after a mute

		/** Sound lists
		 */
		SoundMap mSoundMap;						// Map of all sounds
		ActiveList mActiveSounds;				// list of sounds currently active
		ActiveList mPausedSounds;				// list of sounds currently paused
		ActiveList mSoundsToReactivate;			// list of sounds that need re-activating when sources become available
		ActiveList mWaitingSounds;				// list of sounds that need playing when sources become available
		SourceList mSourcePool;					// List of available sources
		FeatureList mEFXSupportList;			// List of supported EFX effects by OpenAL ID
		SharedBufferList mSharedBuffers;		// List of shared static buffers

		/** Fading vars
		*/																  
		Ogre::Real mFadeTime;					// Time over which to fade
		Ogre::Real mFadeTimer;					// Timer for fade
		bool mFadeIn;							// Direction fade in/out
		bool mFadeVolume;						// Flag for fading

		ALCchar* mDeviceStrings;				// List of available devices strings
		SourceList mSources;					// List of created sources at initialisation
		unsigned int mMaxSources;				// Maximum Number of sources to allocate

		float mGlobalPitch;						// Global pitch modifier

		OgreOggSoundRecord* mRecorder;			// Recorder object
		RecordDeviceList	mRecordDeviceList;	// List of available capture devices

		//! sorts sound list by distance
		struct _sortNearToFar;
		//! sorts sound list by distance
		struct _sortFarToNear;

#if HAVE_EFX
		/**	EFX Support
		*/
		bool mEFXSupport;						// EFX present flag

		// Effect objects
		LPALGENEFFECTS alGenEffects;
		LPALDELETEEFFECTS alDeleteEffects;
		LPALISEFFECT alIsEffect;
		LPALEFFECTI alEffecti;
		LPALEFFECTIV alEffectiv;
		LPALEFFECTF alEffectf;
		LPALEFFECTFV alEffectfv;
		LPALGETEFFECTI alGetEffecti;
		LPALGETEFFECTIV alGetEffectiv;
		LPALGETEFFECTF alGetEffectf;
		LPALGETEFFECTFV alGetEffectfv;

		//Filter objects
		LPALGENFILTERS alGenFilters;
		LPALDELETEFILTERS alDeleteFilters;
		LPALISFILTER alIsFilter;
		LPALFILTERI alFilteri;
		LPALFILTERIV alFilteriv;
		LPALFILTERF alFilterf;
		LPALFILTERFV alFilterfv;
		LPALGETFILTERI alGetFilteri;
		LPALGETFILTERIV alGetFilteriv;
		LPALGETFILTERF alGetFilterf;
		LPALGETFILTERFV alGetFilterfv;

		// Auxiliary slot object
		LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
		LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
		LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot;
		LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;
		LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv;
		LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf;
		LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv;
		LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti;
		LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv;
		LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf;
		LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv;

#	if HAVE_EFX == 1
		/**	XRAM Support
		*/
		typedef ALboolean (__cdecl *LPEAXSETBUFFERMODE)(ALsizei n, ALuint *buffers, ALint value);
		typedef ALenum    (__cdecl *LPEAXGETBUFFERMODE)(ALuint buffer, ALint *value);

		LPEAXSETBUFFERMODE mEAXSetBufferMode;
		LPEAXGETBUFFERMODE mEAXGetBufferMode;
#	endif

		/**	EAX Support
		*/
		bool mEAXSupport;						// EAX present flag
		int mEAXVersion;						// EAX version ID

		bool mXRamSupport;

		FilterList mFilterList;					// List of EFX filters
		EffectList mEffectList;					// List of EFX effects
		EffectSlotList mEffectSlotList;				// List of EFX effect slots
		SlotMultiMap mEffectSlotMultiMap;		// Multi Map of EFX effect slots (used for effect slot concatenation)

		ALint mNumEffectSlots;					// Number of effect slots available
		ALint mNumSendsPerSource;				// Number of aux sends per source

		ALenum	mXRamSize,
				mXRamFree,
				mXRamAuto,
				mXRamHardware,
				mXRamAccessible,
				mCurrentXRamMode;

		ALint	mXRamSizeMB,
				mXRamFreeMB;
#endif

		Ogre::String mResourceGroupName;		// Resource group name to search for all sounds

		Ogre::SceneManager* mSceneMgr;			// Default SceneManager to use to create sound objects

		/**	Listener pointer
		 */
		OgreOggListener *mListener;				// Listener object

		friend class OgreOggSoundFactory;
		friend class OgreOggISound;
	};
}

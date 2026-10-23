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

#include "OgreOggStaticSound.h"
#include <string>
#include <iostream>
#include "OgreOggSound.h"
#include "stb_vorbis.h"

namespace OgreOggSound
{
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStaticSound::OgreOggStaticSound(
		const Ogre::String& name
		#if AV_OGRE_NEXT_VERSION >= 0x20000
		, Ogre::SceneManager* scnMgr, Ogre::IdType id, Ogre::ObjectMemoryManager *objMemMgr, Ogre::uint8 renderQueueId
		#endif
	) : OgreOggISound(
		name
		#if AV_OGRE_NEXT_VERSION >= 0x20000
		, scnMgr, id, objMemMgr, renderQueueId
		#endif
	)
	,mOggStream(nullptr)
	,mSampleRate(0)
	,mChannels(0)
	,mPreviousOffset(0)
	,mAudioName("")
	{
		mStream=false;														
		mBuffers.reset(new BufferList(1, AL_NONE));
	}
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStaticSound::~OgreOggStaticSound()
	{		
		// Notify listener
		if ( mSoundListener ) mSoundListener->soundDestroyed(this);

		_release();
		mBufferData.clear();
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::_openImpl(Ogre::DataStreamPtr& fileStream)
	{
		// Store stream pointer
		mAudioStream = fileStream;

		// Store file name
		mAudioName = mAudioStream->getName();

		// stb_vorbis has no custom IO callbacks; give it the whole stream.
		_readAll(mAudioStream, mOggData);
		int error = 0;
		mOggStream = stb_vorbis_open_memory(mOggData.data(), static_cast<int>(mOggData.size()), &error, nullptr);
		if ( !mOggStream )
		{
			mOggData.clear();
			OGRE_EXCEPT(Ogre::Exception::ERR_FILE_NOT_FOUND, "Could not open Ogg stream (stb_vorbis error " +
				Ogre::StringConverter::toString(error) + ").", "OgreOggStaticSound::_openImpl()");
			return;
		}

		const stb_vorbis_info info = stb_vorbis_get_info(mOggStream);
		mSampleRate = info.sample_rate;
		mChannels = info.channels;

		// Get playtime in secs
		mPlayTime = static_cast<float>(stb_vorbis_stream_length_in_seconds(mOggStream));

		const unsigned int bitrate = mPlayTime > 0.f
			? static_cast<unsigned int>((mOggData.size() * 8.f) / mPlayTime)
			: 0;

		Ogre::LogManager::getSingleton().logMessage("Sound '" + mAudioName + "': Loading OGG - " +
			Ogre::StringConverter::toString(mChannels) + " channels, " +
			Ogre::StringConverter::toString(mSampleRate) + " Hz, " +
			Ogre::StringConverter::toString(bitrate) + " bps");

		// Check format support
		if (!_queryBufferInfo())
		{
			stb_vorbis_close(mOggStream);
			mOggStream = nullptr;
			mOggData.clear();
			OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, "Format NOT supported!", "OgreOggStaticSound::_openImpl()");
		}

		alGenBuffers(1, &(*mBuffers)[0]);

		char* data;
		int sizeRead = 0;

		data = OGRE_ALLOC_T(char, mBufferSize, Ogre::MEMCATEGORY_GENERAL);
		do
		{
			const int frames = stb_vorbis_get_samples_short_interleaved(
				mOggStream, mChannels, reinterpret_cast<short*>(data),
				static_cast<int>(mBufferSize / sizeof(short)));
			sizeRead = frames * mChannels * static_cast<int>(sizeof(short));
			if (sizeRead > 0)
				mBufferData.insert(mBufferData.end(), data, data + sizeRead);
		}
		while(sizeRead > 0);
		OGRE_FREE(data, Ogre::MEMCATEGORY_GENERAL);

		// Static sounds keep the decoded PCM only.
		stb_vorbis_close(mOggStream);
		mOggStream = nullptr;
		mOggData.clear();

#if OGGSOUND_HAVE_EFX == 1
		// Upload to XRAM buffers if available
		if ( OgreOggSoundManager::getSingleton().hasXRamSupport() )
			OgreOggSoundManager::getSingleton().setXRamBuffer(1, &(*mBuffers)[0]);
#endif

		alGetError();
		alBufferData((*mBuffers)[0], mFormat, &mBufferData[0], static_cast<ALsizei>(mBufferData.size()), mSampleRate);
		if ( alGetError()!=AL_NO_ERROR )
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, "Unable to load audio data into buffer.", "OgreOggStaticSound::_openImpl()");
			return;
		}

		// Register shared buffer
		OgreOggSoundManager::getSingleton()._registerSharedBuffer(mAudioName, (*mBuffers)[0], this);

		// Notify listener
		if (mSoundListener) mSoundListener->soundLoaded(this);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::_openImpl(const Ogre::String& fName, sharedAudioBuffer* buffer)
	{
		if ( !buffer ) return;

		// Set buffer
		_setSharedProperties(buffer);

		// Filename
		mAudioName = fName;

		// Notify listener
		if (mSoundListener) mSoundListener->soundLoaded(this);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::_release()
	{
		setSource(AL_NONE);
		OgreOggSoundManager::getSingleton()._releaseSharedBuffer(mAudioName, (*mBuffers)[0]);
		if ( mOggStream )
		{
			stb_vorbis_close(mOggStream);
			mOggStream = nullptr;
		}
		mOggData.clear();
		mPlayPosChanged = false;
		mPlayPos = 0.f;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::_prebuffer()
	{
		if (mSource==AL_NONE) return;

		// Queue buffer
		alSourcei(mSource, AL_BUFFER, (*mBuffers)[0]);
	}
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggStaticSound::isMono()
	{
		if ( !mInitialised ) return false;

		return ( (mFormat==AL_FORMAT_MONO16) || (mFormat==AL_FORMAT_MONO8) );
	}
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggStaticSound::_queryBufferInfo()
	{
		if (mChannels <= 0)
		{
			Ogre::LogManager::getSingleton().logMessage("No vorbis info!");
			return false;
		}

		switch(mChannels)
		{
		case 1:
			{
				mFormat = AL_FORMAT_MONO16;
				// Set BufferSize to 250ms (Frequency * 2 (16bit) divided by 4 (quarter of a second))
				mBufferSize = mSampleRate >> 1;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 2);
			}
			break;
		case 2:
			{
				mFormat = AL_FORMAT_STEREO16;
				// Set BufferSize to 250ms (Frequency * 4 (16bit stereo) divided by 4 (quarter of a second))
				mBufferSize = mSampleRate;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 4);
			}
			break;
		case 4:
			{
				mFormat = _queryFormat("AL_FORMAT_QUAD16");
				if (!mFormat) return false;
				// Set BufferSize to 250ms (Frequency * 8 (16bit 4-channel) divided by 4 (quarter of a second))
				mBufferSize = mSampleRate * 2;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 8);
			}
			break;
		case 6:
			{
				mFormat = _queryFormat("AL_FORMAT_51CHN16");
				if (!mFormat) return false;
				// Set BufferSize to 250ms (Frequency * 12 (16bit 6-channel) divided by 4 (quarter of a second))
				mBufferSize = mSampleRate * 3;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 12);
			}
			break;
		case 7:
			{
				mFormat = _queryFormat("AL_FORMAT_61CHN16");
				if (!mFormat) return false;
				// Set BufferSize to 250ms (Frequency * 16 (16bit 7-channel) divided by 4 (quarter of a second))
				mBufferSize = mSampleRate * 4;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 16);
			}
			break;
		case 8:
			{
				mFormat = _queryFormat("AL_FORMAT_71CHN16");
				if (!mFormat) return false;
				// Set BufferSize to 250ms (Frequency * 20 (16bit 8-channel) divided by 4 (quarter of a second))
				mBufferSize = mSampleRate * 5;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 20);
			}
			break;
		default:
			// Couldn't determine buffer format so log the error and default to mono
			Ogre::LogManager::getSingleton().logMessage("!!WARNING!! Could not determine buffer format! Defaulting to MONO");

			mFormat = AL_FORMAT_MONO16;
			// Set BufferSize to 250ms (Frequency * 2 (16bit) divided by 4 (quarter of a second))
			mBufferSize = mSampleRate >> 1;
			// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
			mBufferSize -= (mBufferSize % 2);
			break;
		}
		return true;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::setSource(ALuint src)
	{
		if (src!=AL_NONE)
		{
			// Attach new source
			mSource=src;

			// Load audio data onto source
			_prebuffer();

			// Init source properties
			_initSource();
		}
		else
		{
			// Validity check
			if ( mSource!=AL_NONE )
			{
				// Need to stop sound BEFORE unqueuing
				alSourceStop(mSource);

				// Unqueue buffer
				alSourcei(mSource, AL_BUFFER, 0);
			}

			// Attach new source
			mSource=src;

			// Cancel initialisation
			mInitialised = false;
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::_pauseImpl()
	{
		assert(mState != SS_DESTROYED);

		if ( mSource==AL_NONE ) return;

		alSourcePause(mSource);
		mState = SS_PAUSED;

		// Notify listener
		if (mSoundListener) 
			mSoundListener->soundPaused(this);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::_playImpl()
	{
		assert(mState != SS_DESTROYED);

		if(isPlaying())
			return;

		if (mSource == AL_NONE)
			if ( !OgreOggSoundManager::getSingleton()._requestSoundSource(this) )
				return;

		// Pick up playback position change..
		if ( mPlayPosChanged )
			setPlayPosition(mPlayPos);

		alSourcePlay(mSource);
		mState = SS_PLAYING;

		// Notify listener
		if (mSoundListener) 
			mSoundListener->soundPlayed(this);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::_stopImpl()
	{
		assert(mState != SS_DESTROYED);

		if ( mSource==AL_NONE || isStopped() ) return;

		alSourceStop(mSource);
		alSourceRewind(mSource);
		mState = SS_STOPPED;
		mPreviousOffset=0;

		// Notify listener
		if (mSoundListener) mSoundListener->soundStopped(this);

		// Mark for destruction
		if (mTemporary)
		{
			mState = SS_DESTROYED;
			OgreOggSoundManager::getSingletonPtr()->_destroyTemporarySound(this);
		}
		// Give up source immediately if specfied
		else if (mGiveUpSource) 
			OgreOggSoundManager::getSingleton()._releaseSoundSource(this);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::loop(bool loop)
	{
		mLoop = loop;

		if(mSource != AL_NONE)
		{
			alSourcei(mSource, AL_LOOPING, loop);

			if ( alGetError() != AL_NO_ERROR )
				OGRE_LOG_ERROR("OgreOggStaticSound::loop() - Unable to set looping status!");
		}
		else
			Ogre::LogManager::getSingleton().logMessage("OgreOggStaticSound::loop() - No source attached to sound!");
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::_updateAudioBuffers()
	{
		if (!isPlaying())
			return;

		ALenum state;
		alGetSourcei(mSource, AL_SOURCE_STATE, &state);

		if (state == AL_STOPPED)
		{
			stop();

			// Finished callback
			if ( mSoundListener ) 
				mSoundListener->soundFinished(this);
		}
		else
		{
			ALint bytes=0;

			// Use byte offset to work out current position
			alGetSourcei(mSource, AL_BYTE_OFFSET, &bytes);

			// Has the audio looped?
			if ( mPreviousOffset>bytes )
			{
				// Notify listener
				if (mSoundListener) 
					mSoundListener->soundLooping(this);
			}

			// Store current offset position
			mPreviousOffset=bytes;
		}
	}
}

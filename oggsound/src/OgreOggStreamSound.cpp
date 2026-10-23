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

#include "OgreOggStreamSound.h"
#include <string>
#include <iostream>
#include "OgreOggSoundManager.h"
#include "stb_vorbis.h"

namespace OgreOggSound
{
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStreamSound::OgreOggStreamSound(
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
	,mStreamEOF(false)
	,mLastOffset(0.f)
	{
		mStream=true;															
		mBuffers.reset(new BufferList(NUM_BUFFERS, AL_NONE));
	}
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStreamSound::~OgreOggStreamSound()
	{	
		// Notify listener
		if ( mSoundListener ) mSoundListener->soundDestroyed(this);

		_release();
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_openImpl(Ogre::DataStreamPtr& fileStream)
	{
		// Store stream pointer
		mAudioStream = fileStream;

		// stb_vorbis has no custom IO callbacks; give it the whole stream.
		_readAll(mAudioStream, mOggData);
		int error = 0;
		mOggStream = stb_vorbis_open_memory(mOggData.data(), static_cast<int>(mOggData.size()), &error, nullptr);
		if ( !mOggStream )
		{
			mOggData.clear();
			OGRE_EXCEPT(Ogre::Exception::ERR_FILE_NOT_FOUND, "Could not open Ogg stream (stb_vorbis error " +
				Ogre::StringConverter::toString(error) + ").", "OgreOggStreamSound::_openImpl()");
			return;
		}

		mSeekable = true;

		const stb_vorbis_info info = stb_vorbis_get_info(mOggStream);
		mSampleRate = info.sample_rate;
		mChannels = info.channels;

		// Get total playtime in seconds
		mPlayTime = static_cast<float>(stb_vorbis_stream_length_in_seconds(mOggStream));

		const unsigned int bitrate = mPlayTime > 0.f
			? static_cast<unsigned int>((mOggData.size() * 8.f) / mPlayTime)
			: 0;

		Ogre::LogManager::getSingleton().logMessage("Sound '" + mAudioStream->getName() + "': Loading OGG - " +
			Ogre::StringConverter::toString(mChannels) + " channels, " +
			Ogre::StringConverter::toString(mSampleRate) + " Hz, " +
			Ogre::StringConverter::toString(bitrate) + " bps");

		// Generate audio buffers
		alGenBuffers(NUM_BUFFERS, &(*mBuffers)[0]);

			// Check format support
		if (!_queryBufferInfo())
		{
			stb_vorbis_close(mOggStream);
			mOggStream = nullptr;
			mOggData.clear();
			OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, "Format NOT supported!", "OgreOggStreamSound::_openImpl()");
		}

#if OGGSOUND_HAVE_EFX == 1
		// Upload to XRAM buffers if available
		if ( OgreOggSoundManager::getSingleton().hasXRamSupport() )
			OgreOggSoundManager::getSingleton().setXRamBuffer(NUM_BUFFERS, &(*mBuffers)[0]);
#endif

		// In case loop point set BEFORE loaded re-check here
		if ( mLoopOffset>0.f )
		{
			if ( mLoopOffset>=mPlayTime )
			{
				OGRE_LOG_ERROR("OgreOggStreamSound::open() - Loop time invalid!");
				mLoopOffset=0.f;
			}
		}
		
		// Notify listener
		if ( mSoundListener ) mSoundListener->soundLoaded(this);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_release()
	{
		setSource(AL_NONE);
		for (int i=0; i<NUM_BUFFERS; i++)
		{										   
			if ((*mBuffers)[i]!=AL_NONE)
				alDeleteBuffers(1, &(*mBuffers)[i]);
		}
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
	void OgreOggStreamSound::setLoopOffset(float startTime)
	{
		// Store requested loop point
		mLoopOffset=startTime;

		// If loaded check validity
		if ( !mAudioStream )
			if ( mLoopOffset>=mPlayTime ) 
			{
				OGRE_LOG_ERROR("OgreOggStreamSound::setLoopOffset() - Loop time invalid!");
				// Invalid - cancel loop point
				mLoopOffset=0.f;
			}
	}  
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggStreamSound::isMono()
	{
		if ( !mInitialised ) return false;

		return ( (mFormat==AL_FORMAT_MONO16) || (mFormat==AL_FORMAT_MONO8) );
	}					  
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggStreamSound::_queryBufferInfo()
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
	void OgreOggStreamSound::_prebuffer()
	{
		if (mSource==AL_NONE) return;

		int i=0;
		while ( i<NUM_BUFFERS )
		{															   
			if ( _stream((*mBuffers)[i]) )
				alSourceQueueBuffers(mSource, 1, &(*mBuffers)[i++]);
			else
				break;
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::setSource(ALuint src)
	{
		if (src!=AL_NONE)
		{
			// Set source
			mSource=src;

			// Fill data buffers
			_prebuffer();

			// Init source
			_initSource();
		}
		else
		{
			// Unqueue buffers
			_dequeue();

			// Set source
			mSource=src;

			// Cancel initialisation
			mInitialised = false;
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_updateAudioBuffers()
	{
		if (!isPlaying()) return;

		ALenum state;
		alGetSourcei(mSource, AL_SOURCE_STATE, &state);

		if (state == AL_PAUSED) return;

		// Ran out of buffer data?
		if (state == AL_STOPPED)
		{
			if(mStreamEOF)
			{
				stop();
				
				// Finished callback
				if ( mSoundListener ) 
					mSoundListener->soundFinished(this);
				
				return;
			}
			else
			{
				// Clear audio data already played...
				_dequeue();

				// Fill with next chunk of audio...
				_prebuffer();

				// Play...
				alSourcePlay(mSource);
			}
		}

		int processed;

		alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);

		while(processed--)
		{
			ALuint buffer;
			ALint size, bits, channels, freq;

			alSourceUnqueueBuffers(mSource, 1, &buffer);

			// Get buffer details
			alGetBufferi(buffer, AL_SIZE, &size);
			alGetBufferi(buffer, AL_BITS, &bits);
			alGetBufferi(buffer, AL_CHANNELS, &channels);
			alGetBufferi(buffer, AL_FREQUENCY, &freq);    

			// Update offset (in seconds)
			mLastOffset += ((ALuint)size/channels/(bits/8)) / (ALfloat)freq;
			if ( mLastOffset>=mPlayTime )
			{
				mLastOffset = mLastOffset-mPlayTime;
				
				/**	This is the closest we can get to a loop trigger.
				@remarks 
					If played data size exceeds audio data size trigger callback.
				*/
				if ( mSoundListener ) mSoundListener->soundLooping(this);
			}

			if ( _stream(buffer) ) alSourceQueueBuffers(mSource, 1, &buffer);
		}

		// handle play position change 
		if ( mPlayPosChanged ) 
		{
			_updatePlayPosition();
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	static bool seekOggStream(stb_vorbis* stream, unsigned int sampleRate, float seconds)
	{
		if (!stream) return false;
		if (seconds <= 0.f) return stb_vorbis_seek_start(stream) != 0;
		const unsigned int sample = static_cast<unsigned int>(seconds * sampleRate);
		return stb_vorbis_seek(stream, sample) != 0;
	}
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggStreamSound::_stream(ALuint buffer)
	{
		std::vector<char> audioData;
		char* data;
		int  result = 0;

		// Create buffer
		data = OGRE_ALLOC_T(char, mBufferSize, Ogre::MEMCATEGORY_GENERAL);
		memset(data, 0, mBufferSize);
		
		// Read only what was asked for
		while( !mStreamEOF && (static_cast<int>(audioData.size()) < mBufferSize) )
		{
			const size_t remaining = mBufferSize - audioData.size();
			// Read up to a buffer's worth of data. stb_vorbis returns the
			// number of sample frames, which is what OpenAL calls "samples".
			const int frames = stb_vorbis_get_samples_short_interleaved(
				mOggStream, mChannels, reinterpret_cast<short*>(data),
				static_cast<int>(remaining / sizeof(short)));
			const int bytes = frames * mChannels * static_cast<int>(sizeof(short));
			// EOF check
			if (bytes == 0)
			{
				// If set to loop wrap to start of stream
				if ( mLoop && mSeekable )
				{
					if ( !seekOggStream(mOggStream, mSampleRate, mLoopOffset) )
					{
						OGRE_LOG_ERROR("OgreOggStream::_stream() - Looping stream, failed to seek to loop point!");
						break;
					}
				}
				else
				{
					mStreamEOF=true;
					// Don't loop - finish.
					break;
				}
			}
			// Append to end of buffer
			audioData.insert(audioData.end(), data, data + bytes);
			// Keep track of read data
			result+=bytes;
		}

		// EOF
		if(result == 0)
		{
			OGRE_FREE(data, Ogre::MEMCATEGORY_GENERAL);
			return false;
		}

		alGetError();
		// Copy buffer data
		alBufferData(buffer, mFormat, &audioData[0], static_cast<ALsizei>(audioData.size()), mSampleRate);

		// Cleanup
		OGRE_FREE(data, Ogre::MEMCATEGORY_GENERAL);

		return true;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_dequeue()
	{
		if(mSource == AL_NONE)
			return;

		/** Check current state
		@remarks
			Fix for bug where prebuffering a streamed sound caused a buffer problem
			resulting in only 1st buffer repeatedly looping. This is because alSourceStop() 
			doesn't function correctly if the sources state hasn't previously been set!!???
		*/
		ALenum state;
		alGetSourcei(mSource, AL_SOURCE_STATE, &state);

		// Force mSource to change state so the call to alSourceStop() will mark buffers correctly.
		if (state == AL_INITIAL)
			alSourcePlay(mSource);

		// Stop source to allow unqueuing
		alSourceStop(mSource);

		int queued=0;

		// Get number of buffers queued on source
		alGetError();
		alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &queued);

		while (queued--)
		{
			ALuint buffer;
			// Remove number of buffers from source
			alSourceUnqueueBuffers(mSource, 1, &buffer);

			// Any problems?
			if ( alGetError() != AL_NO_ERROR )
				OGRE_LOG_ERROR("OgreOggStreamSound::_dequeue() - Unable to unqueue buffers");
		}
	}		 
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_updatePlayPosition()
	{
		if ( mSource==AL_NONE || !mSeekable ) 
			return;

		bool paused = isPaused();
		bool playing = isPlaying();

		// Seek...
		pause();
		if ( !seekOggStream(mOggStream, mSampleRate, mPlayPos) )
		{
			OGRE_LOG_ERROR("OgreOggStreamSound::_updatePlayPosition() - Unable to seek!");
			return;
		}

		// Unqueue all buffers
		_dequeue();

		// Fill buffers
		_prebuffer();

		// Reset state..
		if		( playing ) play();
		else if ( paused ) pause();

		// Set flag
		mStreamEOF=false;
		mPlayPosChanged = false;
		mLastOffset = mPlayPos;
	}			   
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::setPlayPosition(float seconds)
	{
		if ( !mSeekable || seconds<0.f ) 
			return;

		// Wrap time
		if ( seconds>mPlayTime )
		{
			do { seconds-=mPlayTime; } while ( seconds>mPlayTime );
		}

		// Set position
		mPlayPos = seconds;
	
		// Set flag
		mPlayPosChanged = true;
	}	 
	/*/////////////////////////////////////////////////////////////////*/
	float OgreOggStreamSound::getPlayPosition() const
	{
		if ( !mSeekable || !mSource ) 
			return -1.f;

		float pos=0.f;
		alGetSourcef(mSource, AL_SEC_OFFSET, &pos);

		// Wrap if necessary
		if ((mLastOffset+pos)>=mPlayTime) 
			return (mLastOffset+pos) - mPlayTime;
		else
			return 
			mLastOffset + pos;
	}			  
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_pauseImpl()
	{
		assert(mState != SS_DESTROYED);

		if(mSource == AL_NONE) return;

		alSourcePause(mSource);
		mState = SS_PAUSED;
		
		// Notify listener
		if ( mSoundListener ) mSoundListener->soundPaused(this);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_playImpl()
	{
		assert(mState != SS_DESTROYED);

		if (isPlaying())
			return;

		// Grab a source if not already attached
		if (mSource == AL_NONE)
			if ( !OgreOggSoundManager::getSingleton()._requestSoundSource(this) )
				return;

		alGetError();
		// Play source
		alSourcePlay(mSource);
		if ( alGetError() )
		{
			OGRE_LOG_ERROR("OgreOggStreamSound::_playImpl() - Unable to play sound");
			return;
		}
		// Set play flag
		mState = SS_PLAYING;
		
		// Notify listener
		if ( mSoundListener ) mSoundListener->soundPlayed(this);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_stopImpl()
	{
		assert(mState != SS_DESTROYED);

		if(mSource != AL_NONE)
		{
			// Remove audio data from source
			_dequeue();

			// Stop playback
			mState = SS_STOPPED;

			if (mTemporary)
			{
				mState = SS_DESTROYED;
				OgreOggSoundManager::getSingletonPtr()->_destroyTemporarySound(this);
				return;
			}

			// Jump to beginning if seeking available
			if ( mSeekable ) 
			{
				seekOggStream(mOggStream, mSampleRate, 0.f);
				mLastOffset=0;
				mStreamEOF=false;
			}
			// Non-seekable - close/reopen
			else
			{
				// Close
				_release();

				// Reopen
				_openImpl(mAudioStream);
			}

			// If marked for auto-destruction request destroy()
			if (mTemporary)
			{
				mAwaitingDestruction=true;
			}
			else
			{
				// Reload data
				_prebuffer();

				// Give up source immediately if specfied
				if (mGiveUpSource) OgreOggSoundManager::getSingleton()._releaseSoundSource(this);
			}
		
			// Notify listener
			if ( mSoundListener ) mSoundListener->soundStopped(this);
		}
	}
}

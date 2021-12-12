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

#include "OgreOggStreamBufferSound.h"
#include <string>
#include <iostream>
#include "OgreOggSound.h"

namespace OgreOggSound
{

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStreamBufferSound::OgreOggStreamBufferSound(
		const Ogre::String& name
		#if OGRE_VERSION_MAJOR == 2
		, Ogre::IdType id, Ogre::ObjectMemoryManager *objMemMgr, Ogre::uint8 renderQueueId
		#endif
	) : OgreOggISound(
		name
		#if OGRE_VERSION_MAJOR == 2
		, id, objMemMgr, renderQueueId
		#endif
	)
		,mFreq(0)
	{
		mStream=false;
	}
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStreamBufferSound::~OgreOggStreamBufferSound()
	{		
		// Notify listener
		if ( mSoundListener ) mSoundListener->soundDestroyed(this);

		_release();
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamBufferSound::_release()
	{
		ALuint src=AL_NONE;
		setSource(src);
		mPlayPos = 0.f;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamBufferSound::setSource(ALuint& src)
	{
		if (src!=AL_NONE)
		{
			// Attach new source
			mSource=src;

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
	bool OgreOggStreamBufferSound::isMono()
	{
		if ( !mInitialised ) return false;
		return ( (mFormat==AL_FORMAT_MONO16) || (mFormat==AL_FORMAT_MONO8) );
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamBufferSound::setFormat(ALenum format, int freq)
	{
		mFormat = format;
		mFreq = freq;
		mInitialised = true;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamBufferSound::insertData(char* data, size_t dataLen, bool start)
	{
		if (mSource == AL_NONE)
			if ( !OgreOggSoundManager::getSingleton()._requestSoundSource(this) )
				return;
			
		ALuint buffID;
		alGenBuffers(1, &buffID);
		alBufferData(buffID, mFormat, data, dataLen, mFreq);
		alSourceQueueBuffers(mSource, 1, &buffID);
		mAlBuffers.push_back(buffID);
		if (start && mState != SS_PLAYING) {
			play();
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamBufferSound::_pauseImpl()
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
	void OgreOggStreamBufferSound::_playImpl()
	{
		assert(mState != SS_DESTROYED);

		if(isPlaying())
			return;

		if (mSource == AL_NONE)
			if ( !OgreOggSoundManager::getSingleton()._requestSoundSource(this) )
				return;

		alSourcePlay(mSource);
		mState = SS_PLAYING;

		// Notify listener
		if (mSoundListener) 
			mSoundListener->soundPlayed(this);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamBufferSound::_stopImpl()
	{
		assert(mState != SS_DESTROYED);

		if ( mSource==AL_NONE || isStopped() ) return;

		alSourceStop(mSource);
		alSourceRewind(mSource);
		mState = SS_STOPPED;

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
	void OgreOggStreamBufferSound::_updateAudioBuffers()
	{
		// do nothing when not playing
		if (!isPlaying())
			return;

		// dequeue processed buffers
		int processed;
		alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
		while(processed--)
		{
			//printf("processed %d / %d\n", processed+1, mAlBuffers.size());
			ALuint buff = mAlBuffers.front();
			mAlBuffers.pop_front();
			alSourceUnqueueBuffers(mSource, 1, &buff);
			alDeleteBuffers(1, &buff);
		}

		ALenum state;
		alGetSourcei(mSource, AL_SOURCE_STATE, &state);
		if (state == AL_STOPPED && mAlBuffers.size() == 0) {
			// source is in stop state and we don't have more buffers to play ... so stop
			stop();
			// Finished callback
			if ( mSoundListener ) 
				mSoundListener->soundFinished(this);
		}
	}
}

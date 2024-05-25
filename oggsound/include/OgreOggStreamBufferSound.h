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
* DESCRIPTION: Implements methods for creating/using a streamed ogg sound
*/

#pragma once

#include "OgreOggSoundPrereqs.h"
#include "OgreOggISound.h"

namespace OgreOggSound
{
	//! A single streaming sound (OGG)
	/** Handles playing a sound from an ogg stream.
	*/
	class _OGGSOUND_EXPORT OgreOggStreamBufferSound : public OgreOggISound
	{
	public:
		/** Sets looping status.
		@remarks
			Sets wheter the sound should loop
			@param loop
				If true, then sound will loop
		 */
		void loop(bool loop) { mLoop = loop; };
		/** Sets the position of the playback cursor in seconds
		@param seconds
			Play position in seconds 
		 */
		void setPlayPosition(float seconds) {}
		/** Gets the position of the playback cursor in seconds
		 */
		float getPlayPosition() const { return mPlayPos; }
		/** Sets the source to use for playback.
		@remarks
			Sets the source object this sound will use to queue buffers onto
			for playback. Also handles refilling buffers and queuing up.
			@param src
				OpenAL Source ID.
		 */
		void setSource(ALuint src);
		/** Returns whether sound is mono
		*/
		bool isMono();
		/** Returns the buffer sample rate
		 */
		unsigned int getSampleRate() { return 0; };
		/** Returns the buffer number of channels
		 */
		unsigned short getChannels()  { return 0; };
		/** Returns the buffer bits per sample
		 */
		unsigned int getBitsPerSample() { return 0; };
		/** Set format and sampling frequency
		 */
		void setFormat(ALenum format, int freq);
		/** Insert sound data buffor
		 */
		void insertData(char* data, size_t dataLen, bool start = true);

	protected:
		/** Constructor
		@remarks
			Creates a streamed sound object for playing audio directly from a file stream.
			@param name
				Unique name for sound.
			@param scnMgr
				SceneManager which created this sound (if the sound was created through the plugin method createMovableobject()).
		 */
		OgreOggStreamBufferSound(
			const Ogre::String& name
			#if AV_OGRE_NEXT_VERSION >= 0x20000
			, Ogre::SceneManager* scnMgr, Ogre::IdType id, Ogre::ObjectMemoryManager *objMemMgr, Ogre::uint8 renderQueueId
			#endif
		);
		/**
		 * Destructor
		 */
		~OgreOggStreamBufferSound();
		/** Opens audio file.
		@remarks
			Opens a specified file and checks validity.
			Reads first chunks of audio data into buffers.
			@param fileStream
				File stream pointer
		 */
		void _openImpl(Ogre::DataStreamPtr& fileStream) {}
		/** Stops playing sound.
		@remarks
			Stops playing audio immediately and resets playback. 
			If specified to do so its source will be released also.
		*/
		void _stopImpl();
		/** Pauses sound.
		@remarks
			Pauses playback on the source.
		 */
		void _pauseImpl();
		/** Plays the sound.
		@remarks
			Begins playback of all buffers queued on the source. If a
			source hasn't been setup yet it is requested and initialised
			within this call.
		 */
		void _playImpl();
		/** Updates the data buffers with sound information.
		@remarks
			This function refills processed buffers with audio data from
			the stream, it automatically handles looping if set.
		 */
		void _updateAudioBuffers();
		/** Prefills buffers with audio data.
		@remarks
			Loads audio data from the stream into the predefined data
			buffers and queues them onto the source ready for playback.
		 */
		void _prebuffer() {}
		/** Calculates buffer size and format.
		@remarks
			Calculates a block aligned buffer size of 250ms using
			sounds properties
		 */
		bool _queryBufferInfo() { return false; };
		/** Releases buffers and OpenAL objects.
		@remarks
			Cleans up this sounds OpenAL objects, including buffers
			and file pointers ready for destruction.
		 */
		void _release();

		int mFreq;
		std::list<ALuint> mAlBuffers;
		friend class OgreOggSoundManager;
	};
}

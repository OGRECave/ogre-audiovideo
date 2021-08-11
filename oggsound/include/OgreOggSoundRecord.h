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
* DESCRIPTION: Implements methods for recording audio
*/

#pragma once

#include "OgreOggSoundPrereqs.h"

#include <fstream>

// WAVE Format documentation:
// http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html

// Format Codes (only PCM supported)
#define WAVE_FORMAT_PCM			0x0001	// PCM
#define WAVE_FORMAT_IEEE_FLOAT	0x0003	// IEEE float
#define WAVE_FORMAT_ALAW		0x0006	// 8-bit ITU-T G.711 A-law
#define WAVE_FORMAT_MULAW		0x0007	// 8-bit ITU-T G.711 µ-law
#define WAVE_FORMAT_EXTENSIBLE	0xFFFE	// Determined by SubFormat

namespace OgreOggSound
{
	// https://en.cppreference.com/w/cpp/language/types
	// https://en.cppreference.com/w/cpp/types/integer
	typedef char CHAR;				// 8-bit char
	typedef unsigned short WORD;	// 16-bit int
	typedef unsigned int  DWORD;	// 32-bit int

	//! WAVE file header information
	struct WAVEHEADER
	{
		CHAR	cRIFF[4];	// The 'RIFF' chunk descriptor
		DWORD	dwRIFFSize;	// RIFF Chunk Size
		CHAR	cWave[4];	// Type: WAVE
		CHAR	cFmt[4];	// The 'fmt_' subchunk
		DWORD	dwFmtSize;	// fmt_ subchunk Size

		//! WAVE file format structure
		struct wFormat
		{
			WORD 	wFormatTag;			// Format category (WAVE_FORMAT_*)
			WORD 	wChannels;			// Number of channels
			DWORD 	dwSamplesPerSec;	// Sampling rate
			DWORD 	dwAvgBytesPerSec;	// For buffer estimation
			WORD 	wBlockAlign;		// Data block size
			WORD 	wBitsPerSample; 	// Sample size
		} wfex;

		CHAR	cData[4];	// The 'data' subchunk
		DWORD	dwDataSize;	// data subchunk size
	};

	//! Captures audio data
	/**
	@remarks
		This class can be used to capture audio data to an external file, WAV file ONLY.\n
		Use control panel --> Sound and Audio devices applet to select input type and volume.
	@par
		Default file properties are - Frequency: 44.1Khz, Format: 16-bit stereo, Buffer Size: 8820 bytes.
	@note
		This class should be instantiated by using the OgreOggSoundManager::createRecorder() function.
	*/
	class _OGGSOUND_EXPORT OgreOggSoundRecord
	{
	private:
		ALCdevice*			mDevice;
		ALCcontext*			mContext;
		ALCdevice*			mCaptureDevice;
		const ALCchar*		mDefaultCaptureDevice;
		ALint				mSamplesAvailable;
		std::ofstream		mFile;
		ALchar*				mBuffer;
		WAVEHEADER			mWaveHeader;
		ALint				mDataSize;
		ALint				mSize;
		Ogre::String		mOutputFile;
		Ogre::String		mDeviceName;
		ALCuint				mFreq;
		ALCenum				mFormat;
		ALsizei				mBufferSize;
		unsigned short		mBitsPerSample;
		unsigned short		mNumChannels;
		bool				mRecording;

		/** Updates recording from the capture device
		*/
		void _updateRecording();
		/** Initialises a capture device ready to record audio data
		@remarks
			Gets a list of capture devices, initialises one, and opens output file for writing to.
		*/
		bool _openDevice();

	public:
		/** Creates an instance of the class used to manage sound recording with OpenAL.
		@note
			This class is intended to be instanced by calling OgreOggSoundManager::createRecorder().
		 */
		OgreOggSoundRecord(ALCdevice& alDevice);
		/** Creates a capture object
		*/
		bool initCaptureDevice(const Ogre::String& devName="", const Ogre::String& fileName="output.wav", ALCuint freq=44100, ALCenum format=AL_FORMAT_STEREO16, ALsizei bufferSize=8820);
		/** Starts a recording from a capture device
		*/
		void startRecording();
		/** Returns whether the ALC_EXT_CAPTURE extension is available for the OpenAL implementation being used
		*/
		bool isCaptureAvailable();
		/** Stops recording from the capture device
		*/
		void stopRecording();
		/** Closes capture device, outputs captured data to a file if available.
		*/
		~OgreOggSoundRecord();

		// Manager friend
		friend class OgreOggSoundManager;
	};
}

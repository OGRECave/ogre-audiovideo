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

#include "OgreOggSoundRecord.h"

namespace OgreOggSound
{
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggSoundRecord::OgreOggSoundRecord(ALCdevice& alDevice) :
		mDevice(&alDevice)
		,mContext(0)
		,mCaptureDevice(0)
		,mDefaultCaptureDevice(0)
		,mSamplesAvailable(0)
		,mDataSize(0)
		,mBuffer(0)
		,mSize(0)
		,mOutputFile("output.wav")
		,mFreq(44100)
		,mBitsPerSample(16)
		,mNumChannels(2)
		,mFormat(AL_FORMAT_STEREO16)
		,mBufferSize(8820)
		,mRecording(false)
	{
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundRecord::_updateRecording()
	{
		if ( !mRecording || !mCaptureDevice || !mFile.is_open() ) return;

		// Find out how many samples have been captured
		alcGetIntegerv(mCaptureDevice, ALC_CAPTURE_SAMPLES, 1, &mSamplesAvailable);

		// When we have enough data to fill our BUFFERSIZE byte buffer, grab the samples
		if (mSamplesAvailable > (mBufferSize / mWaveHeader.wfex.wBlockAlign))
		{
			// Consume Samples
			alcCaptureSamples(mCaptureDevice, mBuffer, mBufferSize / mWaveHeader.wfex.wBlockAlign);

			// Write the audio data to a file
			mFile.write(mBuffer, mBufferSize);

			// Record total amount of data recorded
			mDataSize += mBufferSize;
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundRecord::isCaptureAvailable()
	{
		if (alcIsExtensionPresent(mDevice, "ALC_EXT_CAPTURE") == AL_FALSE)
		{
			Ogre::LogManager::getSingleton().logMessage("*** --- No Capture Extension detected! --- ***");
			return false;
		}
		return true;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundRecord::initCaptureDevice(const Ogre::String& deviceName, const Ogre::String& fileName, ALCuint freq, ALCenum format, ALsizei bufferSize)
	{
		if ( !isCaptureAvailable() ) return false;

		mFreq=freq;
		mFormat=format;
		mBufferSize=bufferSize;

		// Set channel
		if		( mFormat==AL_FORMAT_MONO8 )	{ mNumChannels=1; mBitsPerSample=8; }
		else if ( mFormat==AL_FORMAT_STEREO8 )	{ mNumChannels=2; mBitsPerSample=8; }
		else if ( mFormat==AL_FORMAT_MONO16 )	{ mNumChannels=1; mBitsPerSample=16;}
		else if ( mFormat==AL_FORMAT_STEREO16 )	{ mNumChannels=2; mBitsPerSample=16;}
#if HAVE_ALEXT == 1
		else if ( mFormat==AL_FORMAT_MONO_FLOAT32 )		{ mNumChannels=1; mBitsPerSample=32;}
		else if ( mFormat==AL_FORMAT_STEREO_FLOAT32 )	{ mNumChannels=2; mBitsPerSample=32;}
#endif

		// Selected device
		mDeviceName = deviceName;

		// No device specified - select default
		if ( mDeviceName.empty() )
		{
			// Get the name of the 'default' capture device
			mDefaultCaptureDevice = alcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
			mDeviceName = mDefaultCaptureDevice;
			Ogre::LogManager::getSingleton().logMessage("Selected Default Capture Device: " + mDeviceName);
		}

		mCaptureDevice = alcCaptureOpenDevice(mDeviceName.c_str(), mFreq, mFormat, mBufferSize);
		if ( mCaptureDevice )
		{
			Ogre::LogManager::getSingleton().logMessage("Opened Capture Device: " + mDeviceName);

			// attempt to open file (binary | writing)
			mFile.open(mOutputFile.c_str(), std::ios::out|std::ios::binary);

			if ( mFile.is_open() )
			{
				// Prepare a WAVE file header for the captured data
				sprintf(mWaveHeader.cRIFF, "RIFF");
				mWaveHeader.dwRIFFSize = 0;
				sprintf(mWaveHeader.cWave, "WAVE");
				sprintf(mWaveHeader.cFmt, "fmt ");
				mWaveHeader.dwFmtSize = sizeof(mWaveHeader.wfex);
#if HAVE_ALEXT == 1
				mWaveHeader.wfex.wFormatTag = (mBitsPerSample == 32 ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM);
#else
				mWaveHeader.wfex.wFormatTag = WAVE_FORMAT_PCM;
#endif
				mWaveHeader.wfex.wChannels = mNumChannels;
				mWaveHeader.wfex.dwSamplesPerSec = mFreq;
				mWaveHeader.wfex.wBitsPerSample = mBitsPerSample;
				mWaveHeader.wfex.wBlockAlign = mWaveHeader.wfex.wChannels * mWaveHeader.wfex.wBitsPerSample / 8;
				mWaveHeader.wfex.dwAvgBytesPerSec = mWaveHeader.wfex.dwSamplesPerSec * mWaveHeader.wfex.wBlockAlign;
				sprintf(mWaveHeader.cData, "data");
				mWaveHeader.dwDataSize = 0;

				mFile.write(reinterpret_cast<char*>(&mWaveHeader), sizeof(WAVEHEADER));

				// Generate buffer for capture data
				mBuffer = OGRE_ALLOC_T(ALchar, mBufferSize, Ogre::MEMCATEGORY_GENERAL);

				return true;
			}

			Ogre::LogManager::getSingleton().logError("*** --- OgreOggSoundRecord::initCaptureDevice() - Unable to open recording file: " + mOutputFile);
			return false;
		}

		Ogre::LogManager::getSingleton().logError("*** --- OgreOggSoundRecord::initCaptureDevice() - Unable to open recording device: " + mDeviceName);

		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundRecord::startRecording()
	{
		if ( !mCaptureDevice ) return;

		// Start audio capture
		alcCaptureStart(mCaptureDevice);

		// Flag recording
		mRecording = true;
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundRecord::stopRecording()
	{
		if ( !mRecording || !mCaptureDevice ) return;

		// Stop capture
		alcCaptureStop(mCaptureDevice);

		mRecording = false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggSoundRecord::~OgreOggSoundRecord()
	{
		// Stop recording if necessary
		if ( mRecording ) stopRecording();

		// Close file write output
		if ( mFile.is_open() )
		{
			// Check if any Samples haven't been consumed yet
			alcGetIntegerv(mCaptureDevice, ALC_CAPTURE_SAMPLES, 1, &mSamplesAvailable);
			while (mSamplesAvailable)
			{
				if (mSamplesAvailable > (mBufferSize / mWaveHeader.wfex.wBlockAlign))
				{
					alcCaptureSamples(mCaptureDevice, mBuffer, mBufferSize / mWaveHeader.wfex.wBlockAlign);
					mFile.write(mBuffer, mBufferSize);
					mSamplesAvailable -= (mBufferSize / mWaveHeader.wfex.wBlockAlign);
					mDataSize += mBufferSize;
				}
				else
				{
					alcCaptureSamples(mCaptureDevice, mBuffer, mSamplesAvailable);
					mFile.write(mBuffer, mSamplesAvailable * mWaveHeader.wfex.wBlockAlign);
					mDataSize += mSamplesAvailable * mWaveHeader.wfex.wBlockAlign;
					mSamplesAvailable = 0;
				}
			}

			// Fill in Size information in Wave Header
			mFile.seekp(sizeof(mWaveHeader.cRIFF));
			mSize = mDataSize + sizeof(WAVEHEADER) - 8;
			mFile.write(reinterpret_cast<char*>(&mSize), 4);
			mFile.seekp(sizeof(WAVEHEADER) - 4);
			mFile.write(reinterpret_cast<char*>(&mDataSize), 4);
			mFile.close();
		}

		// Destroy audio buffer
		if ( mBuffer ) 	OGRE_FREE(mBuffer, Ogre::MEMCATEGORY_GENERAL);

		// Close the Capture Device
		if ( mCaptureDevice ) alcCaptureCloseDevice(mCaptureDevice);
	}
}

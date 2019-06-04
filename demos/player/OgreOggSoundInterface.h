/** @file

This is simple interface between
  Ogre Theora Video Plugin
and
  OgreOggSound Plugin

@section COPYRIGHT

Copyright (c) 2018, Robert Paciorek (http://www.opcode.eu.org/),
                    BSD/MIT-type license

This text/program is free document/software. Redistribution and use in
source and binary forms, with or without modification, ARE PERMITTED provided
save this copyright notice. This document/program is distributed WITHOUT any
warranty, use at YOUR own risk.


The MIT License:

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#include <OgreOggSoundManager.h>
#include <OgreOggStreamBufferSound.h>
#include <TheoraAudioInterface.h>
#include <TheoraVideoClip.h>

#ifndef DEBUG_LOG_STREAM
#define DEBUG_LOG_STREAM_OWN
#define DEBUG_LOG_STREAM(a)
#endif

struct OgreOggSoundInterface : public TheoraAudioInterface {
	OgreOggSound::OgreOggStreamBufferSound* ogreOggSoundObj;
	int numOfOutputChannels;

	short* dataBuf;
	int    dataBufSize;
	int    dataBufPos;

	int    inserCounter;

	OgreOggSoundInterface(TheoraVideoClip *owner, int nChannels, int freq) :
		TheoraAudioInterface(owner, nChannels, freq),
		numOfOutputChannels( (nChannels == 1) ? 1 : 2 ),
		dataBufSize(numOfOutputChannels * freq / 20),
		dataBufPos(0),
		inserCounter(0)
	{
		DEBUG_LOG_STREAM("OgreOggSoundInterface for " << owner->getName() <<
		                 " inputChannels=" << mNumChannels << "outputChannels " << numOfOutputChannels <<
		                 " freq=" << mFreq << " dataBufSize=" << dataBufSize
		);

		ogreOggSoundObj = static_cast<OgreOggSound::OgreOggStreamBufferSound*>(
			OgreOggSound::OgreOggSoundManager::getSingletonPtr()->createSound( owner->getName(), "BUFFER" )
		);
		ogreOggSoundObj->setFormat( (nChannels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, mFreq/nChannels ); // FIXME: why mFreq/nChannels instefd of mFreq

		dataBuf = new short[dataBufSize + 32];
	}

	void insertData(float **data, int nSamples) override final {
		int nSamplePacks = nSamples / mNumChannels;
		for (int i=0, inIdx=0; i<nSamplePacks; ++i) {
			for (int j=0; j<numOfOutputChannels; ++j)
				dataBuf[dataBufPos++]   = static_cast<short>(Ogre::Math::Clamp((*data)[inIdx+j], -1.0f, 1.0f) * 32767);
			inIdx  += mNumChannels;

			if (dataBufPos >= dataBufSize) {
				ogreOggSoundObj->insertData(reinterpret_cast<char*>(dataBuf), dataBufPos * 2, (++inserCounter) > 2);
				dataBufPos = 0;
			}
		}
	}

	void destroy() override final {
		delete this;
	}
};

struct OgreOggSoundInterfaceFactory : TheoraAudioInterfaceFactory {
	TheoraAudioInterface* createInstance(TheoraVideoClip *owner, int nChannels, int freq) override final {
		return new OgreOggSoundInterface(owner, nChannels, freq);
	}

	static OgreOggSoundInterfaceFactory* getSingletonPtr() {
		static OgreOggSoundInterfaceFactory singleton;
		return &singleton;
	}
protected:
	OgreOggSoundInterfaceFactory() {}

	virtual ~OgreOggSoundInterfaceFactory() {}

};

#ifdef DEBUG_LOG_STREAM_OWN
#undef DEBUG_LOG_STREAM
#endif

#include "OpenAL_AudioInterface.h"



#include "OgrePrerequisites.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"


ALCdevice* gDevice=0;
ALCcontext* gContext=0;


namespace Ogre
{
	OpenAL_AudioInterface::OpenAL_AudioInterface(TheoraVideoClip* owner,int nChannels,int freq) :
		TheoraAudioInterface(owner,nChannels,freq)
	{
		mMaxBuffSize=freq*2;
		mBuffSize=0;
		mNumProcessedSamples=0;
		mTimeOffset=0;

		mTempBuffer=new short[mMaxBuffSize];
		alGenSources(1,&mSource);
		owner->setTimer(this);
		mNumPlayedSamples=0;
	}

	OpenAL_AudioInterface::~OpenAL_AudioInterface()
	{
		// todo: delete buffers and source
		if (mTempBuffer) delete mTempBuffer;
	}

	void OpenAL_AudioInterface::insertData(float** data,int nSamples)
	{
		int s;
		for (int i=0;i<nSamples;i++)
		{
			s=data[0][i]*32767;
			if (s >  32767) s= 32767;
			if (s < -32767) s=-32767;

			if (mBuffSize < mMaxBuffSize) mTempBuffer[mBuffSize++]=s;
			if (mBuffSize == mFreq/4)
			{	
				OpenAL_Buffer buff;
				alGenBuffers(1,&buff.id);
				alBufferData(buff.id,AL_FORMAT_MONO16,mTempBuffer,mBuffSize*2,mFreq);
				alSourceQueueBuffers(mSource, 1, &buff.id);
				buff.nSamples=mBuffSize;
				mNumProcessedSamples+=mBuffSize;
				mBufferQueue.push(buff);

				mBuffSize=0;

				int state;
				alGetSourcei(mSource,AL_SOURCE_STATE,&state);
				if (state != AL_PLAYING)
				{
					//alSourcef(mSource,AL_PITCH,0.5); // debug
					alSourcef(mSource,AL_SAMPLE_OFFSET,mNumProcessedSamples-mFreq/4);
					alSourcePlay(mSource);

				}

			}
		}


	}

	void OpenAL_AudioInterface::update(float time_increase)
	{
		int i,state,nProcessed;
		OpenAL_Buffer buff;

		// process played buffers
		alGetSourcei(mSource,AL_BUFFERS_PROCESSED,&nProcessed);
		for (i=0;i<nProcessed;i++)
		{
			buff=mBufferQueue.front();
			mBufferQueue.pop();
			mNumPlayedSamples+=buff.nSamples;
			alSourceUnqueueBuffers(mSource,1,&buff.id);
			alDeleteBuffers(1,&buff.id);
		}

		// control playback and return time position
		alGetSourcei(mSource,AL_SOURCE_STATE,&state);
		if (state == AL_PLAYING)
		{
			alGetSourcef(mSource,AL_SEC_OFFSET,&mTime);
			mTime+=(float) mNumPlayedSamples/mFreq;
			mTimeOffset=0;
		}
		else
		{
			mTime=(float) mNumProcessedSamples/mFreq+mTimeOffset;
			mTimeOffset+=time_increase;
		}

		float duration=mClip->getDuration();
		if (mTime > duration) mTime=duration;
	}

	void OpenAL_AudioInterface::pause()
	{
		alSourcePause(mSource);
		TheoraTimer::pause();
	}

	void OpenAL_AudioInterface::play()
	{
		alSourcePlay(mSource);
		TheoraTimer::play();
	}


	OpenAL_AudioInterfaceFactory::OpenAL_AudioInterfaceFactory()
	{
		// openal init is here used only to simplify samples for this plugin
		// if you want to use this interface in your own program, you'll
		// probably want to remove the openal init/destory lines
		gDevice = alcOpenDevice("Generic Software");
		if (alcGetError(gDevice) != ALC_NO_ERROR) goto Fail;
		gContext = alcCreateContext(gDevice, NULL);
		if (alcGetError(gDevice) != ALC_NO_ERROR) goto Fail;
		alcMakeContextCurrent(gContext);
		if (alcGetError(gDevice) != ALC_NO_ERROR) goto Fail;

		return;
Fail:
		gDevice=NULL;
		gContext=NULL;
	}

	OpenAL_AudioInterfaceFactory::~OpenAL_AudioInterfaceFactory()
	{
		if (gDevice)
		{
			alcMakeContextCurrent(NULL);
			alcDestroyContext(gContext);
			alcCloseDevice(gDevice);
		}
	}

	OpenAL_AudioInterface* OpenAL_AudioInterfaceFactory::createInstance(TheoraVideoClip* owner,int nChannels,int freq)
	{
		return new OpenAL_AudioInterface(owner,nChannels,freq);
	}
}
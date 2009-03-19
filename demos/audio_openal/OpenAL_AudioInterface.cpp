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
		}
	}


	void OpenAL_AudioInterface::update(float time_increase)
	{

		int state;
		alGetSourcei(mSource,AL_SOURCE_STATE,&state);

		int nProcessed;
		OpenAL_Buffer buff;
		alGetSourcei(mSource,AL_BUFFERS_PROCESSED,&nProcessed);

		for (int i=0;i<nProcessed;i++)
		{
			buff=mBufferQueue.front();
			mBufferQueue.pop();
			mNumPlayedSamples+=buff.nSamples;
			alSourceUnqueueBuffers(mSource,1,&buff.id);
			alDeleteBuffers(1,&buff.id);
		}

		if (mBuffSize >= mFreq/4)
		{
			int size=mBuffSize; mBuffSize=mFreq/4;

			
			alGenBuffers(1,&buff.id);
			alBufferData(buff.id,AL_FORMAT_MONO16,mTempBuffer,mBuffSize*2,mFreq);
			alSourceQueueBuffers(mSource, 1, &buff.id);
			buff.nSamples=mBuffSize;
			mNumProcessedSamples+=mBuffSize;
			mBufferQueue.push(buff);

			for (int i=0;i<size-mFreq/4;i++)
				mTempBuffer[i]=mTempBuffer[i+mFreq/4];

			mBuffSize=size-mFreq/4;

			if (state != AL_PLAYING)
			{
				//alSourcef(mSource,AL_PITCH,0.5); //temp, for debug
				alSourcePlay(mSource);
				alSourcef(mSource,AL_SAMPLE_OFFSET,mNumProcessedSamples-mFreq/4);

			}

		}
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
		//mTime=mSourceTime;
		float duration=mClip->getDuration();
		if (mTime > duration) mTime=duration;
	}



	OpenAL_AudioInterfaceFactory::OpenAL_AudioInterfaceFactory()
	{
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
#include "OpenAL_AudioInterface.h"

ALCdevice* gDevice=0;
ALCcontext* gContext=0;


namespace Ogre
{
	OpenAL_AudioInterface::OpenAL_AudioInterface(TheoraVideoClip* owner,int nChannels,int freq) :
		TheoraAudioInterface(owner,nChannels,freq)
	{
		mMaxBuffSize=freq*2;
		mBuffSize=0;
		mBufferIndex=0;
		mNumProcessedSamples=0;
		mSourceTime=0.0;

		mTempBuffer=new short[mMaxBuffSize];
		for (int i=0;i<2;i++)
		{
			alGenBuffers(1,&mBuffers[i].id);
			mBuffers[i].queued=false;
		}
		alGenSources(1,&mSource);
		owner->setTimer(this);
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
		float time;
		alGetSourcef(mSource,AL_SEC_OFFSET,&time);
		if (time > mSourceTime) mSourceTime=time;
		
		// check for processed buffers
		int nProcessed;
		alGetSourcei(mSource,AL_BUFFERS_PROCESSED,&nProcessed);
		if (nProcessed > 0)
		{
			int index=(mBuffers[!mBufferIndex].queued) ? !mBufferIndex : mBufferIndex;
			if (mBuffers[index].queued)
			{
				mBuffers[index].queued=false;
				alSourceUnqueueBuffers(mSource,1,&mBuffers[index].id);
				mNumProcessedSamples+=mBuffers[index].nSamples;
				alGetSourcef(mSource,AL_SEC_OFFSET,&mSourceTime);
			}

		}


		int state;
		bool write_buffer=false;
		alGetSourcei(mSource,AL_SOURCE_STATE,&state);

		if (state == AL_PLAYING)
		{
			if (time > (mBuffers[!mBufferIndex].nSamples*0.8)/mFreq)
				write_buffer=true;
		}
		else
		{
			if (mBuffSize >= mFreq/2) write_buffer=true;
		}

		if (write_buffer) // let's hold half a second of audio data in each buffer
		{
			if (!mBuffers[mBufferIndex].queued)
			{
				alBufferData(mBuffers[mBufferIndex].id,AL_FORMAT_MONO16,mTempBuffer,mBuffSize*2,mFreq);
				alSourceQueueBuffers(mSource, 1, &mBuffers[mBufferIndex].id);
				mBuffers[mBufferIndex].queued=true;
				mBuffers[mBufferIndex].nSamples=mBuffSize;
				mBufferIndex=!mBufferIndex;
				mBuffSize=0;

				if (state != AL_PLAYING)
				{
					alSourcePlay(mSource);

				}
			}
		}



		//mTime+=time_increase;
		mTime=mSourceTime+(float) mNumProcessedSamples/mFreq;

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
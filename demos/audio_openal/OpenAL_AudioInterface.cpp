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
		mBufferIndex=0;
		mNumProcessedSamples=0;
		mSourceTime=0.0;
		mQueueCounter=0;

		mTempBuffer=new short[mMaxBuffSize];
		for (int i=0;i<1000;i++)
		{
			alGenBuffers(1,&mBuffers[i].id);
			mBuffers[i].queue_index=-1;
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
		
		int state;
		alGetSourcei(mSource,AL_SOURCE_STATE,&state);
/*
		if (time > mSourceTime) mSourceTime=time;
		

		int i,nProcessed,index;
		int pendreki[10];
		alGetSourcei(mSource,AL_BUFFERS_PROCESSED,&nProcessed);
		alGetSourceiv(mSource,AL_BUFFERS_PROCESSED,pendreki);
		if (nProcessed == 1)
			i=i;
		for (i=0;i<nProcessed;i++)
		{
			index=(mBuffers[0].queue_index > mBuffers[1].queue_index) ? 1 : 0;
			mBuffers[index].queue_index=-1;
			alSourceUnqueueBuffers(mSource,1,&mBuffers[index].id);
			mNumProcessedSamples+=mBuffers[index].nSamples;
			alGetSourcef(mSource,AL_SEC_OFFSET,&mSourceTime);
			time=mSourceTime;

			LogManager::getSingleton().logMessage("unqueuing: "+StringConverter::toString(index));
		}

		int state;
		bool write_buffer=false;
		alGetSourcei(mSource,AL_SOURCE_STATE,&state);

		if (state == AL_PLAYING)
		{
			//if (time > (mBuffers[!mBufferIndex].nSamples*0.7)/mFreq)
			//	write_buffer=true;
			if (mBuffSize >= mFreq/2) write_buffer=true;
		}
		else
		{
			if (mBuffSize >= mFreq/2) write_buffer=true;
		}

		if (write_buffer) // let's hold half a second of audio data in each buffer
		{
			mBufferIndex=(mBuffers[0].queue_index == -1) ? 0 : 1;
			if (mBuffers[mBufferIndex].queue_index == -1)
			{
				alBufferData(mBuffers[mBufferIndex].id,AL_FORMAT_MONO16,mTempBuffer,mBuffSize*2,mFreq);
				alSourceQueueBuffers(mSource, 1, &mBuffers[mBufferIndex].id);
				mBuffers[mBufferIndex].queue_index=mQueueCounter++;
				mBuffers[mBufferIndex].nSamples=mBuffSize;
				//mBufferIndex=!mBufferIndex;
				mBuffSize=0;

				if (state != AL_PLAYING)
				{
					alSourcef(mSource,AL_PITCH,0.5); //temp, for debug
					alSourcePlay(mSource);

				}
			}
		}



		if (state == AL_PLAYING) mTime+=time_increase*0.5;
		//mTime=mSourceTime+(float) mNumProcessedSamples/mFreq;
		*/

		if (mBuffSize >= mFreq/4)
		{
			int size=mBuffSize; mBuffSize=mFreq/4;
			alBufferData(mBuffers[mBufferIndex].id,AL_FORMAT_MONO16,mTempBuffer,mBuffSize*2,mFreq);
			alSourceQueueBuffers(mSource, 1, &mBuffers[mBufferIndex].id);
			mBuffers[mBufferIndex].queue_index=mQueueCounter++;
			mBuffers[mBufferIndex].nSamples=mBuffSize;
			mBufferIndex++;
			mNumProcessedSamples+=mBuffSize;
			mBuffSize=0;

			for (int i=0;i<size-mFreq/4;i++)
				mTempBuffer[i]=mTempBuffer[i+mFreq/4];

			mBuffSize=size-mFreq/4;

			if (state != AL_PLAYING)
			{
				//alSourcef(mSource,AL_PITCH,0.5); //temp, for debug
				alSourcePlay(mSource);
				alSourcef(mSource,AL_SAMPLE_OFFSET,mNumProcessedSamples-mBuffers[mBufferIndex-1].nSamples);

			}

		}
		alGetSourcei(mSource,AL_SOURCE_STATE,&state);
		if (state == AL_PLAYING)
		{
			alGetSourcef(mSource,AL_SEC_OFFSET,&mTime);
		}
		else
		{
			mTime=(float) mNumProcessedSamples/mFreq;
		}
		//mTime=mSourceTime;
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
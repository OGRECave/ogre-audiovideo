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
		for (int i=0;i<2;i++)
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
		alGetSourcef(mSource,AL_SEC_OFFSET,&time);
		if (time > mSourceTime) mSourceTime=time;
		
		// check for processed buffers
		LogManager::getSingleton().logMessage("0: "+StringConverter::toString(mBuffers[0].queue_index));
		LogManager::getSingleton().logMessage("1: "+StringConverter::toString(mBuffers[1].queue_index));
		int nProcessed;
		alGetSourcei(mSource,AL_BUFFERS_PROCESSED,&nProcessed);
		if (nProcessed > 0)
		{
			int index=(mBuffers[0].queue_index > mBuffers[1].queue_index) ? 1 : 0;
			mBuffers[index].queue_index=-1;
			alSourceUnqueueBuffers(mSource,1,&mBuffers[index].id);
			mNumProcessedSamples+=mBuffers[index].nSamples;
			alGetSourcef(mSource,AL_SEC_OFFSET,&mSourceTime);

			LogManager::getSingleton().logMessage("unqueuing: "+StringConverter::toString(index));
		}


		int state;
		bool write_buffer=false;
		alGetSourcei(mSource,AL_SOURCE_STATE,&state);

		if (state == AL_PLAYING)
		{
			if (time > (mBuffers[!mBufferIndex].nSamples*0.5)/mFreq)
				write_buffer=true;
		}
		else
		{
			if (mBuffSize >= mFreq/2) write_buffer=true;
		}

		if (write_buffer) // let's hold half a second of audio data in each buffer
		{
			if (mBuffers[mBufferIndex].queue_index == -1)
			{
				alBufferData(mBuffers[mBufferIndex].id,AL_FORMAT_MONO16,mTempBuffer,mBuffSize*2,mFreq);
				alSourceQueueBuffers(mSource, 1, &mBuffers[mBufferIndex].id);
				mBuffers[mBufferIndex].queue_index=mQueueCounter++;
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
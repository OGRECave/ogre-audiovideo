#include "OpenAL_AudioInterface.h"

ALCdevice* gDevice=0;
ALCcontext* gContext=0;


namespace Ogre
{
	OpenAL_AudioInterface::OpenAL_AudioInterface(TheoraVideoClip* owner,int nChannels) :
		TheoraAudioInterface(owner,nChannels)
	{
		mMaxBuffSize=40960*10;
		mBuffSize=0;
		mTempBuffer=new short[mMaxBuffSize];
		alGenBuffers(2,mBuffers);
		alGenSources(1,&mSource);
	}

	OpenAL_AudioInterface::~OpenAL_AudioInterface()
	{
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

			mTempBuffer[mBuffSize++]=s;
			if (mBuffSize == mMaxBuffSize)
			{
				alSourceStop(mSource);
//				short* drek=new short[100000];

//				for (int j=0;j<100000;j++)
//					drek[j]=sin(float(j)/5.0f)*32000;

				alSourcei(mSource, AL_BUFFER, 0);
				alBufferData(mBuffers[0],AL_FORMAT_MONO16,mTempBuffer,mBuffSize,44100);
				alSourcei(mSource, AL_BUFFER, mBuffers[0]);
				alSourcePlay(mSource);

				//delete drek;
				// dump buffer to OpenAL buffer and clear temp buffer
				mBuffSize=0;
			}
		}
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

	OpenAL_AudioInterface* OpenAL_AudioInterfaceFactory::createInstance(TheoraVideoClip* owner,int nChannels)
	{
		return new OpenAL_AudioInterface(owner,nChannels);
	}

}
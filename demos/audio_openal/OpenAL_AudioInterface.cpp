#include "OpenAL_AudioInterface.h"

namespace Ogre
{
	OpenAL_AudioInterface::OpenAL_AudioInterface(TheoraVideoClip* owner,int nChannels) :
		TheoraAudioInterface(owner,nChannels)
	{
		mMaxBuffSize=4096;
		mBuffSize=0;
		mTempBuffer=new unsigned short[mMaxBuffSize];
	}

	OpenAL_AudioInterface::~OpenAL_AudioInterface()
	{
		if (mTempBuffer) delete mTempBuffer;
	}

	void OpenAL_AudioInterface::insertData(float** data,int nSamples)
	{
		for (int i=0;i<nSamples;i++)
		{
			mTempBuffer[mBuffSize++]=data[0][i]*1000;
			if (mBuffSize == mMaxBuffSize)
			{
				mBuffSize=0;
			}
		}
	}

	OpenAL_AudioInterface* OpenAL_AudioInterfaceFactory::createInstance(TheoraVideoClip* owner,int nChannels)
	{
		return new OpenAL_AudioInterface(owner,nChannels);
	}

}
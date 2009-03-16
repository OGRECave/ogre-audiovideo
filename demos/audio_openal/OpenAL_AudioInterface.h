#include "TheoraAudioInterface.h"
#include "TheoraVideoClip.h"

#include "al.h"
#include "alc.h"

namespace Ogre
{
	class OpenAL_AudioInterface : public TheoraAudioInterface
	{
		int mMaxBuffSize;
		int mBuffSize;
		short *mTempBuffer;
		ALuint mBuffers[2];
		ALuint mSource;
	public:
		OpenAL_AudioInterface(TheoraVideoClip* owner,int nChannels);
		~OpenAL_AudioInterface();
		void insertData(float** data,int nSamples);
	};



	class OpenAL_AudioInterfaceFactory : public TheoraAudioInterfaceFactory
	{
	public:
		OpenAL_AudioInterfaceFactory();
		~OpenAL_AudioInterfaceFactory();
		OpenAL_AudioInterface* createInstance(TheoraVideoClip* owner,int nChannels);
	};
}
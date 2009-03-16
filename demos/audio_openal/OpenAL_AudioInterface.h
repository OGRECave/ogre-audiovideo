#include "TheoraAudioInterface.h"
#include "TheoraVideoClip.h"

namespace Ogre
{
	class OpenAL_AudioInterface : public TheoraAudioInterface
	{
		int mMaxBuffSize;
		int mBuffSize;
		unsigned short *mTempBuffer;
	public:
		OpenAL_AudioInterface(TheoraVideoClip* owner,int nChannels);
		~OpenAL_AudioInterface();
		void insertData(float** data,int nSamples);
	};

	class OpenAL_AudioInterfaceFactory : public TheoraAudioInterfaceFactory
	{
	public:
		OpenAL_AudioInterface* createInstance(TheoraVideoClip* owner,int nChannels);
	};
}
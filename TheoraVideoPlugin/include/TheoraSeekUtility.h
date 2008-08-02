#ifndef SEEK_UTIL_HEADER
#define SEEK_UTIL_HEADER

#include "OgreDataStream.h"
#include "ogg/ogg.h"
#include "theora/theora.h"
#include "vorbis/codec.h"
#include <map>

namespace Ogre
{
	class TheoraSeekUtility
	{
	private:
		TheoraSeekUtility() {}
	public:
		#if OGRE_COMPILER == OGRE_COMPILER_MSVC
			typedef __int64 my_int64;
		#else
			typedef int64_t my_int64;
		#endif

		enum RESOLUTION
		{
			High,		//Store every page
			Medium,		//store every other page
			Low			//store every third page
		};

		/** @remarks 
			Use to set the amount of pre seek scanning to be 
			done/built - currently not implement.. Only High is used
		*/
		static void setSeekBuilderResolution( RESOLUTION res ) 
			{ mResolution = res; }
		
		/** @remarks 
			Only constructor you should use to construct this object.
		*/
		TheoraSeekUtility( 
			DataStreamPtr stream, ogg_sync_state* sync_state,
			ogg_stream_state* th_stream, ogg_stream_state* vb_stream,
			theora_info *th_info, vorbis_info *vb_info,
			theora_state *th_state,	vorbis_dsp_state *dsp_state,
			vorbis_block *vb_block );
		
		~TheoraSeekUtility();

		/** @remarks 
				Builds a seek map (relates file byte position to movie time)
				for quick/easy seeking. Uses global resolution setting for 
				fine control
			@returns
				Time that movie "cursor" is actually at after we have messed 
				up and reset the current ogg/theora packets/streams
		*/		
		float buildTheoraSeekMap();

		//float buildVorbisSeekMap();

		/** @remarks 
				Seek will seek to the page prceding what you asked for,
				and grab the first keyframe
			@param seconds
				the seconds time to try to seek to
			@returns
				The granule time in seconds of where we seeked to (this
				is because of the guestimate factor - we may not be exactly
				at the time requested)
		*/
		float doSeek( float seconds );

	protected:
		void addSeekerPoint( float seconds, unsigned int byte );
		float getKeyFrameLock();
		
		typedef std::map<float, unsigned int> tSeeker;
		tSeeker SeekMap;
		static RESOLUTION mResolution;

		DataStreamPtr mStream;

		//We keep pointers to all the needed Ogg/Vorbis/Theora
		//structures for less method parameter overhead (me being lazy :)
		ogg_sync_state* m_sync_state;
		ogg_stream_state* m_th_stream;
		ogg_stream_state* m_vb_stream;
		theora_info *m_th_info;
		vorbis_info *m_vb_info;
		theora_state *m_th_state;
		vorbis_dsp_state *m_dsp_state;
		vorbis_block *m_vb_block;
	};
}
#endif

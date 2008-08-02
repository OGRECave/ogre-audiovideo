#include "TheoraSeekUtility.h"

namespace Ogre
{
	TheoraSeekUtility::RESOLUTION TheoraSeekUtility::mResolution = 
		TheoraSeekUtility::High;
	//-------------------------------------------------------------//
	TheoraSeekUtility::TheoraSeekUtility(
			DataStreamPtr stream,
			ogg_sync_state* sync_state,
			ogg_stream_state* th_stream, 
			ogg_stream_state* vb_stream,
			theora_info *th_info,
			vorbis_info *vb_info,
			theora_state *th_state,
			vorbis_dsp_state *dsp_state,
			vorbis_block *vb_block )

	{
		mStream = stream;

		m_sync_state = sync_state;
		m_th_info = th_info;
		m_vb_info = vb_info;
		m_th_stream = th_stream;
		m_vb_stream = vb_stream;

		m_th_state = th_state;
		m_dsp_state = dsp_state;
		m_vb_block = vb_block;
	}

	//-------------------------------------------------------------//
	TheoraSeekUtility::~TheoraSeekUtility()
	{
		//Destroy our reference to the stream
		mStream.setNull();
	}

	//-------------------------------------------------------------//
	float TheoraSeekUtility::doSeek( float seconds )
	{
		tSeeker::iterator i, end = SeekMap.end();

		//Retrieve closest time
		for( i = SeekMap.begin(); i != end; ++i )
			if( seconds < i->first )
				break;

		//Check for error conditions
		if( i == end )
			return 0.0f;

		//Make sure we don't decrement iterator is we are at the start
		if( i != SeekMap.begin() )
			--i;	
		
		//Reset ogg/theora/vorbis structures/decoding states
		ogg_sync_reset( m_sync_state );
		ogg_stream_reset( m_th_stream );
		ogg_stream_reset( m_vb_stream );
		
		theora_clear( m_th_state );
		theora_decode_init( m_th_state, m_th_info );
		vorbis_synthesis_restart( m_dsp_state );

		mStream->seek( i->second );
        //return the time that we actually ended up seeking to
		return i->first; //getKeyFrameLock();
	}

	//-------------------------------------------------------------//
	inline int intlog(int num) 
	{
		int r = 0;
		while( num > 0 )
		{
			num = num / 2;
			r = r + 1;
		}
		
		return r;
	}

	//-------------------------------------------------------------//	
	float TheoraSeekUtility::getKeyFrameLock()
	{
		return 0.0f;

		ogg_page page;
		char *buffer;
		int read = 1;
//		__int64 gran;
		int keyframe_granule_shift=intlog( 
			m_th_info->keyframe_frequency_force - 1 );

		while( !mStream->eof() && read != 0 )
		{
			buffer = ogg_sync_buffer( m_sync_state, 4096);
			read = mStream->read( buffer, 4096 );
			ogg_sync_wrote( m_sync_state, read );

			while ( ogg_sync_pageout( m_sync_state, &page ) > 0 )
			{
				//To Do // Search for keyframe --------
			}
		}
	}

	//-------------------------------------------------------------//
	float TheoraSeekUtility::buildTheoraSeekMap( )
	{
		if( SeekMap.size() > 0 )
			return 0.0f;

		ogg_page page;
		char *buffer;
		int read = 1;
		my_int64 gran;
		float movieLentgh = 0;
		unsigned int start = mStream->tell();
		unsigned int begin = start - 4096;	//To protect from the overlap between reading the header packet, 
											//and the first ogg_page after that

		int keyframe_granule_shift=intlog( 
			m_th_info->keyframe_frequency_force - 1 );

		while( !mStream->eof() && read != 0 )
		{
			buffer = ogg_sync_buffer( m_sync_state, 4096);
			read = mStream->read( buffer, 4096 );
			ogg_sync_wrote( m_sync_state, read );

			while ( ogg_sync_pageout( m_sync_state, &page ) > 0 )
			{
				int serno = ogg_page_serialno( &page );
				//This is theora stream we were searching for
				if( m_th_stream->serialno == serno )
				{
					//Calculate a rough time estimate
					gran = ogg_page_granulepos( &page );
					if( gran >= 0 )
					{
					    my_int64 iframe = gran >> keyframe_granule_shift;
						my_int64 pframe = gran	-(iframe << keyframe_granule_shift );
						movieLentgh = (iframe+pframe)*
							((double)m_th_info->fps_denominator / m_th_info->fps_numerator );
						
                        addSeekerPoint( movieLentgh, begin );
						begin = mStream->tell();
					}
				}
			}
		}

		ogg_sync_reset( m_sync_state );
		ogg_sync_clear( m_sync_state );
		ogg_sync_init(  m_sync_state );
		mStream->seek( start );
		
		//Lock on to key frame now that we have messed with sync's
		//doSeek( 0.0f );
		return movieLentgh;
	}
	
	//-------------------------------------------------------------//
	void TheoraSeekUtility::addSeekerPoint( float seconds, unsigned int byte )
	{
		SeekMap.insert( tSeeker::value_type( seconds, byte ) );
	}
}

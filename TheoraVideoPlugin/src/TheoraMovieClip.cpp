/*
-----------------------------------------------------------------------------
This source file is part of the TheoraVideoSystem ExternalTextureSource PlugIn 
for OGRE (Object-oriented Graphics Rendering Engine)
For the latest info, see www.wreckedgames.com or www.ogre3d.org
*****************************************************************************
				This PlugIn uses the following resources:

Ogre - see above
Ogg / Vorbis / Theora www.xiph.org
C++ Portable Types Library (PTypes - http://www.melikyan.com/ptypes/ )

*****************************************************************************
Copyright © 2000-2004 pjcast@yahoo.com

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License (LGPL) as published by the 
Free Software Foundation; either version 2 of the License, or (at your option) 
any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
***************************************************************************/
#include "OgreResourceGroupManager.h"
#include "OgreStringConverter.h"
#include "OgreLogManager.h"
#include "OgreException.h"
#include "OgreTimer.h"
#include "OgreRoot.h"

#include "TheoraMovieClip.h"

#include "TheoraAudioDriver.h"
#include "TheoraVideoDriver.h"
#include "TheoraVideoController.h"

namespace Ogre
{
	//--------------------------------------------------------------------//
	TheoraMovieClip::TheoraMovieClip() : 
		pt::thread( false ), 
		m_Dispatcher(0),
		m_AudioStarted( false ),
		mPlayMode(TextureEffectPause),
		m_FrameNum(0),
		m_FramesDropped(0),
		m_lastFrameTime(0.0f),
		m_audioInterface(0),
		m_ThreadRunning(false),
		m_EndOfAudio(false),
		m_EndOfVideo(false),
		m_EndOfFile(false),
		m_autoUpdate(false),
		m_theora_streams(0), 
		m_vorbis_streams(0),
		currentTicks(-1),	
		m_VideoFrameReady(false),
		videobuf_time(0.0f),
		audiobuf_granulepos(0),
		m_Timer(0),
		mSumDecoded(0),
		mSumYUVConverted(0),
		mSumBlited(0),
		mNumFramesEvaluated(0)
	{
		//Ensure all structures get cleared out. Already bit me in the arse ;)
		memset( &m_oggSyncState, 0, sizeof( ogg_sync_state ) );
		memset( &m_oggPage, 0, sizeof( ogg_page ) );
		memset( &m_vorbisStreamState, 0, sizeof( ogg_stream_state ) );
		memset( &m_theoraStreamState, 0, sizeof( ogg_stream_state ) );
		memset( &m_theoraInfo, 0, sizeof( theora_info ) );
		memset( &m_theoraComment, 0, sizeof( theora_comment ) );
		memset( &m_theoraState, 0, sizeof( theora_state ) );
		memset( &m_vorbisInfo, 0, sizeof( vorbis_info ) );
		memset( &m_vorbisDSPState, 0, sizeof( vorbis_dsp_state ) );
		memset( &m_vorbisBlock, 0, sizeof( vorbis_block ) );
		memset( &m_vorbisComment, 0, sizeof( vorbis_comment ) );

		mDoSeek = false;
		m_Seeker = 0;
	}

	//--------------------------------------------------------------------//
	TheoraMovieClip::~TheoraMovieClip()
	{
		changePlayMode( TextureEffectPause );
		if( m_ThreadRunning )
		{
			//Terminate Thread and wait for it to leave
			m_ThreadRunning = false;
			waitfor();
		}

		if( m_Timer )
			delete m_Timer;

		m_Timer = 0;
		
		close();
	}
	
	//--------------------------------------------------------------------//	
	void TheoraMovieClip::setAudioDriver( TheoraAudioDriver *pAud )
	{
		if( !m_ThreadRunning )
			m_audioInterface = pAud;
		else
			LogManager::getSingleton().logMessage("**** void TheoraMovieClip::set"
				"AudioDriver( TheoraAudioDriver *pAud )> Tried to set sound \n"
				"on already setup clip... Ignored...! ****");
	}
	
	//--------------------------------------------------------------------//	
	void TheoraMovieClip::createMovieClip( 
		const String &sMovieName, const String &sMaterialName,
		const String &sGroupName, int TechniqueLevel, int PassLevel,
		int TextureUnitStateLevel, bool HasSound, eTexturePlayMode eMode,
		TextureSpecialRenderFX renderMode, bool seekingEnabled,
		bool autoUpdateAudio )
	{
		mMovieName = sMovieName;
		mMaterialName = sMaterialName;
		m_autoUpdate = autoUpdateAudio;

		load( mMovieName, sGroupName, HasSound );
			
		//Build seeking data
		if( seekingEnabled )
			m_Seeker = new TheoraSeekUtility( m_oggFile, &m_oggSyncState,
				&m_theoraStreamState, &m_vorbisStreamState, &m_theoraInfo, &m_vorbisInfo,
				&m_theoraState, &m_vorbisDSPState, &m_vorbisBlock );

		// Attach our video to a texture unit
		m_videoInterface.attachVideoToTextureUnit( 
			sMaterialName, sMovieName, sGroupName, TechniqueLevel, 
			PassLevel, TextureUnitStateLevel, m_theoraInfo.width, 
			m_theoraInfo.height, renderMode );

		changePlayMode( eMode );
	}

	//--------------------------------------------------------------------//
	void TheoraMovieClip::load( const String& filename,
		const String& groupName, bool useAudio )
	{
		//ensure a file is not already open
		if( !(m_oggFile.isNull()) )
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "File name already loded! " + filename, "TheoraMovieClip::load" );
	
		m_EndOfFile = false;
		m_oggFile = ResourceGroupManager::getSingleton().openResource( filename, groupName );

		initVorbisTheoraLayer( );
		parseVorbisTheoraHeaders( useAudio );
		activateVorbisTheoraCodecs( useAudio );
	}
	
	//--------------------------------------------------------------------//
	void TheoraMovieClip::initVorbisTheoraLayer( )
	{
		//start up Ogg stream synchronization layer
		ogg_sync_init( &m_oggSyncState );
		//init supporting Theora structures needed in header parsing
		theora_comment_init(&m_theoraComment);
		theora_info_init(&m_theoraInfo);
		
		vorbis_info_init( &m_vorbisInfo );//init supporting Vorbis structures needed in header parsing
		vorbis_comment_init(&m_vorbisComment);
	}

	//--------------------------------------------------------------------//
	void TheoraMovieClip::parseVorbisTheoraHeaders( bool useAudio )
	{
		ogg_packet tempOggPacket;
		bool NotDone = true;

		while( NotDone )
		{
			char *buffer = ogg_sync_buffer( &m_oggSyncState, 4096);
			int bytesRead = m_oggFile->read( buffer, 4096 );
			ogg_sync_wrote( &m_oggSyncState, bytesRead );
		
			if( bytesRead == 0 )
				break;
		
			while( ogg_sync_pageout( &m_oggSyncState, &m_oggPage ) > 0 )
			{
				ogg_stream_state OggStateTest;
	    		
				//is this an initial header? If not, stop
				if( !ogg_page_bos( &m_oggPage ) )
				{
					//This is done blindly, because stream only accept them selfs
					if(m_theora_streams) 
						ogg_stream_pagein( &m_theoraStreamState, &m_oggPage );
					if(m_vorbis_streams) 
						ogg_stream_pagein( &m_vorbisStreamState, &m_oggPage );
					
					NotDone = false;
					break;
				}
		
				ogg_stream_init( &OggStateTest, ogg_page_serialno( &m_oggPage ) );
				ogg_stream_pagein( &OggStateTest, &m_oggPage );
				ogg_stream_packetout( &OggStateTest, &tempOggPacket );

				//identify the codec
				if( !m_theora_streams && 
					theora_decode_header( &m_theoraInfo, &m_theoraComment, &tempOggPacket) >=0 )
				{
					//This is the Theora Header
					memcpy( &m_theoraStreamState, &OggStateTest, sizeof(OggStateTest));
					m_theora_streams = 1;
				}
				else if( !m_vorbis_streams && useAudio &&
					vorbis_synthesis_headerin(&m_vorbisInfo, &m_vorbisComment, &tempOggPacket) >=0 )
				{
					//This is vorbis header
					memcpy( &m_vorbisStreamState, &OggStateTest, sizeof(OggStateTest));
					m_vorbis_streams = 1;
				}
				else
				{
					//Hmm. I guess it's not a header we support, so erase it
					ogg_stream_clear(&OggStateTest);
				}
			} //end while ogg_sync_pageout
		} //end while notdone

		while( (m_theora_streams && (m_theora_streams < 3)) ||
			   (m_vorbis_streams && (m_vorbis_streams < 3)) )
		{
			//Check 2nd'dary headers... Theora First
			int iSuccess;
			while( m_theora_streams && 
				 ( m_theora_streams < 3) && 
				 ( iSuccess = ogg_stream_packetout( &m_theoraStreamState, &tempOggPacket)) ) 
			{
				if( iSuccess < 0 ) 
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Error parsing Theora stream headers.",
						"TheoraMovieClip::parseVorbisTheoraHeaders" );

				if( theora_decode_header(&m_theoraInfo, &m_theoraComment, &tempOggPacket) )
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "invalid stream",
						"TheoraMovieClip::parseVorbisTheoraHeaders ");

				m_theora_streams++;			
			} //end while looking for more theora headers
		
			//look 2nd vorbis header packets
			while( m_vorbis_streams && 
				 ( m_vorbis_streams < 3 ) && 
				 ( iSuccess=ogg_stream_packetout( &m_vorbisStreamState, &tempOggPacket))) 
			{
				if(iSuccess < 0) 
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Error parsing vorbis stream headers",
						"TheoraMovieClip::parseVorbisTheoraHeaders ");

				if(vorbis_synthesis_headerin( &m_vorbisInfo, &m_vorbisComment,&tempOggPacket)) 
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "invalid stream",
						"TheoraMovieClip::parseVorbisTheoraHeaders ");

				m_vorbis_streams++;
			} //end while looking for more vorbis headers
		
			//Not finished with Headers, get some more file data
			if( ogg_sync_pageout( &m_oggSyncState, &m_oggPage ) > 0 )
			{
				if(m_theora_streams) 
					ogg_stream_pagein( &m_theoraStreamState, &m_oggPage );
				if(m_vorbis_streams) 
					ogg_stream_pagein( &m_vorbisStreamState, &m_oggPage );
			}
			else
			{
				char *buffer = ogg_sync_buffer( &m_oggSyncState, 4096);
				int bytesRead = m_oggFile->read( buffer, 4096 );
				ogg_sync_wrote( &m_oggSyncState, bytesRead );

				if( bytesRead == 0 )
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "End of file found prematurely",
						"TheoraMovieClip::parseVorbisTheoraHeaders " );
			}
		} //end while looking for all headers

		String temp1 = StringConverter::toString( m_vorbis_streams );
		String temp2 = StringConverter::toString( m_theora_streams );
		LogManager::getSingleton().logMessage("Vorbis Headers: " + temp1 + " Theora Headers : " + temp2);
	}

	//--------------------------------------------------------------------//
	void TheoraMovieClip::activateVorbisTheoraCodecs( bool useAudio )
	{
		if( m_theora_streams )
			theora_decode_init( &m_theoraState, &m_theoraInfo );

		if( m_vorbis_streams )
		{
			vorbis_synthesis_init( &m_vorbisDSPState, &m_vorbisInfo );
			vorbis_block_init( &m_vorbisDSPState, &m_vorbisBlock );  
		}
	}

	//--------------------------------------------------------------------//
	void TheoraMovieClip::changePlayMode( eTexturePlayMode eMode )
	{
		//TextureEffectPause = 0,			//! Video starts out paused
		//TextureEffectPlay_ASAP = 1,		//! Video starts playing as soon as posible
		//TextureEffectPlay_Looping = 2		//! Video Plays Instantly && Loops

		if( mPlayMode == eMode )
			return;

		//If current mode is paused than - sent mode must either be loop, or playASAP
		if( mPlayMode == TextureEffectPause )
		{
			if( m_ThreadRunning == false )
			{
				//Get audio setup and ready is we have vorbis and interface
				if( m_vorbis_streams && m_audioInterface )
					m_audioInterface->open( &m_vorbisInfo );

				//Set playmode for thread, and start it thread decoding
				mPlayMode = eMode;
				start();
			}
			else //Thread is already running, so change status only
			{
				//Resumed Paused Audio
				if( m_vorbis_streams && m_audioInterface )
					m_audioInterface->setAudioStreamPause( false );

				mPlayMode = eMode;
			}
		}
		else if( eMode == TextureEffectPause )
		{		
			if( m_vorbis_streams && m_audioInterface )
				m_audioInterface->setAudioStreamPause( true );
			//else
			//	mTimer->setPausedState( true ); //Pause our reference timer

			mPlayMode = eMode;
		}
	}
	
	//--------------------------------------------------------------------//	
	void TheoraMovieClip::blitFrameCheck()
	{
		if( m_VideoFrameReady )
		{
			int time=GetTickCount();
			yuv_buffer yuv;
			theora_decode_YUVout( &m_theoraState, &yuv);
			mDecodedTime+=GetTickCount()-time; // add buffer dumping to decoding time
			
			m_videoInterface.renderToTexture( &yuv );
			mYUVConvertTime=m_videoInterface.mYUVConvertTime;
			mBlitTime=m_videoInterface.mBlitTime;
			//m_videoInterface.randomizeTexture();
			
			m_VideoFrameReady = false;
			m_lastFrameTime = getMovieTime();
			float audTime = 0.0f;

			if( m_vorbis_streams && m_audioInterface )
			{
				if( m_AudioStarted == false )
				{
					m_audioInterface->startAudioStream();
					m_AudioStarted = true;
				}
				
				audTime = m_audioInterface->getAudioStreamTime() / 1000.0f;
			}

			if(m_Dispatcher)
			{
				TheoraMovieMessage::FrameInfo info;
				info.mAudioTime=0.0f; info.mVideoTime=videobuf_time; info.mCurrentFrame=m_FrameNum;
				info.mDecodeTime=mDecodedTime;  info.mYUVConvertTime=mYUVConvertTime;  info.mBlitTime=mBlitTime;
				
				mNumFramesEvaluated++;
				mSumDecoded+=mDecodedTime;  mSumYUVConverted+=mYUVConvertTime;  mSumBlited+=mBlitTime;
				info.mAvgDecodeTime=mSumDecoded/mNumFramesEvaluated;
				info.mAvgYUVConvertTime=mSumYUVConverted/mNumFramesEvaluated;
				info.mAvgBlitTime=mSumBlited/mNumFramesEvaluated;

				info.mNumFramesDropped=m_FramesDropped;
				m_Dispatcher->displayedFrame(info);
			}
			mDecodedTime=mBlitTime=mYUVConvertTime=0.0f; // reset
		}

		//If user requested that we update audio buffers
		if( m_autoUpdate && m_AudioStarted )
			m_audioInterface->autoUpdate();

		if( m_Dispatcher )
		{
			//Check for done playback
			if( m_audioInterface )
			{
				if( m_EndOfAudio && m_EndOfVideo )
					m_Dispatcher->messageEvent( TheoraMovieMessage::TH_EndOfMovie );
			}
			else if( m_EndOfVideo )
			{
				m_Dispatcher->messageEvent( TheoraMovieMessage::TH_EndOfMovie );			
			}
		}
	}

	//--------------------------------------------------------------------//
	void TheoraMovieClip::execute()
	{
		m_ThreadRunning = true;
		int bytesRead = 1;
		
		//Build seek map if seeking is enabled for this clip
		if( m_Seeker ) {
			mMovieLength = m_Seeker->buildTheoraSeekMap();
			if( m_Dispatcher )
				m_Dispatcher->discoveredMovieTime( mMovieLength );
		}

		while( m_ThreadRunning )
		{
			if( mPlayMode == TextureEffectPause ) {
				//If we are paused, just sleep a bit, and then check again
				//relax(30);
				pt::psleep(30);
				continue;
			}
			
			if( mDoSeek && m_Seeker )
			{
				//Handle the seeking request
				if( m_audioInterface )
					m_audioInterface->close();
				float seekedTo = m_Seeker->doSeek( mSeekTime );
				if( m_audioInterface )
					m_audioInterface->open( &m_vorbisInfo, seekedTo * 1000 );
				m_AudioStarted = false;
				mDoSeek = false;
				m_EndOfFile = false;
			}

			if( m_audioInterface && m_vorbis_streams )
				decodeVorbis();

			if( !m_VideoFrameReady && m_theora_streams )
				decodeTheora();

			//Buffer data into Ogg Pages
			if( bytesRead > 0 )
			{
				char *buffer = ogg_sync_buffer( &m_oggSyncState, 4096);
				bytesRead = m_oggFile->read( buffer, 4096 );
				ogg_sync_wrote( &m_oggSyncState, bytesRead );

				while ( ogg_sync_pageout( &m_oggSyncState, &m_oggPage ) > 0 )
				{
					if(m_theora_streams) 
						ogg_stream_pagein( &m_theoraStreamState, &m_oggPage );
					if(m_vorbis_streams) 
						ogg_stream_pagein( &m_vorbisStreamState, &m_oggPage );
				}
			}
			else
			{
				//End of file, but movie is still playing
				if( m_Dispatcher && m_EndOfFile == false )
					m_Dispatcher->messageEvent( TheoraMovieMessage::TH_OggStreamDone );
			
				m_EndOfFile = true;
			}

			//XXX: hmmm :?
			if( m_VideoFrameReady )	{
				int ticks = 1000.0f * ( videobuf_time - getMovieTime() );
				if(ticks > 0)
					pt::psleep(ticks);
					//relax(ticks);
			}
		} //while m_ThreadRunning
	}

	//--------------------------------------------------------------------//
	void TheoraMovieClip::decodeVorbis()
	{
		int ret, maxBytesToWrite = 0;
		float **pcm;
		ogg_packet opVorbis;

		//get some audio data
		for(;;)
		{
			//is there pending audio... Will it fit our circular buffer without blocking
			ret = vorbis_synthesis_pcmout( &m_vorbisDSPState, &pcm );
			maxBytesToWrite = m_audioInterface->getAudioStreamWriteable();

			//don't break out until there is a significant amount of
			//data to avoid a series of small write operations.
			if ( maxBytesToWrite <= FRAMES_PER_BUFFER )
				break;

			//if there's pending, decoded audio, grab it
			if(( ret > 0 ) && ( maxBytesToWrite > 0 ))
			{
				//XXX: opt -> converts to 16 bit Stereo PCM format
				int Writting = maxBytesToWrite / m_vorbisInfo.channels;
				int i = 0;
				//short *pData = m_audioInterface->samples;
				int count = 0;

				for( ; i < ret && i < Writting; i++ )
				{
					for(int j  =0; j < m_vorbisInfo.channels; j++)
					{
						int val=(int)(pcm[j][i]*32767.f);
						if(val>32767)	val=32767;
						if(val<-32768)	val=-32768;
			//			*pData = val;
						m_audioInterface->samples[count]=val;
						count++;
					}
				}

				m_audioInterface->writeAudioStream( i );

				//tell libvorbis how many samples we actually consumed
				vorbis_synthesis_read( &m_vorbisDSPState, i );

				if( m_vorbisDSPState.granulepos >= 0 )	
					audiobuf_granulepos = m_vorbisDSPState.granulepos - ret + i;
				else					
					audiobuf_granulepos += i;

			} //end if audio bytes
			else
			{
				//no pending audio; is there a pending packet to decode?
				if( ogg_stream_packetout( &m_vorbisStreamState, &opVorbis) >0 )
				{
					//test for success!
					if(vorbis_synthesis( &m_vorbisBlock, &opVorbis) == 0 )
						vorbis_synthesis_blockin( &m_vorbisDSPState, &m_vorbisBlock );
				}
				else	//we need more data; break out to suck in another page
				{
					if( m_EndOfFile )
					{
						if( m_Dispatcher && m_EndOfAudio == false )
							m_Dispatcher->messageEvent( TheoraMovieMessage::TH_VorbisStreamDone );
						
						m_EndOfAudio = true;
					}

					break;
				}
			} //end else: no audio bytes
		} //end audio cycle
	}

	//--------------------------------------------------------------------//
	void TheoraMovieClip::decodeTheora()
	{
		ogg_packet opTheora;
		long time=GetTickCount();
		for(;;)
		{

			//get one video packet...
			if( ogg_stream_packetout( &m_theoraStreamState, &opTheora) > 0 )
			{

      			theora_decode_packetin( &m_theoraState, &opTheora );
				mDecodedTime=GetTickCount()-time;
				videobuf_time = theora_granule_time( &m_theoraState, m_theoraState.granulepos );
				
				//update the frame counter
				m_FrameNum++;

				//check if this frame time has not passed yet.
				//If the frame is late we need to decode additonal
				//ones and keep looping, since theora at this stage
				//needs to decode all frames
				
				//gran time & our time is in seconds
				float nowTime = getMovieTime();
				float delay = videobuf_time - nowTime;
					
				if( delay >= 0.0f )
				{
					//got a good frame, within time window
					m_VideoFrameReady = true;
					break;
				}
				else if( nowTime - m_lastFrameTime >= 1.0f )
				{
					//display at least one frame per second, regardless
					m_VideoFrameReady = true;
					break;
				}
				else //frame is dropped
				{
					m_FramesDropped++;
				}
			}
			else
			{
				if( m_EndOfFile )
				{
					if( m_Dispatcher && m_EndOfVideo == false )
						m_Dispatcher->messageEvent( TheoraMovieMessage::TH_TheoraStreamDone );

					m_EndOfVideo = true;
				}


				//need more data
				break;
			}
		}
	}

	//--------------------------------------------------------------------//
	void TheoraMovieClip::cleanup()
	{
	}

	//--------------------------------------------------------------------//
	float TheoraMovieClip::getMovieTime() 
	{
		if ( m_audioInterface ) 
		{
			//We are using audio, so get the audio time as sync
			return m_audioInterface->getAudioStreamTime() / 1000.0f;
		}
		else
		{
			//We are not using audio, base our time off of an ogre clock
			if ( m_Timer )
			{
				return m_Timer->getMilliseconds() / 1000.0f;
			}
			else
			{
				//Initialize timer variable first time up
				m_Timer = new Timer();
				m_Timer->reset();
				return 0.0f;
			}
		}
	}

	//--------------------------------------------------------------------//
	void TheoraMovieClip::close()
	{
		m_EndOfFile = false;
		
		delete m_Seeker;
		m_Seeker = 0;

		m_oggFile.setNull();

		if(m_vorbis_streams)
		{
			if( m_audioInterface )
				m_audioInterface->close();

			ogg_stream_clear( &m_vorbisStreamState );
			vorbis_block_clear( &m_vorbisBlock );
			vorbis_dsp_clear( &m_vorbisDSPState );
			vorbis_comment_clear( &m_vorbisComment );
			vorbis_info_clear( &m_vorbisInfo ); 
			
			m_vorbis_streams = 0;
		}

		if(m_theora_streams)
		{
			ogg_stream_clear( &m_theoraStreamState );
			theora_clear( &m_theoraState );
			theora_comment_clear( &m_theoraComment );
			theora_info_clear( &m_theoraInfo );

			m_theora_streams = 0;
		}

		ogg_sync_clear( &m_oggSyncState );
	}

	void TheoraMovieClip::seekToTime( float seconds )
	{
		//No point seeking to the end.. were already there
		if( seconds >= mMovieLength || seconds < 0.0f )
			return;
		
		//If seeking is not enabled, just return
		if( !m_Seeker )
			return;

		//Sets seeking flag on for thread to know & carry out later
		mDoSeek = true;
		mSeekTime = seconds;
	}
} //end namespace Ogre

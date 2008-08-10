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
Copyright © 2008 Kresimir Spes (kreso@cateia.com)
          © 2000-2004 pjcast@yahoo.com

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
	TheoraFrame::TheoraFrame(TheoraMovieClip* parent,int w,int h)
	{
		mPixelBuffer=new unsigned char[w*h*4];
		mInUse=false;
		mParent=parent;
	}
	TheoraFrame::~TheoraFrame()
	{
		delete mPixelBuffer;
	}

	void TheoraFrame::copyYUV(yuv_buffer yuv,double timeToDisplay,bool convert_to_rgb)
	{
		mTimeToDisplay=timeToDisplay;
		mParent->getVideoDriver()->decodeYUVtoTexture(&yuv,mPixelBuffer);
		//yuvToRGB(yuv,mPixelBuffer);
		mInUse=true;
	}





	//--------------------------------------------------------------------//
	TheoraMovieClip::TheoraMovieClip() : 
		pt::thread( false ), 
		mMessageListener(0),
		mAudioStarted( false ),
		mPlayMode(TextureEffectPause),
		mFrameNum(0),
		mNumDroppedFrames(0),
		mLastFrameTime(0.0f),
		mAudioInterface(0),
		mThreadRunning(false),
		mEndOfAudio(false),
		mEndOfVideo(false),
		mEndOfFile(false),
		mAutoUpdate(false),
		mTheoraStreams(0), 
		mVorbisStreams(0),
		currentTicks(-1),	
		mVideoFrameReady(false),
		videobuf_time(0.0f),
		audiobuf_granulepos(0),
		mTimer(0),
		mAvgDecodedTime(0),
		mAvgYUVConvertedTime(0),
		mSumBlited(0),
		mNumFramesEvaluated(0),
		mFramesReady(false)
	{
		//Ensure all structures get cleared out. Already bit me in the arse ;)
		memset( &mOggSyncState, 0, sizeof( ogg_sync_state ) );
		memset( &mOggPage, 0, sizeof( ogg_page ) );
		memset( &mVorbisStreamState, 0, sizeof( ogg_stream_state ) );
		memset( &mTheoraStreamState, 0, sizeof( ogg_stream_state ) );
		memset( &mTheoraInfo, 0, sizeof( theora_info ) );
		memset( &mTheoraComment, 0, sizeof( theora_comment ) );
		memset( &mTheoraState, 0, sizeof( theora_state ) );
		memset( &mVorbisInfo, 0, sizeof( vorbis_info ) );
		memset( &mVorbisDSPState, 0, sizeof( vorbis_dsp_state ) );
		memset( &mVorbisBlock, 0, sizeof( vorbis_block ) );
		memset( &mVorbisComment, 0, sizeof( vorbis_comment ) );

		mDoSeek = false;
		mSeeker = 0;
	}

	//--------------------------------------------------------------------//
	TheoraMovieClip::~TheoraMovieClip()
	{
		changePlayMode( TextureEffectPause );
		if( mThreadRunning )
		{
			//Terminate Thread and wait for it to leave
			mThreadRunning = false;
			waitfor();
		}

		if( mTimer )
			delete mTimer;

		mTimer = 0;
		
		close();
	}
	
	//--------------------------------------------------------------------//	
	void TheoraMovieClip::setAudioDriver( TheoraAudioDriver *pAud )
	{
		if( !mThreadRunning )
			mAudioInterface = pAud;
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
		bool seekingEnabled,
		bool autoUpdateAudio )
	{
		mMovieName = sMovieName;
		mMaterialName = sMaterialName;
		mAutoUpdate = autoUpdateAudio;

		load( mMovieName, sGroupName, HasSound );
			
		//Build seeking data
		if( seekingEnabled )
			mSeeker = new TheoraSeekUtility( mOggFile, &mOggSyncState,
				&mTheoraStreamState, &mVorbisStreamState, &mTheoraInfo, &mVorbisInfo,
				&mTheoraState, &mVorbisDSPState, &mVorbisBlock );

		// Attach our video to a texture unit
		mVideoInterface.attachVideoToTextureUnit( 
			sMaterialName, sMovieName, sGroupName, TechniqueLevel, 
			PassLevel, TextureUnitStateLevel, mTheoraInfo.width, 
			mTheoraInfo.height );

		changePlayMode( eMode );
	}

	//--------------------------------------------------------------------//
	void TheoraMovieClip::setNumPrecachedFrames(int num)
	{
		TheoraFrame* frame;
		// clear current frame repository (if any)
		while (mFrameRepository.size())
		{
			frame=mFrameRepository.front();
			delete frame;
			mFrameRepository.pop_back();
		}

		for (int i=0;i<num;i++)
		{
			frame=new TheoraFrame(this,mVideoInterface.getWidth(),mVideoInterface.getHeight());
			mFrameRepository.push_back(frame);
		}
	}


	//--------------------------------------------------------------------//
	void TheoraMovieClip::load( const String& filename,
		const String& groupName, bool useAudio )
	{
		//ensure a file is not already open
		if( !(mOggFile.isNull()) )
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "File name already loded! " + filename, "TheoraMovieClip::load" );
	
		mEndOfFile = false;
		mOggFile = ResourceGroupManager::getSingleton().openResource( filename, groupName );

		initVorbisTheoraLayer( );
		parseVorbisTheoraHeaders( useAudio );
		activateVorbisTheoraCodecs( useAudio );
	}
	
	//--------------------------------------------------------------------//
	void TheoraMovieClip::initVorbisTheoraLayer( )
	{
		//start up Ogg stream synchronization layer
		ogg_sync_init( &mOggSyncState );
		//init supporting Theora structures needed in header parsing
		theora_comment_init(&mTheoraComment);
		theora_info_init(&mTheoraInfo);
		
		vorbis_info_init( &mVorbisInfo );//init supporting Vorbis structures needed in header parsing
		vorbis_comment_init(&mVorbisComment);
	}

	//--------------------------------------------------------------------//
	void TheoraMovieClip::parseVorbisTheoraHeaders( bool useAudio )
	{
		ogg_packet tempOggPacket;
		bool NotDone = true;

		while( NotDone )
		{
			char *buffer = ogg_sync_buffer( &mOggSyncState, 4096);
			int bytesRead = mOggFile->read( buffer, 4096 );
			ogg_sync_wrote( &mOggSyncState, bytesRead );
		
			if( bytesRead == 0 )
				break;
		
			while( ogg_sync_pageout( &mOggSyncState, &mOggPage ) > 0 )
			{
				ogg_stream_state OggStateTest;
	    		
				//is this an initial header? If not, stop
				if( !ogg_page_bos( &mOggPage ) )
				{
					//This is done blindly, because stream only accept them selfs
					if(mTheoraStreams) 
						ogg_stream_pagein( &mTheoraStreamState, &mOggPage );
					if(mVorbisStreams) 
						ogg_stream_pagein( &mVorbisStreamState, &mOggPage );
					
					NotDone = false;
					break;
				}
		
				ogg_stream_init( &OggStateTest, ogg_page_serialno( &mOggPage ) );
				ogg_stream_pagein( &OggStateTest, &mOggPage );
				ogg_stream_packetout( &OggStateTest, &tempOggPacket );

				//identify the codec
				if( !mTheoraStreams && 
					theora_decode_header( &mTheoraInfo, &mTheoraComment, &tempOggPacket) >=0 )
				{
					//This is the Theora Header
					memcpy( &mTheoraStreamState, &OggStateTest, sizeof(OggStateTest));
					mTheoraStreams = 1;
				}
				else if( !mVorbisStreams && useAudio &&
					vorbis_synthesis_headerin(&mVorbisInfo, &mVorbisComment, &tempOggPacket) >=0 )
				{
					//This is vorbis header
					memcpy( &mVorbisStreamState, &OggStateTest, sizeof(OggStateTest));
					mVorbisStreams = 1;
				}
				else
				{
					//Hmm. I guess it's not a header we support, so erase it
					ogg_stream_clear(&OggStateTest);
				}
			} //end while ogg_sync_pageout
		} //end while notdone

		while( (mTheoraStreams && (mTheoraStreams < 3)) ||
			   (mVorbisStreams && (mVorbisStreams < 3)) )
		{
			//Check 2nd'dary headers... Theora First
			int iSuccess;
			while( mTheoraStreams && 
				 ( mTheoraStreams < 3) && 
				 ( iSuccess = ogg_stream_packetout( &mTheoraStreamState, &tempOggPacket)) ) 
			{
				if( iSuccess < 0 ) 
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Error parsing Theora stream headers.",
						"TheoraMovieClip::parseVorbisTheoraHeaders" );

				if( theora_decode_header(&mTheoraInfo, &mTheoraComment, &tempOggPacket) )
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "invalid stream",
						"TheoraMovieClip::parseVorbisTheoraHeaders ");

				mTheoraStreams++;			
			} //end while looking for more theora headers
		
			//look 2nd vorbis header packets
			while( mVorbisStreams && 
				 ( mVorbisStreams < 3 ) && 
				 ( iSuccess=ogg_stream_packetout( &mVorbisStreamState, &tempOggPacket))) 
			{
				if(iSuccess < 0) 
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Error parsing vorbis stream headers",
						"TheoraMovieClip::parseVorbisTheoraHeaders ");

				if(vorbis_synthesis_headerin( &mVorbisInfo, &mVorbisComment,&tempOggPacket)) 
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "invalid stream",
						"TheoraMovieClip::parseVorbisTheoraHeaders ");

				mVorbisStreams++;
			} //end while looking for more vorbis headers
		
			//Not finished with Headers, get some more file data
			if( ogg_sync_pageout( &mOggSyncState, &mOggPage ) > 0 )
			{
				if(mTheoraStreams) 
					ogg_stream_pagein( &mTheoraStreamState, &mOggPage );
				if(mVorbisStreams) 
					ogg_stream_pagein( &mVorbisStreamState, &mOggPage );
			}
			else
			{
				char *buffer = ogg_sync_buffer( &mOggSyncState, 4096);
				int bytesRead = mOggFile->read( buffer, 4096 );
				ogg_sync_wrote( &mOggSyncState, bytesRead );

				if( bytesRead == 0 )
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "End of file found prematurely",
						"TheoraMovieClip::parseVorbisTheoraHeaders " );
			}
		} //end while looking for all headers

		String temp1 = StringConverter::toString( mVorbisStreams );
		String temp2 = StringConverter::toString( mTheoraStreams );
		LogManager::getSingleton().logMessage("Vorbis Headers: " + temp1 + " Theora Headers : " + temp2);
	}

	//--------------------------------------------------------------------//
	void TheoraMovieClip::activateVorbisTheoraCodecs( bool useAudio )
	{
		if( mTheoraStreams )
			theora_decode_init( &mTheoraState, &mTheoraInfo );

		if( mVorbisStreams )
		{
			vorbis_synthesis_init( &mVorbisDSPState, &mVorbisInfo );
			vorbis_block_init( &mVorbisDSPState, &mVorbisBlock );  
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
			if( mThreadRunning == false )
			{
				//Get audio setup and ready is we have vorbis and interface
				if( mVorbisStreams && mAudioInterface )
					mAudioInterface->open( &mVorbisInfo );

				//Set playmode for thread, and start it thread decoding
				mPlayMode = eMode;
				start();
			}
			else //Thread is already running, so change status only
			{
				//Resumed Paused Audio
				if( mVorbisStreams && mAudioInterface )
					mAudioInterface->setAudioStreamPause( false );

				mPlayMode = eMode;
			}
		}
		else if( eMode == TextureEffectPause )
		{		
			if( mVorbisStreams && mAudioInterface )
				mAudioInterface->setAudioStreamPause( true );
			//else
			//	mTimer->setPausedState( true ); //Pause our reference timer

			mPlayMode = eMode;
		}
	}
	
	//--------------------------------------------------------------------//	
	void TheoraMovieClip::blitFrameCheck()
	{
		if( mFramesReady )
		{
			
			double nowTime=getMovieTime();
			TheoraFrame* frame;
			
			// we go through the list of available frames and try to find one suitable for presentation
			// if a frame's time to display has ended, it will be dropped, which will hopefully happen
			// rearely.
			// if all frames are dropped, the last still get's displayed.

			if (mTimeOfNextFrame > nowTime) return;

			mFrameMutex.lock();
			while (mFrames.size())
			{
				frame=mFrames.front();
				//if (frame->mDisplayed && frame->mTimeToDisplay > nowTime) return;

				mFrames.pop();
				if (frame->mTimeToDisplay < nowTime)
				{
					if (mFrames.size() > 0) frame->mInUse=false;
					mNumDroppedFrames++;
				}
				else break;
			}
				
			if (mFrames.size() == 0) mFramesReady=false;
			mFrameMutex.unlock();
			
			Ogre::Timer timer;
			mVideoInterface.renderToTexture( frame->mPixelBuffer );
			// benchmarking
			mSumBlited+=timer.getMilliseconds();
			mNumFramesEvaluated++;

			mTimeOfNextFrame=frame->mTimeToDisplay;
			frame->mInUse=false;

			mVideoFrameReady = false;
			mLastFrameTime = getMovieTime();
			float audTime = 0.0f;

			if (mVorbisStreams && mAudioInterface)
			{
				if( mAudioStarted == false )
				{
					mAudioInterface->startAudioStream();
					mAudioStarted = true;
				}
				
				audTime = mAudioInterface->getAudioStreamTime() / 1000.0f;
			}

			if (mMessageListener)
			{
				TheoraMovieMessage::FrameInfo info;
				info.mAudioTime=0.0f; info.mVideoTime=videobuf_time; info.mCurrentFrame=mFrameNum;
				
				info.mAvgDecodeTime=mAvgDecodedTime;
				info.mAvgYUVConvertTime=mAvgYUVConvertedTime;
				info.mAvgBlitTime=mSumBlited/mNumFramesEvaluated;
				info.mNumPrecachedFrames=mFrames.size();
				info.mNumFramesDropped=mNumDroppedFrames;
				mMessageListener->displayedFrame(info);
			}
		}

		//If user requested that we update audio buffers
		if( mAutoUpdate && mAudioStarted )
			mAudioInterface->autoUpdate();

		if( mMessageListener )
		{
			//Check for done playback
			if( mAudioInterface )
			{
				if( mEndOfAudio && mEndOfVideo )
					mMessageListener->messageEvent( TheoraMovieMessage::TH_EndOfMovie );
			}
			else if( mEndOfVideo )
			{
				mMessageListener->messageEvent( TheoraMovieMessage::TH_EndOfMovie );			
			}
		}
	}

	//--------------------------------------------------------------------//
	void TheoraMovieClip::execute()
	{
		mThreadRunning = true;
		int bytesRead = 1;
		TheoraFrame* frame;
		std::list<TheoraFrame*>::iterator it;
		yuv_buffer yuv; // holds theora yuv buffer

		int numFramesEvaluated=0; // for calculating average values
		int nDropped=0,decodingTime=0;
		float sumDecoded=0,sumYUV=0;
		double frame_time; // holds granule time
		Ogre::Timer timer;

		
		
		//Build seek map if seeking is enabled for this clip
		if( mSeeker ) {
			mMovieLength = mSeeker->buildTheoraSeekMap();
			if( mMessageListener )
				mMessageListener->discoveredMovieTime( mMovieLength );
		}

		// init our frame repository
		setNumPrecachedFrames(16);
		timer.reset();
		while( mThreadRunning )
		{
			if (mPlayMode == TextureEffectPause)
			{
				//If we are paused, just sleep a bit, and then check again
				//relax(30);
				pt::psleep(30);
				continue;
			}
			
			if (mDoSeek && mSeeker)
			{
				//Handle the seeking request
				if( mAudioInterface )
					mAudioInterface->close();
				float seekedTo = mSeeker->doSeek( mSeekTime );
				if( mAudioInterface )
					mAudioInterface->open( &mVorbisInfo, seekedTo * 1000 );
				mAudioStarted = false;
				mDoSeek = false;
				mEndOfFile = false;
			}
			// find an empty frame page
			for (it=mFrameRepository.begin();it!=mFrameRepository.end();it++)
			{
				frame=*it;
				if (!frame->mInUse) break;
			}
			// if frame not found (max number of precached frames reached), sleep a bit and continue
			if (it == mFrameRepository.end())
			{
				pt::psleep(10);
				continue;
			}

			if (mAudioInterface && mVorbisStreams)
				decodeVorbis();

			if (!mVideoFrameReady && mTheoraStreams)
			{
				int nDropped=mNumDroppedFrames;
				timer.reset();
				decodeTheora();
				decodingTime+=timer.getMilliseconds();
				
			}

			if (mVideoFrameReady)
			{
				numFramesEvaluated++; timer.reset(); // inc number of evaluated frame, for benchmarking

				theora_decode_YUVout( &mTheoraState, &yuv);

				// calculate number of dropped frames to get the actual decoding time
				nDropped=(mNumDroppedFrames-nDropped) ? mNumDroppedFrames-nDropped : 1;
				sumDecoded+=timer.getMilliseconds()+(float) decodingTime/nDropped;
				mAvgDecodedTime=sumDecoded/numFramesEvaluated;
				decodingTime=0; nDropped=mNumDroppedFrames;

				frame_time = theora_granule_time( &mTheoraState, mTheoraState.granulepos );
				timer.reset();
				frame->copyYUV(yuv,frame_time);
				sumYUV+=timer.getMilliseconds();
				mAvgYUVConvertedTime=sumYUV/numFramesEvaluated;


				mFrameMutex.lock();
				if (mFrames.size() == 0) mFramesReady=true;
				mFrames.push(frame);
				mFrameMutex.unlock();
				mVideoFrameReady=false;
				timer.reset();
				continue;
			}
			//Buffer data into Ogg Pages
			if( bytesRead > 0 )
			{
				char *buffer = ogg_sync_buffer( &mOggSyncState, 4096);
				bytesRead = mOggFile->read( buffer, 4096 );
				ogg_sync_wrote( &mOggSyncState, bytesRead );

				while ( ogg_sync_pageout( &mOggSyncState, &mOggPage ) > 0 )
				{
					if(mTheoraStreams) 
						ogg_stream_pagein( &mTheoraStreamState, &mOggPage );
					if(mVorbisStreams) 
						ogg_stream_pagein( &mVorbisStreamState, &mOggPage );
				}
			}
			else
			{
				//End of file, but movie is still playing
				if( mMessageListener && mEndOfFile == false )
					mMessageListener->messageEvent( TheoraMovieMessage::TH_OggStreamDone );
			
				mEndOfFile = true;
			}

			//XXX: hmmm :?
			/*
			if( mVideoFrameReady )	{
				int ticks = 1000.0f * ( videobuf_time - getMovieTime() );
				if(ticks > 0)
					pt::psleep(ticks);
					//relax(ticks);
			}
			*/
		} //while mThreadRunning
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
			ret = vorbis_synthesis_pcmout( &mVorbisDSPState, &pcm );
			maxBytesToWrite = mAudioInterface->getAudioStreamWriteable();

			//don't break out until there is a significant amount of
			//data to avoid a series of small write operations.
			if ( maxBytesToWrite <= FRAMES_PER_BUFFER )
				break;

			//if there's pending, decoded audio, grab it
			if(( ret > 0 ) && ( maxBytesToWrite > 0 ))
			{
				//XXX: opt -> converts to 16 bit Stereo PCM format
				int Writting = maxBytesToWrite / mVorbisInfo.channels;
				int i = 0;
				//short *pData = mAudioInterface->samples;
				int count = 0;

				for( ; i < ret && i < Writting; i++ )
				{
					for(int j  =0; j < mVorbisInfo.channels; j++)
					{
						int val=(int)(pcm[j][i]*32767.f);
						if(val>32767)	val=32767;
						if(val<-32768)	val=-32768;
			//			*pData = val;
						mAudioInterface->samples[count]=val;
						count++;
					}
				}

				mAudioInterface->writeAudioStream( i );

				//tell libvorbis how many samples we actually consumed
				vorbis_synthesis_read( &mVorbisDSPState, i );

				if( mVorbisDSPState.granulepos >= 0 )	
					audiobuf_granulepos = mVorbisDSPState.granulepos - ret + i;
				else					
					audiobuf_granulepos += i;

			} //end if audio bytes
			else
			{
				//no pending audio; is there a pending packet to decode?
				if( ogg_stream_packetout( &mVorbisStreamState, &opVorbis) >0 )
				{
					//test for success!
					if(vorbis_synthesis( &mVorbisBlock, &opVorbis) == 0 )
						vorbis_synthesis_blockin( &mVorbisDSPState, &mVorbisBlock );
				}
				else	//we need more data; break out to suck in another page
				{
					if( mEndOfFile )
					{
						if( mMessageListener && mEndOfAudio == false )
							mMessageListener->messageEvent( TheoraMovieMessage::TH_VorbisStreamDone );
						
						mEndOfAudio = true;
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
		float nowTime,delay;
		for(;;)
		{

			//get one video packet...
			if( ogg_stream_packetout( &mTheoraStreamState, &opTheora) > 0 )
			{
				
      			theora_decode_packetin( &mTheoraState, &opTheora );
				videobuf_time = theora_granule_time( &mTheoraState, mTheoraState.granulepos );
				
				//update the frame counter
				mFrameNum++;

				//check if this frame time has not passed yet.
				//If the frame is late we need to decode additonal
				//ones and keep looping, since theora at this stage
				//needs to decode all frames
				
				//if there are no frames in queue, first let's see if the frame we just decoded
				//should even be displayed. but, display at least one frame per second
				nowTime=getMovieTime();
				delay=videobuf_time - nowTime;
				if (delay >= 0.0f || delay < -1.0f)
				{
					//got a good frame, within time window
					mVideoFrameReady = true;
					break;
				}
				else //frame is dropped
				{
					mNumDroppedFrames++;
				}
			}
			else
			{
				if( mEndOfFile )
				{
					if( mMessageListener && mEndOfVideo == false )
						mMessageListener->messageEvent( TheoraMovieMessage::TH_TheoraStreamDone );

					mEndOfVideo = true;
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
		if ( mAudioInterface ) 
		{
			//We are using audio, so get the audio time as sync
			return mAudioInterface->getAudioStreamTime() / 1000.0f;
		}
		else
		{
			//We are not using audio, base our time off of an ogre clock
			if ( mTimer )
			{
				return mTimer->getMilliseconds() / 1000.0f;
			}
			else
			{
				//Initialize timer variable first time up
				mTimer = new Timer();
				mTimer->reset();
				mTimeOfNextFrame=-1.0f;
				return 0.0f;
			}
		}
	}

	//--------------------------------------------------------------------//
	void TheoraMovieClip::close()
	{
		mEndOfFile = false;
		
		delete mSeeker;
		mSeeker = 0;

		mOggFile.setNull();

		if(mVorbisStreams)
		{
			if( mAudioInterface )
				mAudioInterface->close();

			ogg_stream_clear( &mVorbisStreamState );
			vorbis_block_clear( &mVorbisBlock );
			vorbis_dsp_clear( &mVorbisDSPState );
			vorbis_comment_clear( &mVorbisComment );
			vorbis_info_clear( &mVorbisInfo ); 
			
			mVorbisStreams = 0;
		}

		if(mTheoraStreams)
		{
			ogg_stream_clear( &mTheoraStreamState );
			theora_clear( &mTheoraState );
			theora_comment_clear( &mTheoraComment );
			theora_info_clear( &mTheoraInfo );

			mTheoraStreams = 0;
		}

		ogg_sync_clear( &mOggSyncState );
	}

	void TheoraMovieClip::seekToTime( float seconds )
	{
		//No point seeking to the end.. were already there
		if( seconds >= mMovieLength || seconds < 0.0f )
			return;
		
		//If seeking is not enabled, just return
		if( !mSeeker )
			return;

		//Sets seeking flag on for thread to know & carry out later
		mDoSeek = true;
		mSeekTime = seconds;
	}
} //end namespace Ogre

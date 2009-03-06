/************************************************************************************
This source file is part of the TheoraVideoPlugin ExternalTextureSource PlugIn 
for OGRE3D (Object-oriented Graphics Rendering Engine)
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright © 2008-2009 Kresimir Spes (kreso@cateia.com)

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
*************************************************************************************/
#include "OgreTextureManager.h"
#include "OgreMaterialManager.h"
#include "OgreMaterial.h"
#include "OgreTechnique.h"
#include "OgreStringConverter.h"
#include "OgreLogManager.h"
#include "TheoraVideoClip.h"
#include "TheoraVideoFrame.h"
#include "TheoraFrameQueue.h"
#include "OgreHardwarePixelBuffer.h"

namespace Ogre
{
	int nextPow2(int x)
	{
		int y;
		for (y=1;y<x;y*=2);
		return y;

	}
	//! clears a portion of memory with an unsign
	void memset_uint(void* buffer,unsigned int colour,unsigned int size_in_bytes)
	{
		unsigned int* data=(unsigned int*) buffer;
		for (unsigned int i=0;i<size_in_bytes;i+=4)
		{
			*data=colour;
			data++;
		}
	}

	TheoraVideoClip::TheoraVideoClip(std::string name,int nPrecachedFrames):
		mTheoraStreams(0),
		mVorbisStreams(0),
		mTimePos(0),
		mPaused(false),
		mName(name),
		mOutputMode(TH_RGB),
		mBackColourChanged(0)
	{
		mFrameQueue=NULL;
		mAssignedWorkerThread=NULL;
		mNumPrecachedFrames=nPrecachedFrames;

		//Ensure all structures get cleared out.
		memset(&mOggSyncState, 0, sizeof(ogg_sync_state));
		memset(&mOggPage, 0, sizeof(ogg_page));
		memset(&mVorbisStreamState, 0, sizeof(ogg_stream_state));
		memset(&mTheoraStreamState, 0, sizeof(ogg_stream_state));
		memset(&mTheoraInfo, 0, sizeof(th_info));
		memset(&mTheoraComment, 0, sizeof(th_comment));
		//memset(&mTheoraState, 0, sizeof(th_state));
		memset(&mVorbisInfo, 0, sizeof(vorbis_info));
		memset(&mVorbisDSPState, 0, sizeof(vorbis_dsp_state));
		memset(&mVorbisBlock, 0, sizeof(vorbis_block));
		memset(&mVorbisComment, 0, sizeof(vorbis_comment));

		mTheoraSetup=NULL;
	}

	TheoraVideoClip::~TheoraVideoClip()
	{
	
	}

	String TheoraVideoClip::getMaterialName()
	{
		return "";
	}

	void TheoraVideoClip::decodeNextFrame()
	{
		TheoraVideoFrame* frame=mFrameQueue->requestEmptyFrame();
		if (!frame) return; // max number of precached frames reached

		ogg_packet opTheora;
		ogg_int64_t granulePos;
		th_ycbcr_buffer buff;
		for(;;)
		{
			if (ogg_stream_packetout(&mTheoraStreamState,&opTheora) > 0)
			{
				th_decode_packetin(mTheoraDecoder, &opTheora,&granulePos );
				float time=th_granule_time(mTheoraDecoder,granulePos);
				if (time < mTimePos) continue; // drop frame
				frame->mTimeToDisplay=time;
				th_decode_ycbcr_out(mTheoraDecoder,buff);
				frame->decode(buff);
				break;
			}
			else
			{
				char *buffer = ogg_sync_buffer( &mOggSyncState, 4096);
				int bytesRead = mStream->read( buffer, 4096 );
				ogg_sync_wrote( &mOggSyncState, bytesRead );

				while ( ogg_sync_pageout( &mOggSyncState, &mOggPage ) > 0 )
				{
					if(mTheoraStreams) 
						ogg_stream_pagein( &mTheoraStreamState, &mOggPage );
					if(mVorbisStreams) 
						ogg_stream_pagein( &mVorbisStreamState, &mOggPage );
				}
			}
		}
	}

	void TheoraVideoClip::blitFrameCheck(float time_increase)
	{
		if (mPaused) return;
		mTimePos+=time_increase;
		TheoraVideoFrame* frame;
		while (true)
		{
			frame=mFrameQueue->getFirstAvailableFrame();
			if (!frame) return; // no frames ready
			if (frame->mTimeToDisplay > mTimePos) return;
			if (frame->mTimeToDisplay < mTimePos-0.1)
			{
				mFrameQueue->pop();
			}
			else break;
		}

		// use blitFromMemory or smtg faster
		unsigned char* texData=(unsigned char*) mTexture->getBuffer()->lock(HardwareBuffer::HBL_DISCARD);
		memcpy(texData,frame->getBuffer(),mTexWidth*mHeight*4);
		if (mBackColourChanged)
		{
			memset_uint(texData+mTexWidth*mHeight*4,mFrameQueue->getBackColour(),mTexWidth*(mTexHeight-mHeight)*4);
			mBackColourChanged=false;
		}

		mTexture->getBuffer()->unlock();

		mFrameQueue->pop(); // after transfering frame data to the texture, free the frame
		                    // so it can be used again
	}

	void TheoraVideoClip::createDefinedTexture(const String& name, const String& material_name,
                              const String& group_name, int technique_level, int pass_level, 
			                  int tex_level)
	{
		mName=name;
		load(name,group_name);

		mMaterialName=material_name;
		mTechniqueLevel=technique_level;
		mPassLevel=pass_level;
		mTexLevel=tex_level;
		// create texture
		mTexture = TextureManager::getSingleton().createManual(mName,group_name,TEX_TYPE_2D,
			mTexWidth,mTexHeight,1,0,PF_X8R8G8B8,TU_DYNAMIC_WRITE_ONLY);
		// clear it to black
		unsigned char* texData=(unsigned char*) mTexture->getBuffer()->lock(HardwareBuffer::HBL_DISCARD);
		if (mOutputMode == TH_YUV)
			memset_uint(texData,0xFF008080,mTexWidth*mTexHeight*4); // (0,128,128) is YUV->RGB for Black
		else
			memset(texData,0,mTexWidth*mTexHeight*4);

		mTexture->getBuffer()->unlock();

		// attach it to a material
		MaterialPtr material = MaterialManager::getSingleton().getByName(mMaterialName);
		TextureUnitState* t = material->getTechnique(mTechniqueLevel)->\
			getPass(mPassLevel)->getTextureUnitState(mTexLevel);

		//Now, attach the texture to the material texture unit (single layer) and setup properties
		t->setTextureName(mName,TEX_TYPE_2D);
		t->setTextureFiltering(FO_LINEAR, FO_LINEAR, FO_NONE);
		t->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);

		// scale tex coords to fit the 0-1 uv range
		Matrix4 mat=Matrix4::IDENTITY;
		mat.setScale(Vector3((float) mWidth/mTexWidth, (float) mHeight/mTexHeight,1));
		t->setTextureTransform(mat);
	}

	void TheoraVideoClip::load(const String& file_name,const String& group_name)
	{
		if (!(mStream.isNull()))
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "ogg_video "+file_name+" already loded!",
			             "TheoraVideoClip::load" );
	
		//mEndOfFile = false;
		mStream = ResourceGroupManager::getSingleton().openResource( file_name, group_name );


		readTheoraVorbisHeaders();


		/* 


		ogg_page page;
		char *buffer;
		int read = 1;
		long gran;
		float movieLentgh = 0;
		unsigned int start = mStream->tell();
		unsigned int begin = start - 4096;	//To protect from the overlap between reading the header packet, 
											//and the first ogg_page after that

		int keyframe_granule_shift=mTheoraInfo.keyframe_granule_shift;

		while( !mStream->eof() && read != 0 )
		{
			buffer = ogg_sync_buffer( &mOggSyncState, 4096);
			read = mStream->read( buffer, 4096 );
			ogg_sync_wrote( &mOggSyncState, read );

			while ( ogg_sync_pageout( &mOggSyncState, &page ) > 0 )
			{
				int serno = ogg_page_serialno( &page );
				//This is theora stream we were searching for
				if( mTheoraStreamState.serialno == serno )
				{
					//Calculate a rough time estimate
					gran = ogg_page_granulepos( &page );
					if( gran >= 0 )
					{
					    long iframe = gran >> keyframe_granule_shift;
						long pframe = gran	-(iframe << keyframe_granule_shift );
						movieLentgh = (iframe+pframe)*
							((double)mTheoraInfo.fps_denominator / mTheoraInfo.fps_numerator );
						
                       // addSeekerPoint( movieLentgh, begin );
						begin = mStream->tell();
					}
				}
			}
		}

		ogg_sync_reset( &mOggSyncState );
		ogg_sync_clear( &mOggSyncState );
		ogg_sync_init(  &mOggSyncState );
		mStream->seek( start );





		*/







		mTheoraDecoder=th_decode_alloc(&mTheoraInfo,mTheoraSetup);

		mWidth=mTheoraInfo.frame_width;
		mHeight=mTheoraInfo.frame_height;
		mTexWidth = nextPow2(mWidth);
		mTexHeight= nextPow2(mHeight);

		mFrameQueue=new TheoraFrameQueue(mNumPrecachedFrames,this);
		setOutputMode(mOutputMode); // clear the frame backgrounds
	}

	void TheoraVideoClip::readTheoraVorbisHeaders()
	{
		ogg_packet tempOggPacket;
		bool done = false;
		//init Vorbis/Theora Layer
		ogg_sync_init(&mOggSyncState);
		th_comment_init(&mTheoraComment);
		th_info_init(&mTheoraInfo);
		vorbis_info_init(&mVorbisInfo);
		vorbis_comment_init(&mVorbisComment);

		while (!done)
		{
			char *buffer = ogg_sync_buffer( &mOggSyncState, 4096);
			int bytesRead = mStream->read( buffer, 4096 );
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
					if (mTheoraStreams) 
						ogg_stream_pagein( &mTheoraStreamState, &mOggPage );
					if (mVorbisStreams) 
						ogg_stream_pagein( &mVorbisStreamState, &mOggPage );
					
					done=true;
					break;
				}
		
				ogg_stream_init( &OggStateTest, ogg_page_serialno( &mOggPage ) );
				ogg_stream_pagein( &OggStateTest, &mOggPage );
				ogg_stream_packetout( &OggStateTest, &tempOggPacket );

				//identify the codec
				int ret;
				if( !mTheoraStreams)
				{
					ret=ret=th_decode_headerin( &mTheoraInfo, &mTheoraComment, &mTheoraSetup, &tempOggPacket);

					if (ret > 0)
					{
						//This is the Theora Header
						memcpy( &mTheoraStreamState, &OggStateTest, sizeof(OggStateTest));
						mTheoraStreams = 1;
					}
				}
				/*
				else if( !mVorbisStreams &&
					vorbis_synthesis_headerin(&mVorbisInfo, &mVorbisComment, &tempOggPacket) >=0 )
				{
					//This is vorbis header
					memcpy( &mVorbisStreamState, &OggStateTest, sizeof(OggStateTest));
					mVorbisStreams = 1;
				}
				*/
				else
				{
					//Hmm. I guess it's not a header we support, so erase it
					ogg_stream_clear(&OggStateTest);
				}
			}
		}

		while ((mTheoraStreams && (mTheoraStreams < 3)) ||
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
						"TheoraVideoClip::parseVorbisTheoraHeaders" );

				if( !th_decode_headerin(&mTheoraInfo, &mTheoraComment, &mTheoraSetup, &tempOggPacket) )
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "invalid stream",
						"TheoraVideoClip::parseVorbisTheoraHeaders ");

				mTheoraStreams++;			
			} //end while looking for more theora headers
		
			//look 2nd vorbis header packets
			while( mVorbisStreams && 
				 ( mVorbisStreams < 3 ) && 
				 ( iSuccess=ogg_stream_packetout( &mVorbisStreamState, &tempOggPacket))) 
			{
				if(iSuccess < 0) 
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Error parsing vorbis stream headers",
						"TheoraVideoClip::parseVorbisTheoraHeaders ");

				if(vorbis_synthesis_headerin( &mVorbisInfo, &mVorbisComment,&tempOggPacket)) 
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "invalid stream",
						"TheoraVideoClip::parseVorbisTheoraHeaders ");

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
				int bytesRead = mStream->read( buffer, 4096 );
				ogg_sync_wrote( &mOggSyncState, bytesRead );

				if( bytesRead == 0 )
					OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "End of file found prematurely",
						"TheoraVideoClip::parseVorbisTheoraHeaders " );
			}
		} //end while looking for all headers

		String temp1 = StringConverter::toString( mVorbisStreams );
		String temp2 = StringConverter::toString( mTheoraStreams );
		LogManager::getSingleton().logMessage("Vorbis Headers: " + temp1 + " Theora Headers : " + temp2);
	}

	String TheoraVideoClip::getName()
	{
		return mName;
	}

	TheoraOutputMode TheoraVideoClip::getOutputMode()
	{
		return mOutputMode;
	}

	void TheoraVideoClip::setOutputMode(TheoraOutputMode mode)
	{
		// Yuv black is (0,128,128) and grey/rgb (0,0,0) so we need to make sure we
		// clear our frames to that colour so we won't get border pixels in different colour
		if (mode == TH_YUV) mFrameQueue->fillBackColour(0xFF008080);
		else                mFrameQueue->fillBackColour(0xFF000000);
		mOutputMode=mode;
		mBackColourChanged=true;
	}

	float TheoraVideoClip::getTimePosition()
	{
		return 0;
	}

	float TheoraVideoClip::getDuration()
	{
		return 1;
	}

	void TheoraVideoClip::play()
	{
		mPaused=false;
	}

	void TheoraVideoClip::pause()
	{
		mPaused=true;
	}

	void TheoraVideoClip::stop()
	{
	
	}

	void TheoraVideoClip::seek(float time)
	{
		//mTimePos=time;
		//ogg_sync_reset( &mOggSyncState );
		//ogg_stream_reset( &mTheoraStreamState );
		mStream->seek(mStream->size()/2);
		
	}

	bool TheoraVideoClip::isPlaying()
	{
		return !mPaused;
	}

	float TheoraVideoClip::getPriority()
	{
		return 0;
	}

} // end namespace Ogre

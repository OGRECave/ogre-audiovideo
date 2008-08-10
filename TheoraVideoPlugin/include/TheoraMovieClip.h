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
#ifndef _TheoraVideoMovieClip_H
#define _TheoraVideoMovieClip_H

#include "OgreExternalTextureSource.h"
#include "OgrePrerequisites.h"
#include "OgreDataStream.h"
#include "OgreString.h"

#include "TheoraExport.h"
#include "TheoraPlayerPreReqs.h"
#include "TheoraVideoDriver.h"
#include "TheoraSeekUtility.h"

#include "theora/theora.h"
#include "vorbis/codec.h"
#include "pasync.h"

#ifdef min 
#undef min 
#endif 

#ifdef max 
#undef max 
#endif 

namespace Ogre
{
	/**
		A messenger class for sending off messages relating to stream/movie events	
	*/
	class _OgreTheoraExport TheoraMovieMessage
	{
	public:
		TheoraMovieMessage() {}
		virtual ~TheoraMovieMessage() {}

		enum PLUGIN_theora_message
		{
			//Signal that movie reached end of ogg stream, still playing though
			TH_OggStreamDone = 0,
			//No more vorbis packets for decoding.. buffered audio still present
			TH_VorbisStreamDone,
			//No more theora packets found in the stream.
			TH_TheoraStreamDone,
			//The movie is no longer playing
			TH_EndOfMovie
		};

		/**
			@remarks
				This message is sent once the movie length is determined
			@param discoveredTime
				Movie duration in seconds
		*/		
		virtual void discoveredMovieTime( float discoveredTime ) {}

		/**
			@remarks
				Event method to send messages.
			@param m
				The message that occured
			@return
				varies on message... Currently unused
		*/
		virtual int messageEvent( PLUGIN_theora_message m ) = 0;
		
		/**
			@remarks
				Event raised when a frame is blitted
			@param
				Video Time is seconds
			@param
				Audio Time is seconds
			@param
				The current frame that was blitted
			@param
				Amount of frames that have been discarded to keep sync
		*/

		struct FrameInfo
		{
			float mVideoTime,
				  mAudioTime,
				  mAvgDecodeTime,
				  mAvgYUVConvertTime,
				  mAvgBlitTime;
			int   mCurrentFrame,
				  mNumFramesDropped,
				  mNumPrecachedFrames;
		};
		virtual void displayedFrame(FrameInfo info) {};
	};


//******************************************************************************//
	class TheoraMovieClip; // prototype because it is used in TheoraFrame

	class TheoraFrame
	{
	public:
		/**
			@remarks
				initializes a frame
			@param w
				width of the frame in pixels
			@param h
				height of the frame in pixels
		*/
		TheoraFrame(TheoraMovieClip* parent,int w,int h);
		~TheoraFrame();
		/**
			@remarks
				copies theora YUV buffer to mPixelBuffer and performes either
				YUV->RGB conversion or stores YUV directly into RGB (for shaders to convert later on)
			@param yuv
				theora's yuv_buffer structure
			@param timeToDisplay
				latest time this frame should be displayed
			@param convert_to_rgb
		*/
		void copyYUV(yuv_buffer yuv,double timeToDisplay,bool convert_to_rgb=true);

		double mTimeToDisplay;
		bool mInUse;
		TheoraMovieClip* mParent;

		unsigned char* mPixelBuffer;
	};
//******************************************************************************//




	/** 
		Class that holds an Ogg Theora Movie clip
	*/
	class _OgreTheoraExport TheoraMovieClip : public pt::thread
	{
	public:
		TheoraMovieClip();
		~TheoraMovieClip();

		/**
			@remarks
				Sets up texture for movie playing
			@param sMovieName
			@param sMaterialName
			@param TechniqueLevel
			@param PassLevel
			@param TextureUnitStateLevel
			@param HasSound
			@param eMode
			@param renderMode
			@return
				true on success, false otherwise
		*/
		void createMovieClip( 
			const String &sMovieName, const String &sMaterialName,
			const String &sGroupName, int TechniqueLevel, int PassLevel, 
			int TextureUnitStateLevel, bool HasSound = false, 
			eTexturePlayMode eMode = TextureEffectPause,
			bool seekingEnabled = false,
			bool autoUpdateAudio = false );

		/**
			@remarks
				Changes the play mode of this movie
			@param eMode
				Mode to change to 
		*/
		void changePlayMode( eTexturePlayMode eMode );

		eTexturePlayMode getPlayMode() { return mPlayMode; }

		/**
			@remarks
				You want audio? Then register an audio class that is derived
				from TheoraAudioDriver and register it before you play your video
			@param pAud
				Class to handle audio output
		*/
		void setAudioDriver( TheoraAudioDriver *pAud );
		
		/**
			@remarks
				Get the audio driver this movie uses
			@returns
				The audio driver or null
		*/
		TheoraAudioDriver* getAudioDriver() { return mAudioInterface; }
		
		/**
			@remarks
				Get the video driver this movie uses
			@returns
				The video driver class
		*/
		TheoraVideoDriver* getVideoDriver() { return &mVideoInterface; }

		/**
			@remarks
				Register a listener for movie events
			@param m
				The class to recieve messages
		*/
		void registerMessageHandler( TheoraMovieMessage* m ) {mMessageListener = m;}

		/**
			@remarks
				Loads the movie - after this point it is ready to play
			@param filename
				The string movie filename
			@param useAudio
				Leaves the possibility of sound playing there
		*/			
		void load( const String& filename, 
			const String& groupName, bool useAudio );
		
		/**
			@remarks
				Close & delete all resources for this movie
		*/
		void close();
	
		/**
			@remarks
				Call every frame to check for a ready frame
		*/
		void blitFrameCheck();
		
		/**
			@remarks
				Gets the movie name
			@returns
				Returns the string name of this movie
		*/
		const String & getMovieName() const { return mMovieName; }
		
		/**
			@remarks
				Gets the material name
			@returns
				Returns the string name of this movie
		*/		
		const String & getMaterialName() const { return mMaterialName; }
		
		/**
			@remarks
				Seeks to sent time (in seconds).. This call returns immediately
		*/		
		void seekToTime( float seconds );

		/**
			@remarks
				Gets the best guess (means quickest results) movie length
			@returns
				Returns time in seconds
		*/
		float getMaxSeekTime( ) { return mMovieLength; }

	    //! Main Thread Body - do not call directly!
		void execute();

		//! Called during thread clean up - do not call directly!
		void cleanup();

		/**
			@remarks
				initialises N TheoraFrame objects to hold prerendered frames
		*/
		void setNumPrecachedFrames(int num);
		int getNumPrecachedFrames() { return mFrameRepository.size(); }

	protected:
		/**
			@remarks
				Used in syncronization layer
			@returns
				Returns the time of playing (in seconds)
		*/
		float getMovieTime();


		void initVorbisTheoraLayer( );
		void parseVorbisTheoraHeaders( bool useAudio );
		void activateVorbisTheoraCodecs( bool useAudio );
		void decodeVorbis();
		void decodeTheora();
		
		//Seeking helper objects
		float mMovieLength;
		volatile bool mDoSeek;
		volatile float mSeekTime;
		TheoraSeekUtility *mSeeker;

		// time the video clip has to reach before it should display the next frame
		double mTimeOfNextFrame;
		// a list that holds our available frame buffers
		std::list<TheoraFrame*> mFrameRepository;
		// mutex that syncs the main thread and the frame extraction thread
		pt::mutex mFrameMutex;
		// queue containing rendered frames
		std::queue<TheoraFrame*> mFrames;
		bool mFramesReady;

		//Mux/Demux Structs
		ogg_sync_state   mOggSyncState;		//sync and verify incoming physical bitstream
		ogg_page         mOggPage;				//one Ogg bitstream page
		ogg_stream_state mVorbisStreamState;	//take physical pages, weld into a logical
		ogg_stream_state mTheoraStreamState;	//take physical pages, weld into a logical
		//Theora State
		theora_info      mTheoraInfo;		//struct that stores all the static theora bitstream settings
		theora_comment   mTheoraComment;
		theora_state     mTheoraState;
		//Vorbis State
		vorbis_info      mVorbisInfo;		//struct that stores all the static theora bitstream settings
		vorbis_dsp_state mVorbisDSPState;	//central working state for the packet->PCM decoder
		vorbis_block     mVorbisBlock;		//local working space for packet->PCM decode
		vorbis_comment   mVorbisComment;

		String mMovieName;
		String mMaterialName;

		Timer* mTimer;
		TheoraMovieMessage* mMessageListener;
		eTexturePlayMode mPlayMode;

		int mFrameNum;
		int mNumDroppedFrames;
		float mAvgDecodedTime, mAvgYUVConvertedTime,mSumBlited;
		float mDecodedTime, mYUVConvertTime, mBlitTime;
		int mNumFramesEvaluated;

		unsigned int mLastFrameTime;

		//! A class that handles the audio
		TheoraAudioDriver *mAudioInterface;
		//! A class that handles the video/textures
		TheoraVideoDriver mVideoInterface;

		volatile bool mThreadRunning;
		bool mEndOfFile;
		bool mEndOfAudio;
		bool mEndOfVideo;
		bool mAudioStarted;

		bool mAutoUpdate;

		//Open File Stream
		DataStreamPtr mOggFile;

		int mTheoraStreams, mVorbisStreams;	// Keeps track of Theora and Vorbis Streams
		int currentTicks;		//! ticks information to be used if the audio stream is not present

		//single frame video buffering
		bool mVideoFrameReady;
		float videobuf_time; // TODO: (kspes) hmm, why is this a member var?

		ogg_int64_t audiobuf_granulepos; //time position of last sample
	};
}
#endif//_TheoraVideoMovieClip_H

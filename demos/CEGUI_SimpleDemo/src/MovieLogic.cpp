#include "CEGUI/CEGUI.h"
#include "OgrePrerequisites.h"
#include "OgreCEGUIRenderer.h"

#include "MovieLogic.h"

#include "OgreStringConverter.h"
#include "OgreLogManager.h"
#include "OgreException.h"
#include "OgreTexture.h"
#include "OgreTextureManager.h"
#include "OgreMaterialManager.h"
#include "OgreTechnique.h"
#include "OgreExternalTextureSourceManager.h"

namespace Ogre
{
	//-------------------------------------------------------------------------//
	MovieLogic::MovieLogic( CEGUI::OgreCEGUIRenderer *renderer )
	{
		mGUIRenderer = renderer;
		
		mVideoControl = 0;
		
		mSlider = 0;
		mScroller = 0;
		maxTime = 0.0f;
		mClip = 0;
		mPaused = true;
	}

	//-------------------------------------------------------------------------//
	MovieLogic::~MovieLogic()
	{
		//NOTE: If using audio, you must delete the movie before you delete the
		//sound class, as the decoding thread could be inside or about to use
		//the audio class... and for speed/simplicity, we do not use Mutex's
		if( mClip )
		{
			mClip->changePlayMode( TextureEffectPause );
			mVideoControl->destroyAdvancedTexture( "Example/TheoraVideoPlayer/Play" );
		}
	}

	//-------------------------------------------------------------------------//
	void MovieLogic::initialise()
	{
		mVideoControl = static_cast<TheoraVideoController*>
			(ExternalTextureSourceManager::getSingleton().
				getExternalTextureSource("ogg_video"));

		if( !mVideoControl )
			OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, 
				"Error grabbing Plugin_TheoraVideoSystem. Added in plugins.cfg file?",
				"MovieLogic::initialise");
	}


	//-------------------------------------------------------------------------//
	unsigned int MovieLogic::getWidth()
	{
		return mClip->getVideoDriver()->getWidth();
	}

	//-------------------------------------------------------------------------//
	unsigned int MovieLogic::getHeight()
	{
		return mClip->getVideoDriver()->getHeight();
	}

	//-------------------------------------------------------------------------//
	void MovieLogic::stopMovie()
	{
		if( mClip )
		{
			mClip->changePlayMode( TextureEffectPause );
			mVideoControl->destroyAdvancedTexture( "Example/TheoraVideoPlayer/Play" );

		}

		maxTime = 0.0f;
		mClip = 0;
		mPaused = true;
	}

	//-------------------------------------------------------------------------//
	void MovieLogic::pauseMovie( bool bPause )
	{
		if( mClip )
		{
			if( mPaused == bPause ) 
				return;

			if( mPaused == false )
				mClip->changePlayMode( TextureEffectPause );
			else
				mClip->changePlayMode( TextureEffectPlay_ASAP );

			mPaused = bPause;
		}
	}

	//-------------------------------------------------------------------------//
	void MovieLogic::playMovie( const String& movieName )
	{
		return;
		//Tear down current movie if any
		stopMovie();
		
		//Sets an input file name - needed by plugin
		mVideoControl->setInputName( movieName );
		//Start paused so we can have audio
		mVideoControl->setPlayMode( TextureEffectPause );
		//! Used for attaching texture to Technique, State, and texture unit layer
		mVideoControl->setTextureTecPassStateLevel( 0, 0, 0 );

		//Set to true to allow for seeking - highly experimental though ;)
		mVideoControl->setSeekEnabled( false );
		//This is mainly for OpenAL - but applies to other audio libs which
		//use pooling instead of callbacks for updating...
		//Let TheoraMovieClip update the audioclip.
		mVideoControl->setAutoAudioUpdate( false );

		// Grab Our material, then add a new texture unit
		MaterialPtr material = MaterialManager::getSingleton().getByName("Example/TheoraVideoPlayer/Play");
		
		//Create the material the first time through this method
		if( material.isNull() ) 
			material = MaterialManager::getSingleton().create("Example/TheoraVideoPlayer/Play", "General");

		material->getTechnique(0)->getPass(0)->createTextureUnitState();

		mVideoControl->createDefinedTexture( "Example/TheoraVideoPlayer/Play", "General" );
		mClip = mVideoControl->getMaterialNameClip( "Example/TheoraVideoPlayer/Play" );
		if( !mClip )
			OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Clip not found", "MovieLogic::playMovie" );
		
		//Register to recieve messages
		mClip->registerMessageHandler( this );
		


		//Start playing
		mClip->changePlayMode( TextureEffectPlay_ASAP );
		mPaused = false;

		//Movie name is the texture name
		mCurrentMoviePlaying = mClip->getMovieName();

		//mSlider->setProgress( 0.0f );
		//mScroller->setScrollPosition( 0.0f );
		sliding = false;
	}

	void MovieLogic::discoveredMovieTime( float discoveredTime )
	{ 
		maxTime = discoveredTime;
		//mScroller->setPageSize( maxTime );
	}

	//-------------------------------------------------------------------------//
	int MovieLogic::messageEvent( PLUGIN_theora_message m )
	{
		switch( m )
		{
		case TH_TheoraStreamDone:
			//Signal that movie video packets reached end
			LogManager::getSingleton().logMessage("Video Packets empty"); 
			break;
		case TH_VorbisStreamDone:
			//Signal that movie audio packets reached end
			LogManager::getSingleton().logMessage("Audio Packets empty");
			break;
		case TH_OggStreamDone: 
			//Signal that file reached end
			LogManager::getSingleton().logMessage("End of file reached");
			break;
		case TH_EndOfMovie: 
			//Signal that Movie playback reached end - this is called
			//from the main context, so Clip deletion is ok from within
			//this message.. or at least, should be
			LogManager::getSingleton().logMessage("Movie Playback done");
			break;
		}
		return 0;
	}
	
	//-------------------------------------------------------------------------//
	void MovieLogic::displayedFrame(TheoraMovieMessage::FrameInfo info)
	{
		if (mTimer.getMilliseconds() < 5*60000)
		{
			CEGUI::WindowManager &Mgr = CEGUI::WindowManager::getSingleton();
			
			std::stringstream s1; s1 << "Frame Number: " << info.mCurrentFrame;
			std::stringstream s2; s2 << "Frames dropped: " << info.mNumFramesDropped;
			std::stringstream s3; s3 << "Video time: " << std::fixed << std::setprecision(1) << info.mVideoTime;
			std::stringstream s4; s4 << "Decoding time (ms): " << std::fixed << std::setprecision(2) << info.mAvgDecodeTime;
			std::stringstream s5; s5 << "YUV--RGB time (ms): " << std::fixed << std::setprecision(2) << info.mAvgYUVConvertTime;
			std::stringstream s6; s6 << "TexBlit time (ms): " << std::fixed << std::setprecision(2)  << info.mAvgBlitTime;
			float time=(info.mAvgDecodeTime+info.mAvgYUVConvertTime+info.mAvgBlitTime);
			std::stringstream s7; s7 << "Time per frame (ms): " << std::fixed << std::setprecision(2)  << time;
			std::stringstream s8; s8 << "Max FPS (ms): " << std::fixed << std::setprecision(1) << (1000.0f/time);
			std::stringstream s9; s9 << "Precached frames: " << info.mNumPrecachedFrames;

			Mgr.getWindow("cFrame")->setText(s1.str());
			Mgr.getWindow("droppedFrames")->setText(s2.str());
			Mgr.getWindow("vTime")->setText(s3.str());
			Mgr.getWindow("decodeTime")->setText(s4.str());
			Mgr.getWindow("yuvTime")->setText(s5.str());
			Mgr.getWindow("blitTime")->setText(s6.str());
			Mgr.getWindow("allTime")->setText(s7.str());
			Mgr.getWindow("fps")->setText(s8.str());
			Mgr.getWindow("precached")->setText(s9.str());
		}
	}

} //end Ogre




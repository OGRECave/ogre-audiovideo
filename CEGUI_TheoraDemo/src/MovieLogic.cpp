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
		mText = 0;

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
/*
		if( list.size() != 0 )
		{
			//Populate the CEGUI ComboBox
			CEGUI::Listbox *listBox = static_cast<CEGUI::Listbox*>(CEGUI::WindowManager::
				getSingletonPtr()->getWindow("AudioPlugins"));
			for(std::vector<std::string>::iterator i = list.begin(); i != list.end(); ++i)
			{
				CEGUI::ListboxTextItem* li = new CEGUI::ListboxTextItem((*i));
				li->setTextColours(CEGUI::ColourRect(CEGUI::colour(.5, 0, .3)));
				li->setSelectionColours(CEGUI::ColourRect(CEGUI::colour(0.2, 0.7, 0.7)));
				li->setSelectionBrushImage("WindowsLook", "TabButtonMiddleSelected" );
				listBox->addItem(li);
			}

			//Set first in list as default one
			CEGUI::WindowManager::getSingletonPtr()->getWindow("APS")->setText(list[0]);
		}
	*/
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
	void MovieLogic::populateListBox( CEGUI::Listbox *listBox )
	{
		//Grab all .ogg resources
		StringVectorPtr OGG_MovieList = ResourceGroupManager::getSingleton().
            findResourceNames( "General", "*.ogg" );
		std::vector< String >::iterator i;

		//Add them to movie selection box
		for( i = OGG_MovieList->begin(); i != OGG_MovieList->end(); ++i )
		{
			CEGUI::ListboxTextItem* li = new CEGUI::ListboxTextItem((*i));
			li->setTextColours(CEGUI::ColourRect(CEGUI::colour(.5, 0, .3)));
			li->setSelectionColours(CEGUI::ColourRect(CEGUI::colour(0.2, 0.7, 0.7)));
			li->setSelectionBrushImage("WindowsLook", "TabButtonMiddleSelected" );
			listBox->addItem(li);
		}
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
		mText = 0;
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
	void MovieLogic::seek( float seconds )
	{
		//Seconds is a percentage..so we multiply by the actual length to get the 
		//seek time
		if( mClip )	mClip->seekToTime( seconds * maxTime );
	}

	//-------------------------------------------------------------------------//
	void MovieLogic::playMovie( const String& movieName )
	{
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
		mText = mGUIRenderer->createTexture( mClip->getVideoDriver()->getTexture() );

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
	void MovieLogic::displayedFrame( float vTime, float aTime,
			unsigned int frameNumber, unsigned int framesDropped )
	{
		if( maxTime > 0 )
		{
			float barTime = vTime / maxTime;
			mSlider->setProgress( barTime );
			
			//Do not adjust scroll bar position while the user is sliding it
			if( !sliding )
				mScroller->setScrollPosition( barTime );
		}

		//Adjust stats window too
		static CEGUI::String v = "VT: ";
		static CEGUI::String a = "AT: ";
		static CEGUI::String c = "CF: ";
		static CEGUI::String d = "DF: ";
		
		CEGUI::WindowManager &Mgr = CEGUI::WindowManager::getSingleton();
		return;
		CEGUI::Window* gvTime = Mgr.getWindow("vTime");
		CEGUI::Window* gaTime = Mgr.getWindow("aTime");
		CEGUI::Window* gdropped = Mgr.getWindow("Dropped");
		CEGUI::Window* gcurrent = Mgr.getWindow("Current");

		gvTime->setText(v + StringConverter::toString(vTime));
		gaTime->setText(a + StringConverter::toString(aTime));
		gcurrent->setText(c + StringConverter::toString(frameNumber));
		gdropped->setText(d + StringConverter::toString(framesDropped));
	}

} //end Ogre


#ifndef _MovieLogic_Header_
#define _MovieLogic_Header_

#include "OgreString.h"
#include "SoundManager.h"
#include "TheoraVideoController.h"
#include "TheoraMovieClip.h"

namespace CEGUI
{
	//Predefine some classes of CEGUI
	class Listbox;
	class Texture;
	class StaticImage;
	class OgreCEGUIRenderer;
}

namespace Ogre
{
	class MovieLogic : public TheoraMovieMessage
	{
	private:
		MovieLogic();
	public:
		MovieLogic( CEGUI::OgreCEGUIRenderer *renderer );
		~MovieLogic();

		//This is seperated out from constructor so that exceptions can be thrown
		void initialise();

		void populateListBox( CEGUI::Listbox *listBox );
		CEGUI::Texture * getTexture() { return mText; }
		const String& getTextureName() { return mCurrentMoviePlaying; }
		unsigned int getWidth();
		unsigned int getHeight();

		//Hack - should have plugin update frames itself
		void update() { if(mClip) mClip->blitFrameCheck(); }
		
		//Our own little utility methods for handling movie status
		void stopMovie();
		void pauseMovie( bool bPause = true );
		void playMovie( const String& movieName );
		void seek( float seconds );

		void changeSoundSystem( const std::string& sndMgr );

		//Theora Plugin Stuff
		//Called when something happens (eg end of stream)
		int messageEvent( PLUGIN_theora_message m );
		//A frame was displayed.. Some debug stats
		void displayedFrame( float vTime, float aTime, unsigned int frameNumber, unsigned int framesDropped);
		//The movie length was discovered.. seeking is enabled
		void discoveredMovieTime( float discoveredTime );

        //Used to setup seek bar
		void tieTimeBar( CEGUI::ProgressBar *slider, CEGUI::Scrollbar *scroller ) 
		{ 
			mSlider = slider;
			mScroller = scroller;
		}
		
		//Used by the main module to control the sliding bar
		bool sliding;
	private:
		CEGUI::ProgressBar *mSlider;
		CEGUI::Scrollbar *mScroller;

		float maxTime;
		TheoraVideoController *mVideoControl;
		TheoraMovieClip* mClip;

		CEGUI::OgreCEGUIRenderer* mGUIRenderer;
		CEGUI::Texture *mText;

		//Needed audio stuff
		SoundManager* mSoundSystem;
		TheoraAudioDriver* mAudio;

		String mCurrentMoviePlaying;
		bool mPaused;
	};
}

#endif // _MovieLogic_Header_

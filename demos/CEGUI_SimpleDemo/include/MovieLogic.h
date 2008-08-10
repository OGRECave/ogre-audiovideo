#ifndef _MovieLogic_Header_
#define _MovieLogic_Header_

#include "Ogre.h"
#include "OgreString.h"

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

		const String& getTextureName() { return mCurrentMoviePlaying; }
		unsigned int getWidth();
		unsigned int getHeight();

		//Hack - should have plugin update frames itself
		void update() { if(mClip) mClip->blitFrameCheck(); }
		
		//Our own little utility methods for handling movie status
		void stopMovie();
		void pauseMovie( bool bPause = true );
		void playMovie( const String& movieName );

		void changeSoundSystem( const std::string& sndMgr );

		//Theora Plugin Stuff
		//Called when something happens (eg end of stream)
		int messageEvent( PLUGIN_theora_message m );
		//A frame was displayed.. Some debug stats
		void displayedFrame(TheoraMovieMessage::FrameInfo info);
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

		CEGUI::ProgressBar *mSlider;
		CEGUI::Scrollbar *mScroller;

		float maxTime;
		TheoraVideoController *mVideoControl;
		TheoraMovieClip* mClip;

		CEGUI::OgreCEGUIRenderer* mGUIRenderer;

		//Needed audio stuff

		String mCurrentMoviePlaying;
		Ogre::Timer mTimer;
		bool mPaused;
	};
}

#endif // _MovieLogic_Header_

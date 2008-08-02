/************************************************************************
	filename: 	CEGUIOgre_TestDriver1.cpp
	created:	16/5/2004
	author:		Paul D Turner
	
	purpose:	Source code for application startup
*************************************************************************/
/*************************************************************************
    Crazy Eddie's GUI System (http://crayzedsgui.sourceforge.net)
    Copyright (C)2004 Paul D Turner (crayzed@users.sourceforge.net)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*************************************************************************/
/*************************************************************************
	This is a test driver application for CEGUI under Ogre.
*************************************************************************/
#include "CEGUI/CEGUI.h"
#include "CEGUI/CEGUIExceptions.h"
#include "OgrePrerequisites.h"
#include "OgreStringConverter.h"

#include "MainModule.h"
#include "OgreCEGUIRenderer.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
int main(int argc, char *argv[])
#endif
{
	// Create application object
	ExampleApplication* app = new CEGUIOgre_TestDriver1();

	try
	{
		app->go();
	}
	catch( Ogre::Exception& e ) {
		#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
          MessageBox( NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
		#else
          std::cerr << "An exception has occured: " <<
            e.getFullDescription().c_str() << std::endl;
		#endif
    }
	catch( CEGUI::Exception& ex )
	{
		#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		  MessageBox( NULL, ex.getMessage().c_str(), "CEGUI Threw an EXCEPTION!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
		#else
		  std::cerr << "CEGUI: " << ex.getMessage().c_str() << std::endl;
		#endif
	}

	// cleanup for the CEGUI bits.
	((CEGUIOgre_TestDriver1*)(app))->cleanup();
	delete app;

    return 0;
}


/*************************************************************************
	createScene method - initialises CEGUI & adds some controls etc
*************************************************************************/
void CEGUIOgre_TestDriver1::createScene(void)
{
	// setup GUI system
	mGUIRenderer = new CEGUI::OgreCEGUIRenderer(mWindow, Ogre::RENDER_QUEUE_OVERLAY, false, 3000, mSceneMgr);
	new CEGUI::System(mGUIRenderer);

	// load some GUI stuff for demo.
	using namespace CEGUI;

	Logger::getSingleton().setLoggingLevel(Informative);

	// load scheme and set up defaults
	SchemeManager::getSingleton().loadScheme("WindowsLook.scheme");
	System::getSingleton().setDefaultMouseCursor("WindowsLook", "MouseArrow");

	System::getSingleton().setDefaultFont("Tahoma-12");

	// Create our utility movie class
	mMovieControl = new MovieLogic( mGUIRenderer );

	createDemoWindows();
	createCrazyPanel();
	initDemoEventWiring();

	mSceneMgr->setSkyDome(true, "Examples/CloudySky", 5, 8);
}

/*************************************************************************
	create the windows & widgets for this demo
*************************************************************************/
void	CEGUIOgre_TestDriver1::createDemoWindows(void)
{
	using namespace CEGUI;
	WindowManager& winMgr = WindowManager::getSingleton();

	//Load up our player windows	
	Window *mainWin = winMgr.loadWindowLayout( "DemoPlayer.xml" );

	System::getSingleton().setGUISheet(mainWin);
	
	mMovieControl->initialise();
	mMovieControl->populateListBox( (Listbox*)mainWin->getChild( "Controls" )->getChild("PlayList") );
	mMovieControl->tieTimeBar( (ProgressBar*)winMgr.getWindow("TimePosition"),
		(Scrollbar*)winMgr.getWindow("TimeSeek"));
}

/*************************************************************************
	Perform required event hook-ups for this demo.
*************************************************************************/
void	CEGUIOgre_TestDriver1::initDemoEventWiring(void)
{
	using namespace CEGUI;
	WindowManager *Mgr = WindowManager::getSingletonPtr();

	//Register method for handling play button
	Mgr->getWindow("Play")->
		subscribeEvent(PushButton::EventClicked, 
		Event::Subscriber(&CEGUIOgre_TestDriver1::handlePlay, this));
	Mgr->getWindow("Stop")->
		subscribeEvent(PushButton::EventClicked, 
		Event::Subscriber(&CEGUIOgre_TestDriver1::handleStop, this));
	Mgr->getWindow("Pause")->
		subscribeEvent(PushButton::EventClicked, 
		Event::Subscriber(&CEGUIOgre_TestDriver1::handlePause, this));
	Mgr->getWindow("Resume")->
		subscribeEvent(PushButton::EventClicked, 
		Event::Subscriber(&CEGUIOgre_TestDriver1::handleResume, this));

	Mgr->getWindow("Activate")->
		subscribeEvent(PushButton::EventClicked, 
		Event::Subscriber(&CEGUIOgre_TestDriver1::handleActivate, this));

	Mgr->getWindow("TimeSeek")->
		subscribeEvent(Scrollbar::EventThumbTrackEnded, 
		Event::Subscriber(&CEGUIOgre_TestDriver1::handleSeekDone, this));
	Mgr->getWindow("TimeSeek")->
		subscribeEvent(Slider::EventThumbTrackStarted, 
		Event::Subscriber(&CEGUIOgre_TestDriver1::handleSeekStart, this));
}

bool CEGUIOgre_TestDriver1::handleActivate(const CEGUI::EventArgs& e)
{
	//Stop any movie playing
	handleStop( e );
	
	using namespace CEGUI;
	Listbox* lb = (Listbox*)WindowManager::getSingleton().getWindow("AudioPlugins");
	const ListboxItem* i = lb->getFirstSelectedItem();
	if( !i )	
		return true;
	
	mMovieControl->changeSoundSystem(i->getText().c_str());
	return true;
}

bool CEGUIOgre_TestDriver1::handleSeekStart(const CEGUI::EventArgs& e)
{
	//Keep scroll bar from being repositioned while user is moving it
	mMovieControl->sliding = true;
	return true;
}
bool CEGUIOgre_TestDriver1::handleSeekDone(const CEGUI::EventArgs& e)
{
	using namespace CEGUI;
	mMovieControl->sliding = false;
	Scrollbar* s = (Scrollbar*)WindowManager::getSingleton().getWindow("TimeSeek");
	//Seeks between 0.0-1.0
	mMovieControl->seek( s->getScrollPosition() );
	
	return true;
}
bool CEGUIOgre_TestDriver1::handleSeek(const CEGUI::EventArgs& e)
{
	return true;
}

/*************************************************************************
	Handle PLay Command Button
*************************************************************************/
bool CEGUIOgre_TestDriver1::handlePlay(const CEGUI::EventArgs& e)
{
	using namespace CEGUI;
	Listbox* lbox = (Listbox*)WindowManager::getSingleton().getWindow("PlayList");
	const ListboxItem* item = lbox->getFirstSelectedItem();
	
	//Ensure something was selected
	if( !item )
		return true;
	
	Ogre::String str = item->getText().c_str();

	FrameWindow* fwnd3 = (FrameWindow*)WindowManager::getSingleton().getWindow("PlayWindow");
	assert( fwnd3 );
	StaticImage* simg = 0;

	try
	{
		CEGUI::String te = mMovieControl->getTextureName();
		
		fwnd3->removeChildWindow( "PlayWindow/Image1" );

		simg = (StaticImage*)WindowManager::getSingleton().getWindow("PlayWindow/Image1");
		WindowManager::getSingleton().destroyWindow( simg );
		Imageset *tempimg = ImagesetManager::getSingleton().getImageset( "MyImagesNumber" );
		tempimg->undefineImage( te );
		ImagesetManager::getSingleton().destroyImageset( "MyImagesNumber" );
	}
	catch(...) { //Tis ok, first time through, set does not exist yet
	}

	mMovieControl->playMovie( str );

	simg = (StaticImage*)WindowManager::getSingleton().createWindow("WindowsLook/StaticImage", "PlayWindow/Image1");
	
	fwnd3->addChildWindow(simg);
	simg->setMaximumSize(Size(2.0f, 2.0f));
	simg->setPosition(Point(0.0f, 0.05f));
	simg->setSize(Size(1.0f, 0.86f));
	simg->setFrameEnabled(false);
	simg->setBackgroundEnabled(false);

	//Now attach Texture to
	if( mMovieControl->getTexture() )
	{
		CEGUI::String temp = "MyImagesNumber";
		CEGUI::String tempName = mMovieControl->getTextureName();

		Imageset *img = ImagesetManager::getSingleton().createImageset( 
				temp, mMovieControl->getTexture() );

		unsigned int width = mMovieControl->getWidth();
		unsigned int height= mMovieControl->getHeight();

		img->defineImage( tempName, Point(0.0f,0.0f), Size( width, height ), Point(0.0f,0.0f));
		simg = (StaticImage*)WindowManager::getSingleton().getWindow("PlayWindow/Image1");
		simg->setImage( temp, tempName);
	}
	return true;
}

/*************************************************************************
	Quit then demo.
*************************************************************************/
bool CEGUIOgre_TestDriver1::handleQuit(const CEGUI::EventArgs& e)
{
	mRoot->queueEndRendering();
	return true;
}

bool CEGUIOgre_TestDriver1::handlePause(const CEGUI::EventArgs& e)
{
	mMovieControl->pauseMovie( true );
	return true;
}
bool CEGUIOgre_TestDriver1::handleResume(const CEGUI::EventArgs& e)
{
	mMovieControl->pauseMovie( false );
	return true;
}

bool CEGUIOgre_TestDriver1::handleStop(const CEGUI::EventArgs& e)
{
	using namespace CEGUI;

	FrameWindow* fwnd3 = (FrameWindow*)WindowManager::getSingleton().getWindow("PlayWindow");
	StaticImage* simg = 0;

	try
	{
		CEGUI::String te = mMovieControl->getTextureName();
			
		fwnd3->removeChildWindow( "PlayWindow/Image1" );

		simg = (StaticImage*)WindowManager::getSingleton().getWindow("PlayWindow/Image1");
		WindowManager::getSingleton().destroyWindow( simg );

		Imageset *tempimg = ImagesetManager::getSingleton().getImageset( 
			"MyImagesNumber" );
			
		tempimg->undefineImage( te );
		ImagesetManager::getSingleton().destroyImageset( "MyImagesNumber" );
	}
	catch(...) 
	{
		//Tis ok, first time through, set does not exist yet
	}

	mMovieControl->stopMovie();
	return true;
}

/*************************************************************************
	Handler for the error box 'okay' button
*************************************************************************/
bool CEGUIOgre_TestDriver1::handleErrorBox(const CEGUI::EventArgs& e)
{
	CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"ErrorBox")->hide();
	return true;
}


/*************************************************************************
	cleanup - free some stuff and close-down CEGUI system
*************************************************************************/
void CEGUIOgre_TestDriver1::cleanup(void)
{
	delete CEGUI::System::getSingletonPtr();
	delete mGUIRenderer;
	delete mMovieControl;
}


/*************************************************************************
	Create the example replacement debug overlay
*************************************************************************/
void CEGUIOgre_TestDriver1::createCrazyPanel(void)
{
	using namespace CEGUI;

	Window* sheet = System::getSingleton().getGUISheet();

	if (sheet != NULL)
	{
		//
		// load ogre gui imageset & font
		//
		Imageset* giset = ImagesetManager::getSingleton().createImageset("ogregui.xml");
//		CEGUI::Font* gfont = CEGUI::FontManager::getSingleton().createFont("Tahoma-8", "Tahoma.ttf", 8, 0);
//		gfont->setAutoScalingEnabled(true);
//		gfont->setNativeResolution(Size(800, 600));

		//
		// static for info panel
		//
		StaticText* panel = (StaticText*)WindowManager::getSingleton().createWindow("WindowsLook/StaticText", "Panel 1");
		sheet->addChildWindow(panel);
		panel->setPosition(Point(0.0f, 0.83f));
		panel->setSize(Size(0.23f, 0.17f));
		panel->setAlwaysOnTop(true);

		// replace frame and backdrop images
		panel->setFrameImages(
			&giset->getImage("BoxTopLeft"),
			&giset->getImage("BoxTopRight"),
			&giset->getImage("BoxBottomLeft"),
			&giset->getImage("BoxBottomRight"),
			&giset->getImage("BoxLeft"),
			&giset->getImage("BoxTop"),
			&giset->getImage("BoxRight"),
			&giset->getImage("BoxBottom")
			);

		panel->setBackgroundImage(&giset->getImage("BoxBack"));
		panel->setAlpha(0.9f);
		panel->setEnabled(false);
		panel->setInheritsAlpha(false);

		//
		// Components on panel
		//
		StaticText* st;
		st = (StaticText*)WindowManager::getSingleton().createWindow("WindowsLook/StaticText", "OPCurrentFPS");
		panel->addChildWindow(st);
		st->setFrameEnabled(false);
		st->setBackgroundEnabled(false);
//		st->setFont(gfont);
		st->setPosition(Point(0.075f, 0.12f));
		st->setSize(Size(0.85f, 0.15f));
		st->setTextColours(0xFF4F406F);
		st->setHorizontalFormatting(StaticText::HorzCentred);
		st->setText("Current FPS: ");

		StaticImage* si = (StaticImage*)WindowManager::getSingleton().createWindow("WindowsLook/StaticImage", "OPBar1");
		panel->addChildWindow(si);
		si->setFrameEnabled(false);
		si->setBackgroundEnabled(false);
		si->setPosition(Point(0.2f, 0.27f));
		si->setSize(Size(0.6f, 0.025f));
		si->setImage(&giset->getImage("Bar"));

		st = (StaticText*)WindowManager::getSingleton().createWindow("WindowsLook/StaticText", "OPAverageFPS");
		panel->addChildWindow(st);
		st->setFrameEnabled(false);
		st->setBackgroundEnabled(false);
//		st->setFont(gfont);
		st->setPosition(Point(0.075f, 0.32f));
		st->setSize(Size(1.0f, 0.15f));
		st->setTextColours(0xFF005710);
		st->setText("Average FPS: ");

		st = (StaticText*)WindowManager::getSingleton().createWindow("WindowsLook/StaticText", "OPWorstFPS");
		panel->addChildWindow(st);
		st->setFrameEnabled(false);
		st->setBackgroundEnabled(false);
//		st->setFont(gfont);
		st->setPosition(Point(0.075f, 0.45f));
		st->setSize(Size(1.0f, 0.15f));
		st->setTextColours(0xFF005710);
		st->setText("Worst FPS: ");

		st = (StaticText*)WindowManager::getSingleton().createWindow("WindowsLook/StaticText", "OPBestFPS");
		panel->addChildWindow(st);
		st->setFrameEnabled(false);
		st->setBackgroundEnabled(false);
//		st->setFont(gfont);
		st->setPosition(Point(0.075f, 0.58f));
		st->setSize(Size(1.0f, 0.15f));
		st->setTextColours(0xFF005710);
		st->setText("Best FPS: ");

		st = (StaticText*)WindowManager::getSingleton().createWindow("WindowsLook/StaticText", "OPTriCount");
		panel->addChildWindow(st);
		st->setFrameEnabled(false);
		st->setBackgroundEnabled(false);
	//	st->setFont(gfont);
		st->setPosition(Point(0.075f, 0.71f));
		st->setSize(Size(1.0f, 0.15f));
		st->setTextColours(0xFF005710);
		st->setText("Triangle Count: ");

		st = (StaticText*)WindowManager::getSingleton().createWindow("WindowsLook/StaticText", "OPDebugMsg");
		sheet->addChildWindow(st);
		st->setAlwaysOnTop(true);
		st->setFrameEnabled(false);
		st->setBackgroundEnabled(false);
	//	st->setFont(gfont);
		st->setPosition(Point(0.25f, 0.93f));
		st->setSize(Size(0.65f, 0.07f));
		st->setTextColours(0xA0FFFFFF);
		st->setFormatting(StaticText::WordWrapCentred, StaticText::VertCentred);
		st->setEnabled(false);
		st->setInheritsAlpha(false);
	}
}


/*************************************************************************
	createFrameListener - add out frame lister which handles input
*************************************************************************/
void CEGUIOgre_TestDriver1::createFrameListener(void)
{
	mFrameListener= new CEGUIOgre_FrameListener(mWindow, mCamera, mMovieControl);
	mRoot->addFrameListener(mFrameListener);
}


/*************************************************************************
	frameStarted - method that handles all input for this demo.
*************************************************************************/
bool CEGUIOgre_FrameListener::frameStarted(const FrameEvent& evt)
{
	if (mQuit)
		return false;
	else
	{
		//To Do - make video control do this for all clips
		mMovieControl->update();
		updateStats();
		return true;
	}
}

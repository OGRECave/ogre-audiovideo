#include "CEGUI/CEGUI.h"
#include "OgreCEGUIRenderer.h"
#include "OgreCEGUIResourceProvider.h"

#include "ExampleApplication.h"
#include "TheoraVideoManager.h"
#include "TheoraVideoClip.h"

//----------------------------------------------------------------//
CEGUI::MouseButton convertOISMouseButtonToCegui(int buttonID)
{
    switch (buttonID)
    {
	case 0: return CEGUI::LeftButton;
	case 1: return CEGUI::RightButton;
	case 2:	return CEGUI::MiddleButton;
	case 3: return CEGUI::X1Button;
	default: return CEGUI::LeftButton;
    }
}
//----------------------------------------------------------------//
class ClipListener : public TheoraVideoListener
{
	int messageEvent( PLUGIN_theora_message m ) { return 0; }

	/**
		grabs video info and displays it in cegui staticText objects
	*/
	void displayedFrame(TheoraVideoListener::FrameInfo info)
	{
		CEGUI::WindowManager &Mgr = CEGUI::WindowManager::getSingleton();
		
		std::stringstream s1; s1 << "Frame Number: " << info.mCurrentFrame;
		std::stringstream s2; s2 << "Frames dropped: " << info.mNumFramesDropped;
		std::stringstream s3; s3 << "Video time: " << std::fixed << std::setprecision(1) << info.mVideoTime;
		std::stringstream s4; s4 << "Decoding time (ms): " << std::fixed << std::setprecision(2) << info.mAvgDecodeTime;
		std::stringstream s5; s5 << "YUV decode time (ms): " << std::fixed << std::setprecision(2) << info.mAvgYUVConvertTime;
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
};


class GuiFrameListener : public OIS::KeyListener, public OIS::MouseListener, public ExampleFrameListener
{
private:
    CEGUI::Renderer* mGUIRenderer;
    bool mShutdownRequested;
	bool init;
	ClipListener mMovieListener;

public:
    // NB using buffered input, this is the only change
	GuiFrameListener(RenderWindow* win, Camera* cam, CEGUI::Renderer* renderer)
        : ExampleFrameListener(win, cam, true, true, true), 
          mGUIRenderer(renderer),
          mShutdownRequested(false),
		  init(false),
		  mMovieListener()
    {
		mMouse->setEventCallback(this);
		mKeyboard->setEventCallback(this);
    }

    /// Tell the frame listener to exit at the end of the next frame
    void requestShutdown(void)
    {
        mShutdownRequested = true;
    }

	int mLastFrameTicks;

	bool frameStarted(const FrameEvent& evt)
    {
		if (!init)
		{
			TheoraVideoManager* c = (TheoraVideoManager*) ExternalTextureSourceManager::getSingleton().getExternalTextureSource("ogg_video");
			TheoraVideoClip* clip=c->getMovieNameClip("clip.ogg");
			clip->registerMessageHandler(&mMovieListener);
			init=true;
		}

        if (mShutdownRequested)
            return false;
        else
            return ExampleFrameListener::frameStarted(evt);
    }
	//----------------------------------------------------------------//
	bool mouseMoved( const OIS::MouseEvent &arg )
	{
		CEGUI::System::getSingleton().injectMouseMove( arg.state.X.rel, arg.state.Y.rel );
		return true;
	}

	//----------------------------------------------------------------//
	bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
	{
		CEGUI::System::getSingleton().injectMouseButtonDown(convertOISMouseButtonToCegui(id));
		return true;
	}

	//----------------------------------------------------------------//
	bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
	{
		CEGUI::System::getSingleton().injectMouseButtonUp(convertOISMouseButtonToCegui(id));
		return true;
	}

	//----------------------------------------------------------------//
	bool keyPressed( const OIS::KeyEvent &arg )
	{
		if( arg.key == OIS::KC_ESCAPE )
			mShutdownRequested = true;
		CEGUI::System::getSingleton().injectKeyDown( arg.key );
		CEGUI::System::getSingleton().injectChar( arg.text );
		return true;
	}

	//----------------------------------------------------------------//
	bool keyReleased( const OIS::KeyEvent &arg )
	{
		CEGUI::System::getSingleton().injectKeyUp( arg.key );
		return true;
	}
};

class GuiApplication : public ExampleApplication
{
private:
    CEGUI::OgreCEGUIRenderer* mGUIRenderer;
    CEGUI::System* mGUISystem;
    CEGUI::Window* mEditorGuiSheet;
public:
    GuiApplication()
      : mGUIRenderer(0),
        mGUISystem(0),
        mEditorGuiSheet(0)
    {



	}

    ~GuiApplication()
    {
		return;
        if(mEditorGuiSheet)
        {
            CEGUI::WindowManager::getSingleton().destroyWindow(mEditorGuiSheet);
        }
        if(mGUISystem)
        {
            delete mGUISystem;
            mGUISystem = 0;
        }
        if(mGUIRenderer)
        {
            delete mGUIRenderer;
            mGUIRenderer = 0;
        }
    }

protected:
    // Just override the mandatory create scene method
    void createScene(void)
    {
		mCamera->getViewport()->setBackgroundColour(ColourValue(0.3,0.3,0.3));

        // setup GUI system
        mGUIRenderer = new CEGUI::OgreCEGUIRenderer(mWindow, 
            Ogre::RENDER_QUEUE_OVERLAY, false, 3000, mSceneMgr);

        mGUISystem = new CEGUI::System(mGUIRenderer);

        CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Informative);


		// create one quad, to minimize rendering time inpact on benchmarking
        ManualObject* model = mSceneMgr->createManualObject("quad");
		model->begin("SimpleVideo");

		model->position( 1,-1,0);
		model->textureCoord(1,1);

		model->position( 1,1,0);
		model->textureCoord(1,0);

		model->position(-0.5,1,0);
		model->textureCoord(0,0);

		model->position(-0.5,1,0);
		model->textureCoord(0,0);

		model->position( 1,-1,0);
		model->textureCoord(1,1);

		model->position(-0.5,-1,0);
		model->textureCoord(0,1);

		model->end();

		model->setUseIdentityProjection(true);
		model->setUseIdentityView(true);
		

        SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
        node->attachObject(model);



        // load scheme and set up defaults
        CEGUI::SchemeManager::getSingleton().loadScheme(
                (CEGUI::utf8*)"TaharezLookSkin.scheme");
        mGUISystem->setDefaultMouseCursor(
                (CEGUI::utf8*)"TaharezLook", (CEGUI::utf8*)"MouseArrow");
        mGUISystem->setDefaultFont((CEGUI::utf8*)"BlueHighway-10");

        CEGUI::Window* sheet = 
            CEGUI::WindowManager::getSingleton().loadWindowLayout(
                (CEGUI::utf8*)"SimpleDemo.layout"); 
        mGUISystem->setGUISheet(sheet);

    }

    // Create new frame listener
    void createFrameListener(void)
    {
        mFrameListener= new GuiFrameListener(mWindow, mCamera, mGUIRenderer);
        mRoot->addFrameListener(mFrameListener);
    }

    void setupEventHandlers(void)
    {

    }



    bool handleQuit(const CEGUI::EventArgs& e)
    {
        static_cast<GuiFrameListener*>(mFrameListener)->requestShutdown();
        return true;
    }

};

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
int main(int argc, char *argv[])
#endif
{

    // Create application object
    GuiApplication app;

    try {
        app.go();
    } catch( Ogre::Exception& e ) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        MessageBox( NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
        std::cerr << "An exception has occured: " <<
            e.getFullDescription().c_str() << std::endl;
#endif
    }


    return 0;
}

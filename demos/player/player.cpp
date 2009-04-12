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
#include "TheoraDemoApp.h"



namespace Ogre
{

	class DemoApp : public TheoraDemoApp
	{
		bool mShaders;
		bool mSeeking;
	public:
		DemoApp()
		{
			mSeeking=mShaders=0;
		}

		void frameStarted(const FrameEvent& evt)
		{
			CEGUI::Window* wnd=CEGUI::WindowManager::getSingleton().getWindow("cFrame");
			TheoraVideoManager* mgr = TheoraVideoManager::getSingletonPtr();
			TheoraVideoClip* clip=mgr->getVideoClipByName("konqi.ogg");
			float dur=clip->getDuration();
			String s=StringConverter::toString(dur);
			String s2=StringConverter::toString(clip->getTimePosition(),4);
			wnd->setText("duration: "+s+" seconds");
			CEGUI::WindowManager::getSingleton().getWindow("droppedFrames")->setText("time position: "+s2+" seconds");
			
			String np=StringConverter::toString(clip->getNumPrecachedFrames());
			CEGUI::WindowManager::getSingleton().getWindow("precached")->setText("Precached: "+np);
			

			float cTime=clip->getTimePosition();
			float duration=clip->getDuration();
			int pos=1024*(cTime/duration);

			if (!mSeeking)
			{
				CEGUI::Window* wnd2=CEGUI::WindowManager::getSingleton().getWindow("seeker");
				wnd2->setProperty("ScrollPosition",StringConverter::toString(pos));
			}

		}

		bool OnPlayPause(const CEGUI::EventArgs& e)
		{
			TheoraVideoManager* mgr = (TheoraVideoManager*) ExternalTextureSourceManager::getSingleton().getExternalTextureSource("ogg_video");
			TheoraVideoClip* clip=mgr->getVideoClipByName("konqi.ogg");

			if (!clip->isPaused())
			{
				clip->pause();
			}
			else
			{
				clip->play();
			}
			return true;
		}

		bool OnSeekStart(const CEGUI::EventArgs& e)
		{
			mSeeking=true;
			return true;
		}

		bool OnSeekEnd(const CEGUI::EventArgs& e)
		{
			mSeeking=false;

			CEGUI::Window* wnd=CEGUI::WindowManager::getSingleton().getWindow("seeker");
			TheoraVideoManager* mgr = (TheoraVideoManager*) ExternalTextureSourceManager::getSingleton().getExternalTextureSource("ogg_video");
			TheoraVideoClip* clip=mgr->getVideoClipByName("konqi.ogg");
			float dur=clip->getDuration();

			CEGUI::String prop=wnd->getProperty("ScrollPosition");
			int step=StringConverter::parseInt(prop.c_str());

			float seek_time=((float) step/1024)*dur;


			clip->seek(seek_time);


			return true;
		}

		bool OnRGB(const CEGUI::EventArgs& e)
		{
			TheoraVideoManager* mgr = (TheoraVideoManager*) ExternalTextureSourceManager::getSingleton().getExternalTextureSource("ogg_video");
			TheoraVideoClip* clip=mgr->getVideoClipByName("konqi.ogg");
			clip->setOutputMode(TH_RGB);
			return true;
		}

		bool OnYUV(const CEGUI::EventArgs& e)
		{
			TheoraVideoManager* mgr = (TheoraVideoManager*) ExternalTextureSourceManager::getSingleton().getExternalTextureSource("ogg_video");
			TheoraVideoClip* clip=mgr->getVideoClipByName("konqi.ogg");
			clip->setOutputMode(TH_YUV);
			return true;
		}

		bool OnGrey(const CEGUI::EventArgs& e)
		{
			TheoraVideoManager* mgr = (TheoraVideoManager*) ExternalTextureSourceManager::getSingleton().getExternalTextureSource("ogg_video");
			TheoraVideoClip* clip=mgr->getVideoClipByName("konqi.ogg");
			clip->setOutputMode(TH_GREY);
			return true;
		}


		bool OnShaders(const CEGUI::EventArgs& e)
		{
			
			mShaders=!mShaders;
			CEGUI::Window* wnd=CEGUI::WindowManager::getSingleton().getWindow("shaders_button");
			MaterialPtr mat=MaterialManager::getSingleton().getByName("video_material");
			Pass* pass=mat->getTechnique(0)->getPass(0);
			if (mShaders)
			{
				wnd->setText("Shader yuv2rgb = on");
				pass->setFragmentProgram("TheoraVideoPlugin/yuv2rgb");
			}
			else
			{
				wnd->setText("Shader yuv2rgb = off");
				pass->setFragmentProgram("");
			}
			return true;
		}


		void init()
		{
			CEGUI::Window* sheet = CEGUI::WindowManager::getSingleton().loadWindowLayout((CEGUI::utf8*)"SimpleDemo.layout"); 
			CEGUI::System::getSingleton().setGUISheet(sheet);

			EVENT("rgb_button",DemoApp::OnRGB); EVENT("yuv_button",DemoApp::OnYUV); EVENT("grey_button",DemoApp::OnGrey);
			EVENT("shaders_button",DemoApp::OnShaders); EVENT("Play/Pause",DemoApp::OnPlayPause);
			EVENT_EX("seeker",DemoApp::OnSeekStart,CEGUI::Scrollbar::EventThumbTrackStarted);
			EVENT_EX("seeker",DemoApp::OnSeekEnd,CEGUI::Scrollbar::EventThumbTrackEnded);

			createQuad("video_quad","video_material",-0.5,1,1,-0.94);

			TheoraVideoManager* mgr=TheoraVideoManager::getSingletonPtr();

			mgr->setInputName("konqi.ogg");
			mgr->createDefinedTexture("video_material");
		}
	};

	TheoraDemoApp* start() { return new DemoApp(); }
}

/*

class GuiFrameListener : public OIS::KeyListener, public OIS::MouseListener, public ExampleFrameListener
{
private:
    CEGUI::Renderer* mGUIRenderer;
    bool mShutdownRequested;
	bool init;
	//ClipListener mMovieListener;

public:
    // NB using buffered input, this is the only change
	GuiFrameListener(RenderWindow* win, Camera* cam, CEGUI::Renderer* renderer)
        : ExampleFrameListener(win, cam, true, true, true), 
          mGUIRenderer(renderer),
          mShutdownRequested(false),
		  init(false)
		 // mMovieListener()
    {
		mMouse->setEventCallback(this);
		mKeyboard->setEventCallback(this);
    }

    void requestShutdown(void)
    {
        mShutdownRequested = true;
    }

	bool frameStarted(const FrameEvent& evt)
    {
		if (!init)
		{
			//TheoraVideoManager* c = (TheoraVideoManager*) ExternalTextureSourceManager::getSingleton().getExternalTextureSource("ogg_video");
			//TheoraVideoClip* clip=c->getMovieNameClip("konqi.ogg");
			//clip->registerMessageHandler(&mMovieListener);
			//clip->changePlayMode(Ogre::TextureEffectPlay_ASAP);
			init=true;
		}

		CEGUI::Window* wnd=CEGUI::WindowManager::getSingleton().getWindow("cFrame");
		TheoraVideoManager* mgr = (TheoraVideoManager*) ExternalTextureSourceManager::getSingleton().getExternalTextureSource("ogg_video");
		TheoraVideoClip* clip=mgr->getVideoClipByName("konqi.ogg");
		float dur=clip->getDuration();
		String s=StringConverter::toString(dur);
		String s2=StringConverter::toString(clip->getTimePosition(),4);
		wnd->setText("duration: "+s+" seconds");
		CEGUI::WindowManager::getSingleton().getWindow("droppedFrames")->setText("time position: "+s2+" seconds");
		
		String np=StringConverter::toString(clip->getNumPrecachedFrames());
		CEGUI::WindowManager::getSingleton().getWindow("precached")->setText("Precached: "+np);
		

		float cTime=clip->getTimePosition();
		float duration=clip->getDuration();
		int pos=1024*(cTime/duration);

		if (!mSeeking)
		{
			CEGUI::Window* wnd2=CEGUI::WindowManager::getSingleton().getWindow("seeker");
			wnd2->setProperty("ScrollPosition",StringConverter::toString(pos));
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
    CEGUI::Window* mGuiSheet;
	bool mShaders;
public:

    GuiApplication()
      : mGUIRenderer(0),
        mGUISystem(0),
        mGuiSheet(0),
		mShaders(false)
    {



	}

    ~GuiApplication()
    {
		return;
        if(mGuiSheet)
        {
            CEGUI::WindowManager::getSingleton().destroyWindow(mGuiSheet);
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

		model->position( 1,-0.94,0);
		model->textureCoord(1,1);

		model->position( 1,1,0);
		model->textureCoord(1,0);

		model->position(-0.5,1,0);
		model->textureCoord(0,0);

		model->position(-0.5,1,0);
		model->textureCoord(0,0);

		model->position( 1,-0.94,0);
		model->textureCoord(1,1);

		model->position(-0.5,-0.94,0);
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

		setupEventHandlers();

    }

    // Create new frame listener
    void createFrameListener(void)
    {
        mFrameListener= new GuiFrameListener(mWindow, mCamera, mGUIRenderer);
        mRoot->addFrameListener(mFrameListener);
    }

    bool OnPlayPause(const CEGUI::EventArgs& e)
    {
		TheoraVideoManager* mgr = (TheoraVideoManager*) ExternalTextureSourceManager::getSingleton().getExternalTextureSource("ogg_video");
		TheoraVideoClip* clip=mgr->getVideoClipByName("konqi.ogg");

		if (!clip->isPaused())
		{
			clip->pause();
		}
		else
		{
			clip->play();
		}
        return true;
    }

    bool OnSeekStart(const CEGUI::EventArgs& e)
    {
		mSeeking=true;
		return true;
	}

    bool OnSeekEnd(const CEGUI::EventArgs& e)
    {
		mSeeking=false;

		CEGUI::Window* wnd=CEGUI::WindowManager::getSingleton().getWindow("seeker");
		TheoraVideoManager* mgr = (TheoraVideoManager*) ExternalTextureSourceManager::getSingleton().getExternalTextureSource("ogg_video");
		TheoraVideoClip* clip=mgr->getVideoClipByName("konqi.ogg");
		float dur=clip->getDuration();

		CEGUI::String prop=wnd->getProperty("ScrollPosition");
		int step=StringConverter::parseInt(prop.c_str());

		float seek_time=((float) step/1024)*dur;


		clip->seek(seek_time);


		return true;
	}

    bool OnRGB(const CEGUI::EventArgs& e)
    {
		TheoraVideoManager* mgr = (TheoraVideoManager*) ExternalTextureSourceManager::getSingleton().getExternalTextureSource("ogg_video");
		TheoraVideoClip* clip=mgr->getVideoClipByName("konqi.ogg");
		clip->setOutputMode(TH_RGB);
        return true;
    }

    bool OnYUV(const CEGUI::EventArgs& e)
    {
		TheoraVideoManager* mgr = (TheoraVideoManager*) ExternalTextureSourceManager::getSingleton().getExternalTextureSource("ogg_video");
		TheoraVideoClip* clip=mgr->getVideoClipByName("konqi.ogg");
		clip->setOutputMode(TH_YUV);
        return true;
    }

    bool OnGrey(const CEGUI::EventArgs& e)
    {
		TheoraVideoManager* mgr = (TheoraVideoManager*) ExternalTextureSourceManager::getSingleton().getExternalTextureSource("ogg_video");
		TheoraVideoClip* clip=mgr->getVideoClipByName("konqi.ogg");
		clip->setOutputMode(TH_GREY);
        return true;
    }


    bool OnShaders(const CEGUI::EventArgs& e)
    {
		
		mShaders=!mShaders;
		CEGUI::Window* wnd=CEGUI::WindowManager::getSingleton().getWindow("shaders_button");
		MaterialPtr mat=MaterialManager::getSingleton().getByName("SimpleVideo");
		Pass* pass=mat->getTechnique(0)->getPass(0);
		if (mShaders)
		{
			wnd->setText("Shader yuv2rgb = on");
			pass->setFragmentProgram("TheoraVideoPlugin/yuv2rgb");
		}
		else
		{
			wnd->setText("Shader yuv2rgb = off");
			pass->setFragmentProgram("");
		}
        return true;
    }

    void setupEventHandlers(void)
    {
		CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();
        wmgr.getWindow((CEGUI::utf8*)"rgb_button")
			->subscribeEvent(
				CEGUI::PushButton::EventClicked, 
				CEGUI::Event::Subscriber(&GuiApplication::OnRGB,this));
        wmgr.getWindow((CEGUI::utf8*)"yuv_button")
			->subscribeEvent(
				CEGUI::PushButton::EventClicked, 
				CEGUI::Event::Subscriber(&GuiApplication::OnYUV,this));
        wmgr.getWindow((CEGUI::utf8*)"grey_button")
			->subscribeEvent(
				CEGUI::PushButton::EventClicked, 
				CEGUI::Event::Subscriber(&GuiApplication::OnGrey,this));
        wmgr.getWindow((CEGUI::utf8*)"shaders_button")
			->subscribeEvent(
				CEGUI::PushButton::EventClicked, 
				CEGUI::Event::Subscriber(&GuiApplication::OnShaders,this));

        wmgr.getWindow((CEGUI::utf8*)"Play/Pause")
			->subscribeEvent(
				CEGUI::PushButton::EventClicked, 
				CEGUI::Event::Subscriber(&GuiApplication::OnPlayPause,this));

        wmgr.getWindow((CEGUI::utf8*)"seeker")
			->subscribeEvent(
			CEGUI::Scrollbar::EventThumbTrackStarted, 
				CEGUI::Event::Subscriber(&GuiApplication::OnSeekStart,this));

        wmgr.getWindow((CEGUI::utf8*)"seeker")
			->subscribeEvent(
				CEGUI::Scrollbar::EventThumbTrackEnded, 
				CEGUI::Event::Subscriber(&GuiApplication::OnSeekEnd,this));

    }



    bool handleQuit(const CEGUI::EventArgs& e)
    {
        static_cast<GuiFrameListener*>(mFrameListener)->requestShutdown();
        return true;
    }

};

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN


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
*/
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
#ifndef __TheoraDemoApp_H__
#define __TheoraDemoApp_H__

#include "CEGUI/CEGUI.h"
#include "OgreCEGUIRenderer.h"
#include "OgreCEGUIResourceProvider.h"
#include "OgreExternalTextureSourceManager.h"
#include "ExampleApplication.h"
#include "TheoraVideoManager.h"
#include "TheoraVideoClip.h"


namespace Ogre
{
	SceneManager* SceneMgr;

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

	void createQuad(String name,String material_name,float left,float top,float right,float bottom)
	{
		ManualObject* model = SceneMgr->createManualObject(name);
		model->begin(material_name);

		model->position(right,bottom,0); model->textureCoord(1,1);
		model->position(right,top   ,0); model->textureCoord(1,0);
		model->position(left ,top   ,0); model->textureCoord(0,0);
		model->position(left ,top   ,0); model->textureCoord(0,0);
		model->position(right,bottom,0); model->textureCoord(1,1);
		model->position(left, bottom,0); model->textureCoord(0,1);

		model->end();
		// make the model 2D
		model->setUseIdentityProjection(true);
		model->setUseIdentityView(true);
		// and atach it to the root node
		SceneNode* node = SceneMgr->getRootSceneNode()->createChildSceneNode();
		node->attachObject(model);
	}

	TheoraVideoClip* getClip(String name)
	{
		TheoraVideoManager* mgr = TheoraVideoManager::getSingletonPtr();
		return mgr->getVideoClipByName(name);
	}

    #define EVENT(wnd_name,function) \
		CEGUI::WindowManager::getSingleton().getWindow(wnd_name)->subscribeEvent( \
				CEGUI::PushButton::EventClicked, \
				CEGUI::Event::Subscriber(&function,this));

    #define EVENT_EX(wnd_name,function,type) \
		CEGUI::WindowManager::getSingleton().getWindow(wnd_name)->subscribeEvent( \
				type, CEGUI::Event::Subscriber(&function,this));


	class TheoraDemoApp
	{
	public:
		virtual void frameStarted(const FrameEvent& evt) = 0;
		virtual void init() = 0;
	};

	TheoraDemoApp* demo_app;
	TheoraDemoApp* start();


	class DemoFrameListener : public OIS::KeyListener, public OIS::MouseListener, public ExampleFrameListener
	{
	private:
		CEGUI::Renderer* mGUIRenderer;
		bool mShutdownRequested;

	public:
		DemoFrameListener(RenderWindow* win, Camera* cam, CEGUI::Renderer* renderer)
			: ExampleFrameListener(win, cam, true, true, true), 
			  mGUIRenderer(renderer),
			  mShutdownRequested(false)
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
			demo_app->frameStarted(evt);

			if (mShutdownRequested)
				return false;
			else
				return ExampleFrameListener::frameStarted(evt);
		}

		bool mouseMoved( const OIS::MouseEvent &arg )
		{
			CEGUI::System::getSingleton().injectMouseMove( arg.state.X.rel, arg.state.Y.rel );
			return true;
		}

		bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
		{
			CEGUI::System::getSingleton().injectMouseButtonDown(convertOISMouseButtonToCegui(id));
			return true;
		}

		bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
		{
			CEGUI::System::getSingleton().injectMouseButtonUp(convertOISMouseButtonToCegui(id));
			return true;
		}

		bool keyPressed( const OIS::KeyEvent &arg )
		{
			if( arg.key == OIS::KC_ESCAPE ) mShutdownRequested = true;
			CEGUI::System::getSingleton().injectKeyDown( arg.key );
			CEGUI::System::getSingleton().injectChar( arg.text );
			return true;
		}

		bool keyReleased( const OIS::KeyEvent &arg )
		{
			CEGUI::System::getSingleton().injectKeyUp( arg.key );
			return true;
		}
	};



	class TheoraDemoApplication : public ExampleApplication
	{
	private:
		CEGUI::OgreCEGUIRenderer* mGUIRenderer;
		CEGUI::System* mGUISystem;
		CEGUI::Window* mGuiSheet;
		bool mShaders;
	public:

		TheoraDemoApplication()
		  : mGUIRenderer(0),
			mGUISystem(0),
			mGuiSheet(0),
			mShaders(false)
		{

		}

		~TheoraDemoApplication()
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

		void createScene()
		{
			SceneMgr=mSceneMgr; // make a global shortcut
			mCamera->getViewport()->setBackgroundColour(ColourValue(0.3,0.3,0.3));
			// setup GUI system
			mGUIRenderer = new CEGUI::OgreCEGUIRenderer(mWindow,Ogre::RENDER_QUEUE_OVERLAY, false, 3000, mSceneMgr);
			mGUISystem = new CEGUI::System(mGUIRenderer);
			CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Informative);
			// load scheme and set up defaults
			CEGUI::SchemeManager::getSingleton().loadScheme((CEGUI::utf8*)"TaharezLookSkin.scheme");
			mGUISystem->setDefaultMouseCursor((CEGUI::utf8*)"TaharezLook", (CEGUI::utf8*)"MouseArrow");
			mGUISystem->setDefaultFont((CEGUI::utf8*)"BlueHighway-10");

			demo_app=start();
			demo_app->init();

		}

		// Create new frame listener
		void createFrameListener()
		{
			mFrameListener= new DemoFrameListener(mWindow, mCamera, mGUIRenderer);
			mRoot->addFrameListener(mFrameListener);
		}

		bool handleQuit(const CEGUI::EventArgs& e)
		{
			static_cast<DemoFrameListener*>(mFrameListener)->requestShutdown();
			return true;
		}

	};
}


#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
int main(int argc, char *argv[])
#endif
{

	// Create application object
	Ogre::TheoraDemoApplication app;

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
#endif

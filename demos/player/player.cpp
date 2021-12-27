/************************************************************************************
This source file is part of the Ogre3D Theora Video Plugin
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include "OgreApplicationContext.h"
#include "OgreExternalTextureSourceManager.h"
#include "Ogre.h"

#include "OgreVideoManager.h"
#include "TheoraVideoManager.h"
#include "TheoraVideoClip.h"

#include "OgreOggSoundRoot.h"
#include "OgreOggSoundInterface.h"


#define VIDEO_FILE "konqi.ogg" // must match with video_material

namespace Ogre
{

	static TheoraVideoClip* getClip(String name)
	{
		TheoraVideoManager* mgr = TheoraVideoManager::getSingletonPtr();
		return mgr->getVideoClipByName(name);
	}

    static OgreOggSound::OgreOggISound* getSound(const String& name)
    {
        auto& mgr = OgreOggSound::OgreOggSoundManager::getSingleton();
        return mgr.getSound(name);
    }

	class DemoApp : public OgreBites::ApplicationContext, public OgreBites::InputListener
	{
        OgreVideoPlugin mVideoPlugin;
		OgreOggSound::Root mAudioPlugin;

		bool mShaders;
		bool mSeeking;
		bool mPaused;
		int mSeekStep;
	public:
		DemoApp() : OgreBites::ApplicationContext("TheoraDemo")
		{
			mSeeking=mPaused=mShaders=0;
		}

		bool keyPressed(const OgreBites::KeyboardEvent& e) {
		    switch(e.keysym.sym)
		    {
		    case '\033':
		        Root::getSingleton().queueEndRendering();
		        break;
		    case ' ':
		        OnPlayPause();
		        break;
		    case 'g':
		        OnGrey();
		        break;
		    case 's':
		        OnShaders();
		        mShaders ? OnYUV() : OnRGB();
		        break;
		    }

		    return true;
		}

		bool OnPlayPause()
		{
			TheoraVideoClip* clip=getClip(VIDEO_FILE);

			if (!clip->isPaused())
			{
				mPaused=1;
				clip->pause();
                getSound(VIDEO_FILE)->pause();
			}
			else
			{
				mPaused=0;
				clip->play();
                getSound(VIDEO_FILE)->play();
			}
			return true;
		}

		bool OnSeekStart()
		{
			if (!mPaused) getClip(VIDEO_FILE)->pause();
			mSeeking=true;
			return true;
		}

		bool OnSeekEnd()
		{
			if (!mPaused) getClip(VIDEO_FILE)->play();
			mSeeking=false;
			return true;
		}

		bool OnRGB()
		{
			getClip(VIDEO_FILE)->setOutputMode(TH_RGBA);
			return true;
		}

		bool OnYUV()
		{
			getClip(VIDEO_FILE)->setOutputMode(TH_YUVA);
			return true;
		}

		bool OnGrey()
		{
			getClip(VIDEO_FILE)->setOutputMode(TH_GREY3A);
			return true;
		}


		bool OnShaders()
		{
			
			mShaders=!mShaders;
			MaterialPtr mat=MaterialManager::getSingleton().getByName("video_material");
			Pass* pass=mat->getTechnique(0)->getPass(0);
			if (mShaders)
			{
				pass->setFragmentProgram("TheoraVideoPlugin/yuv2rgb");
			}
			else
			{
				pass->setFragmentProgram("");
			}
			return true;
		}


		void createRoot()
		{
		    OgreBites::ApplicationContext::createRoot();
			mAudioPlugin.initialise();
			mRoot->installPlugin(&mVideoPlugin);

			auto mgr = OgreVideoManager::getSingletonPtr();
			mgr->setAudioInterfaceFactory(OgreOggSoundInterfaceFactory::getSingletonPtr());
		}

		void shutdown()
		{
			mAudioPlugin.shutdown();
			OgreBites::ApplicationContext::shutdown();
		}

		void setup()
		{
			// OggSound peculiarity: need to register scene manager before calling parent setup()
			// this will trigger loading .material scripts and
			// we create sounds alongside the video defined in those, which currently need the scnMgr
			SceneManager* scnMgr = getRoot()->createSceneManager();
			OgreOggSound::OgreOggSoundManager::getSingleton().init();
			OgreOggSound::OgreOggSoundManager::getSingleton().setSceneManager(scnMgr);
			
		    OgreBites::ApplicationContext::setup();

            addInputListener(this);


		    RTShader::ShaderGenerator* shadergen = RTShader::ShaderGenerator::getSingletonPtr();
		    shadergen->addSceneManager(scnMgr);

			Camera* cam = scnMgr->createCamera("myCam");
			getRenderWindow()->addViewport(cam);
			auto camnode = scnMgr->getRootSceneNode()->createChildSceneNode(Vector3(0, 0, 150));
			camnode->attachObject(cam);

			auto ent = scnMgr->createEntity(SceneManager::PT_CUBE);
			ent->setMaterial(MaterialManager::getSingleton().getByName("video_material"));

            SceneNode* node = scnMgr->getRootSceneNode()->createChildSceneNode();
            node->attachObject(ent);

			auto clip = getClip(VIDEO_FILE);
			if (clip->getAudioInterface())
			{
				auto sound = static_cast<OgreOggSoundInterface*>(clip->getAudioInterface())->ogreOggSoundObj;
				node->attachObject(sound);

				sound->disable3D(true);
			}
		}
	};
}


#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
int main(int argc, char *argv[])
#endif
{

	// Create application object
	Ogre::DemoApp app;

	try {
		app.initApp();
		app.getRoot()->startRendering();
		app.closeApp();
	} catch( Ogre::Exception& e ) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		MessageBox( NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
		Ogre::LogManager::getSingleton().logError(e.getFullDescription());
#endif
	}


	return 0;
}

/************************************************************************************
This source file is part of the Ogre3D Theora Video Plugin
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include "TheoraDemoApp.h"

#define VIDEO_FILE "konqi.ogg"

namespace Ogre
{

	class DemoApp : public TheoraDemoApp
	{
		bool mShaders;
		bool mSeeking;
		bool mPaused;
		int mSeekStep;

		OgreVideoManager mVidMgr;
	public:
		DemoApp()
		{
			mSeeking=mPaused=mShaders=0;
			mVidMgr.initialise();
			Root::getSingleton().addFrameListener(&mVidMgr);
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
			}
			else
			{
				mPaused=0;
				clip->play();
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


		void init()
		{
			createQuad("video_quad","video_material",-0.5,1,1,-0.94);

			OgreVideoManager* mgr=(OgreVideoManager*) OgreVideoManager::getSingletonPtr();

			mgr->setInputName(VIDEO_FILE);
			mgr->createDefinedTexture("video_material");
			getClip(VIDEO_FILE)->setAutoRestart(1);
		}
	};

	TheoraDemoApp* start() { return new DemoApp(); }
}

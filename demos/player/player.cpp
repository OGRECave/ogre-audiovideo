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
	public:
		DemoApp()
		{
			mSeeking=mPaused=mShaders=0;
		}

		void frameStarted(const FrameEvent& evt)
		{
			TheoraVideoClip* clip=getClip(VIDEO_FILE);
			CEGUI::WindowManager* wm=CEGUI::WindowManager::getSingletonPtr();
			if (!mSeeking)
			{
				float ctime=clip->getTimePosition(),duration=clip->getDuration();
				CEGUI::Window* wnd2=wm->getWindow("seeker");
				wnd2->setProperty("ScrollPosition",StringConverter::toString(1024*(ctime/duration)));
			}
			else
			{
				CEGUI::Window* wnd=CEGUI::WindowManager::getSingleton().getWindow("seeker");
				float dur=clip->getDuration();

				CEGUI::String prop=wnd->getProperty("ScrollPosition");
				int step=StringConverter::parseInt(prop.c_str());
				if (abs(step-mSeekStep) > 10)
				{
					mSeekStep=step;
					float seek_time=((float) step/1024)*dur;

					clip->seek(seek_time);
				}
			}

			String s=StringConverter::toString(clip->getDuration());
			String s2=StringConverter::toString(clip->getNumDroppedFrames());
			wm->getWindow("cFrame")->setText("duration: "+s+" seconds");
			wm->getWindow("droppedFrames")->setText("dropped frames: "+s2);

			String np=StringConverter::toString(clip->getNumReadyFrames());
			wm->getWindow("precached")->setText("Precached: "+np);


			const RenderTarget::FrameStats& stats = Ogre::Root::getSingleton().getAutoCreatedWindow()->getStatistics();
			wm->getWindow("fps")->setText("FPS: "+StringConverter::toString(stats.lastFPS));

		}




		bool OnPlayPause(const CEGUI::EventArgs& e)
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

		bool OnSeekStart(const CEGUI::EventArgs& e)
		{
			if (!mPaused) getClip(VIDEO_FILE)->pause();
			mSeeking=true;
			return true;
		}

		bool OnSeekEnd(const CEGUI::EventArgs& e)
		{
			if (!mPaused) getClip(VIDEO_FILE)->play();
			mSeeking=false;
			return true;
		}

		bool OnRGB(const CEGUI::EventArgs& e)
		{
			getClip(VIDEO_FILE)->setOutputMode(TH_BGRA);
			return true;
		}

		bool OnYUV(const CEGUI::EventArgs& e)
		{
			getClip(VIDEO_FILE)->setOutputMode(TH_YUVA);
			return true;
		}

		bool OnGrey(const CEGUI::EventArgs& e)
		{
			getClip(VIDEO_FILE)->setOutputMode(TH_GREY3A);
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

			OgreVideoManager* mgr=(OgreVideoManager*) OgreVideoManager::getSingletonPtr();

			mgr->setInputName(VIDEO_FILE);
			mgr->createDefinedTexture("video_material");
			getClip(VIDEO_FILE)->setAutoRestart(1);
		}
	};

	TheoraDemoApp* start() { return new DemoApp(); }
}

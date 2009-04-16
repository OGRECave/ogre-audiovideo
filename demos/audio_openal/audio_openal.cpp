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
#include "OpenAL_AudioInterface.h"

#define VIDEO_FILE "konqi.ogg"

namespace Ogre
{

	class DemoApp : public TheoraDemoApp
	{
		bool mShaders;
		bool mSeeking;
		bool mPaused;
		int mSeekStep;

		OpenAL_AudioInterfaceFactory* mAudioFactory;
	public:
		DemoApp()
		{
			mSeeking=mPaused=mShaders=0;
		}

		void frameStarted(const FrameEvent& evt)
		{
			CEGUI::Window* wnd=CEGUI::WindowManager::getSingleton().getWindow("cFrame");
			TheoraVideoManager* mgr = TheoraVideoManager::getSingletonPtr();
			TheoraVideoClip* clip=mgr->getVideoClipByName(VIDEO_FILE);
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
			else
			{
				CEGUI::Window* wnd=CEGUI::WindowManager::getSingleton().getWindow("seeker");
				TheoraVideoClip* clip=getClip(VIDEO_FILE);
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
			getClip(VIDEO_FILE)->setOutputMode(TH_RGB);
			return true;
		}

		bool OnYUV(const CEGUI::EventArgs& e)
		{
			getClip(VIDEO_FILE)->setOutputMode(TH_YUV);
			return true;
		}

		bool OnGrey(const CEGUI::EventArgs& e)
		{
			getClip(VIDEO_FILE)->setOutputMode(TH_GREY);
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

			mAudioFactory=new OpenAL_AudioInterfaceFactory();
			mgr->setAudioInterfaceFactory(mAudioFactory);

			mgr->setInputName(VIDEO_FILE);
			mgr->createDefinedTexture("video_material");
		}
	};

	TheoraDemoApp* start() { return new DemoApp(); }
}

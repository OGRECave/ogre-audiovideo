/************************************************************************
	filename: 	CEGUIOgre_TestDriver1.h
	created:	16/5/2004
	author:		Paul D Turner
	
	purpose:	Header for CEGUI for Ogre test driver application
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
#ifndef _CEGUIOgre_TestDriver1_h_
#define _CEGUIOgre_TestDriver1_h_

#include "CEGUI/CEGUI.h"
#include "ExampleApplication.h"
#include "OIS/OIS.h"
#include "MovieLogic.h"

using namespace Ogre;

class CEGUIOgre_FrameListener : public ExampleFrameListener, MouseMotionListener, MouseListener
{
public:
	CEGUIOgre_FrameListener(RenderWindow* win, Camera* cam, MovieLogic *movie) : ExampleFrameListener(win, cam, true, true)
	{
		mMovieControl = movie;

		mEventProcessor->addMouseMotionListener(this);
		mEventProcessor->addMouseListener(this);

		mQuit = false;
		mSkipCount = 0;
		mUpdateFreq = 0;
	}


	virtual void 	mouseMoved (MouseEvent *e)
	{
		CEGUI::Renderer* rend = CEGUI::System::getSingleton().getRenderer();
		CEGUI::System::getSingleton().injectMouseMove(e->getRelX() * rend->getWidth(), e->getRelY() * rend->getHeight());
		e->consume();
	}


	virtual void 	mouseDragged (MouseEvent *e)
	{
		mouseMoved(e);
	}


	virtual void 	keyPressed (KeyEvent *e)
	{
		// give 'quitting' priority
		if (e->getKey() == KC_ESCAPE)
		{
			mQuit = true;
			e->consume();
			return;
		}
		
		if (e->getKey() == KC_P)
			mWindow->writeContentsToFile("screen.png");

		// do event injection
		CEGUI::System& cegui = CEGUI::System::getSingleton();

		// key down
		cegui.injectKeyDown(e->getKey());

		// now character
		cegui.injectChar(e->getKeyChar());

		e->consume();
	}


	virtual void 	keyReleased (KeyEvent *e)
	{
		CEGUI::System::getSingleton().injectKeyUp(e->getKey());
	}



	virtual void 	mousePressed (MouseEvent *e)
	{
		CEGUI::System::getSingleton().injectMouseButtonDown(convertOgreButtonToCegui(e->getButtonID()));
		e->consume();
	}


	virtual void 	mouseReleased (MouseEvent *e)
	{
		CEGUI::System::getSingleton().injectMouseButtonUp(convertOgreButtonToCegui(e->getButtonID()));
		e->consume();
	}

	// do-nothing events
	virtual void 	keyClicked (KeyEvent *e) {}
	virtual void 	mouseClicked (MouseEvent *e) {}
	virtual void 	mouseEntered (MouseEvent *e) {}
	virtual void 	mouseExited (MouseEvent *e) {}


	bool frameStarted(const FrameEvent& evt);

protected:
	CEGUI::MouseButton convertOgreButtonToCegui(int ogre_button_id)
	{
		switch (ogre_button_id)
		{
		case MouseEvent::BUTTON0_MASK:
			return CEGUI::LeftButton;
			break;

		case MouseEvent::BUTTON1_MASK:
			return CEGUI::RightButton;
			break;

		case MouseEvent::BUTTON2_MASK:
			return CEGUI::MiddleButton;
			break;

		case MouseEvent::BUTTON3_MASK:
			return CEGUI::X1Button;
			break;

		default:
			return CEGUI::LeftButton;
			break;
		}

	}



	void updateStats(void)
	{
		static CEGUI::String currFps = (CEGUI::utf8*)"Current FPS: ";
		static CEGUI::String avgFps = (CEGUI::utf8*)"Average FPS: ";
		static CEGUI::String bestFps = (CEGUI::utf8*)"Best FPS: ";
		static CEGUI::String worstFps = (CEGUI::utf8*)"Worst FPS: ";
		static CEGUI::String tris = (CEGUI::utf8*)"Triangle Count: ";

		if (mSkipCount >= mUpdateFreq)
		{
			mSkipCount = 0;

			try 
			{
				CEGUI::Window* guiAvg = CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"OPAverageFPS");
				CEGUI::Window* guiCurr = CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"OPCurrentFPS");
				CEGUI::Window* guiBest = CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"OPBestFPS");
				CEGUI::Window* guiWorst = CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"OPWorstFPS");

				const RenderTarget::FrameStats& stats = mWindow->getStatistics();

				guiAvg->setText(avgFps + StringConverter::toString(stats.avgFPS));
				guiCurr->setText(currFps + StringConverter::toString(stats.lastFPS));
				guiBest->setText(bestFps + StringConverter::toString(stats.bestFPS)
					+" "+StringConverter::toString(stats.bestFrameTime)+" ms");
				guiWorst->setText(worstFps + StringConverter::toString(stats.worstFPS)
					+" "+StringConverter::toString(stats.worstFrameTime)+" ms");

				CEGUI::Window* guiTris = CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"OPTriCount");
				guiTris->setText(tris + StringConverter::toString(stats.triangleCount));

				CEGUI::Window* guiDbg = CEGUI::WindowManager::getSingleton().getWindow((CEGUI::utf8*)"OPDebugMsg");
				guiDbg->setText(mWindow->getDebugText());
			}
			catch (CEGUI::UnknownObjectException x)
			{
				// just skip if windows are missing
			}

		}
		else
		{
			mSkipCount++;
		}

	}


protected:
	float	mSkipCount;
	float	mUpdateFreq;

	bool mQuit;
	MovieLogic *mMovieControl;
};

class CEGUIOgre_TestDriver1 : public ExampleApplication
{
public:
	CEGUIOgre_TestDriver1(void) : mGUIRenderer(NULL), mMovieControl(0) {}

	void	cleanup(void);

protected:
	/*************************************************************************
		Methods
	*************************************************************************/
	void	createScene(void);
	void	createFrameListener(void);
	void	createCrazyPanel(void);

	// new demo methods
	void	createDemoWindows(void);
	void	initDemoEventWiring(void);

	bool	handleQuit(const CEGUI::EventArgs& e);
	bool	handleErrorBox(const CEGUI::EventArgs& e);

	bool handlePlay(const CEGUI::EventArgs& e);
	bool handleStop(const CEGUI::EventArgs& e);
	bool handlePause(const CEGUI::EventArgs& e);
	bool handleResume(const CEGUI::EventArgs& e);

	bool handleActivate(const CEGUI::EventArgs& e);

	bool handleSeekStart(const CEGUI::EventArgs& e);
	bool handleSeek(const CEGUI::EventArgs& e);
	bool handleSeekDone(const CEGUI::EventArgs& e);

	/*************************************************************************
		Data
	*************************************************************************/
	CEGUI::OgreCEGUIRenderer* mGUIRenderer;
	
	MovieLogic *mMovieControl;
};


#endif	// end of guard _CEGUIOgre_TestDriver1_h_

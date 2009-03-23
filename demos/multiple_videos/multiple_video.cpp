#include "OgreExternalTextureSourceManager.h"
#include "ExampleApplication.h"
#include "TheoraVideoManager.h"
#include "TheoraVideoClip.h"

class GuiFrameListener : public OIS::KeyListener, public OIS::MouseListener, public ExampleFrameListener
{
private:
    bool mShutdownRequested;
	bool init;
	//ClipListener mMovieListener;

public:
    // NB using buffered input, this is the only change
	GuiFrameListener(RenderWindow* win, Camera* cam)
        : ExampleFrameListener(win, cam, true, true, true), 
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
        if (mShutdownRequested)
            return false;
        else
            return ExampleFrameListener::frameStarted(evt);
    }

	//----------------------------------------------------------------//
	bool mouseMoved( const OIS::MouseEvent &arg )
	{
		return true;
	}

	//----------------------------------------------------------------//
	bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
	{
		return true;
	}

	//----------------------------------------------------------------//
	bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
	{

		return true;
	}

	//----------------------------------------------------------------//
	bool keyPressed( const OIS::KeyEvent &arg )
	{
		if( arg.key == OIS::KC_ESCAPE )
			mShutdownRequested = true;
		return true;
	}

	//----------------------------------------------------------------//
	bool keyReleased( const OIS::KeyEvent &arg )
	{
		return true;
	}
};


class GuiApplication : public ExampleApplication
{
private:
	bool mShaders;
public:

	GuiApplication() :
		mShaders(false)
    {



	}

    ~GuiApplication()
    {
		return;
    }

protected:
    // Just override the mandatory create scene method
    void createScene(void)
    {
		mCamera->getViewport()->setBackgroundColour(ColourValue(0.3,0.3,0.3));

		float left[]=  {   -1, 0.01,    -1,  0.01};
		float top[]=   {    1,    1, -0.01, -0.01};
		float right[]= {-0.01,    1, -0.01,     1};
		float bottom[]={ 0.01, 0.01,    -1,    -1};
		String materials[]={"konqi","fedora01","fedora02","fedora03"};
		for (int i=0;i<4;i++)
		{
			ManualObject* model = mSceneMgr->createManualObject("quad"+StringConverter::toString(i));
			model->begin(materials[i]);

			model->position( right[i],bottom[i],0);
			model->textureCoord(1,1);

			model->position( right[i],top[i],0);
			model->textureCoord(1,0);

			model->position(left[i],top[i],0);
			model->textureCoord(0,0);

			model->position(left[i],top[i],0);
			model->textureCoord(0,0);

			model->position( right[i],bottom[i],0);
			model->textureCoord(1,1);

			model->position(left[i],bottom[i],0);
			model->textureCoord(0,1);

			model->end();

			model->setUseIdentityProjection(true);
			model->setUseIdentityView(true);
			

			SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
			node->attachObject(model);
		}


    }

    // Create new frame listener
    void createFrameListener(void)
    {
        mFrameListener= new GuiFrameListener(mWindow, mCamera);
        mRoot->addFrameListener(mFrameListener);
    }

	void loadResources()
	{
		ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
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

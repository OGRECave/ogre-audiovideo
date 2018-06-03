/************************************************************************************
This source file is part of the Ogre3D Theora Video Plugin
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef __TheoraDemoApp_H__
#define __TheoraDemoApp_H__

#include "OgreApplicationContext.h"
#include "OgreExternalTextureSourceManager.h"
#include "Ogre.h"

#include "OgreVideoManager.h"
#include "TheoraVideoManager.h"
#include "TheoraVideoClip.h"

#include <iostream>


namespace Ogre
{
	SceneManager* SceneMgr;

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
		model->setBoundingBox(AxisAlignedBox(AxisAlignedBox::BOX_INFINITE));
		// and atach it to the root node
		SceneNode* node = SceneMgr->getRootSceneNode()->createChildSceneNode();
		node->attachObject(model);
	}

	TheoraVideoClip* getClip(String name)
	{
		TheoraVideoManager* mgr = TheoraVideoManager::getSingletonPtr();
		return mgr->getVideoClipByName(name);
	}


	class TheoraDemoApp : public OgreBites::InputListener
	{
	public:
		virtual void init() = 0;
	};

	TheoraDemoApp* demo_app;
	TheoraDemoApp* start();

	class TheoraDemoApplication : public OgreBites::ApplicationContext
	{
	private:
		bool mShaders;
	public:

		TheoraDemoApplication()
		  : OgreBites::ApplicationContext("TheoraDemo"), mShaders(false)
		{

		}


	protected:

		void setup()
		{
		    OgreBites::ApplicationContext::setup();

		    Root* root = getRoot();
		    SceneManager* scnMgr = root->createSceneManager();

		    RTShader::ShaderGenerator* shadergen = RTShader::ShaderGenerator::getSingletonPtr();
		    shadergen->addSceneManager(scnMgr);

			SceneMgr=scnMgr; // make a global shortcut

			Camera* cam = scnMgr->createCamera("myCam");
			getRenderWindow()->addViewport(cam);
            cam->getViewport()->setBackgroundColour(ColourValue(0.3,0.3,0.3));

			demo_app=start();
			demo_app->init();
			addInputListener(demo_app);
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
		app.initApp();
		app.getRoot()->startRendering();
		app.closeApp();
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

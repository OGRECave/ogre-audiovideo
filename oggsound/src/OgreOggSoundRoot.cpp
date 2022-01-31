/**
* @author  Ian Stangoe
*
* LICENSE
* 
* This source file is part of OgreOggSound, an OpenAL wrapper library for   
* use with the Ogre Rendering Engine.										 
*                                                                           
* Copyright (c) 2017 Ian Stangoe
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE. 	 
*
*/

#include "OgreOggSoundRoot.h"

#if OGRE_VERSION_MAJOR == 2
OgreOggSound::Root* mOgreOggSoundPlugin;
const Ogre::String sPluginName = "OgreOggSound";

using namespace OgreOggSound;

extern "C" void _OGGSOUND_EXPORT dllStartPlugin( void )
{
	// Create new plugin
	mOgreOggSoundPlugin = OGRE_NEW_T(Root, Ogre::MEMCATEGORY_GENERAL)();

	// Register
	Ogre::Root::getSingleton().installPlugin(mOgreOggSoundPlugin);
}

extern "C" void _OGGSOUND_EXPORT dllStopPlugin( void )
{
	Ogre::Root::getSingleton().uninstallPlugin(mOgreOggSoundPlugin);

	OGRE_DELETE_T(mOgreOggSoundPlugin, Root, Ogre::MEMCATEGORY_GENERAL);
	mOgreOggSoundPlugin = 0;
}
#endif

namespace OgreOggSound
{
	//---------------------------------------------------------------------
	Root::Root() :
	 mOgreOggSoundFactory(0)
	,mOgreOggSoundManager(0)
	{

	}
	
	#if OGRE_VERSION_MAJOR == 2
	const Ogre::String& Root::getName() const
	{
		return sPluginName;
	}
	//---------------------------------------------------------------------
	void Root::install()
	{
		if ( mOgreOggSoundFactory ) return;

		// Create new factory
		mOgreOggSoundFactory = OGRE_NEW_T(OgreOggSoundFactory, Ogre::MEMCATEGORY_GENERAL)();

		// Register
		Ogre::Root::getSingleton().addMovableObjectFactory(mOgreOggSoundFactory, true);
	}
	//---------------------------------------------------------------------
	void Root::initialise()
	{
		if ( mOgreOggSoundManager ) return;

		//initialise OgreOggSoundManager here
		mOgreOggSoundManager = OGRE_NEW_T(OgreOggSoundManager, Ogre::MEMCATEGORY_GENERAL)();
	}
	//---------------------------------------------------------------------
	void Root::shutdown()
	{
		if ( !mOgreOggSoundManager ) return;

		// shutdown OgreOggSoundManager here
		OGRE_DELETE_T(mOgreOggSoundManager, OgreOggSoundManager, Ogre::MEMCATEGORY_GENERAL);
		mOgreOggSoundManager = 0;
	}
	//---------------------------------------------------------------------
	void Root::uninstall()
	{
		if ( !mOgreOggSoundFactory ) return;

		// unregister
		Ogre::Root::getSingleton().removeMovableObjectFactory(mOgreOggSoundFactory);

		OGRE_DELETE_T(mOgreOggSoundFactory, OgreOggSoundFactory, Ogre::MEMCATEGORY_GENERAL);
		mOgreOggSoundFactory = 0;
	}
	#else
	//---------------------------------------------------------------------
	void Root::initialise()
	{
		if ( mOgreOggSoundManager ) return;

		// Create new factory
		mOgreOggSoundFactory = OGRE_NEW_T(OgreOggSoundFactory, Ogre::MEMCATEGORY_GENERAL)();

		// Register
		Ogre::Root::getSingleton().addMovableObjectFactory(mOgreOggSoundFactory, true);

		//initialise OgreOggSoundManager here
		mOgreOggSoundManager = OGRE_NEW_T(OgreOggSoundManager, Ogre::MEMCATEGORY_GENERAL)();
	}
	//---------------------------------------------------------------------
	void Root::shutdown()
	{
		if ( !mOgreOggSoundManager ) return;

		// shutdown OgreOggSoundManager here
		OGRE_DELETE_T(mOgreOggSoundManager, OgreOggSoundManager, Ogre::MEMCATEGORY_GENERAL);
		mOgreOggSoundManager = 0;

		// unregister
		Ogre::Root::getSingleton().removeMovableObjectFactory(mOgreOggSoundFactory);

		OGRE_DELETE_T(mOgreOggSoundFactory, OgreOggSoundFactory, Ogre::MEMCATEGORY_GENERAL);
		mOgreOggSoundFactory = 0;
	}
	#endif
}

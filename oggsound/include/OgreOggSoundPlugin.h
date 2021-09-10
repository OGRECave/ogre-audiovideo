/**
* @author  Ian Stangoe
*
* LICENSE:
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
* DESCRIPTION: Impements the plugin interface for OGRE
*/

#ifndef __OGREOGGSOUNDPlugin_H__
#define __OGREOGGSOUNDPlugin_H__

#include <OgrePlugin.h>

#include "OgreOggSound.h"
#include "OgreOggSoundFactory.h"

namespace OgreOggSound
{
	//! Plugin instance for the MovableText 
	class OgreOggSoundPlugin : public Ogre::Plugin
	{
	public:
		OgreOggSoundPlugin();

        /** Get the name of the plugin.
        @remarks An implementation must be supplied for this method to uniquely identify the plugin.
        */
		const Ogre::String& getName() const;

		/** Perform the plugin initial installation sequence.
		@remarks An implementation must be supplied for this method.\n
			It must perform the startup tasks necessary to install any rendersystem customisations or anything else that is not dependent on system initialisation, ie only dependent on the core of Ogre.\n
			It must not perform any operations that would create rendersystem-specific objects at this stage, that should be done in initialise().
		*/
		void install();

		/** Perform any tasks the plugin needs to perform on full system initialisation.
		@remarks An implementation must be supplied for this method.\n
			It is called just after the system is fully initialised (either after Root::initialise if a window is created then, or after the first window is created) 
			and therefore all rendersystem functionality is available at this time.\n
			You can use this hook to create any resources which are dependent on a rendersystem or have rendersystem-specific implementations.
        */
		void initialise();

		/** Perform any tasks the plugin needs to perform when the system is shut down.
		@remarks An implementation must be supplied for this method.\n
			This method is called just before key parts of the system are unloaded, such as rendersystems being shut down.\n
			You should use this hook to free up  resources and decouple custom objects from the OGRE system, whilst all the instances of other plugins (e.g. rendersystems) still exist.
		*/
		void shutdown();

		/** Perform the final plugin uninstallation sequence.
		@remarks An implementation must be supplied for this method.\n
			It must perform the cleanup tasks which haven't already been performed in shutdown() (e.g. final deletion of custom instances, if you kept them around incase the system was reinitialised).\n
			At this stage you cannot be sure what other plugins are still loaded or active.\n
			It must therefore not perform any operations that would reference any rendersystem-specific objects - those should have been sorted out in the 'shutdown' method.
		*/
		void uninstall();

	protected:
		OgreOggSoundFactory* mOgreOggSoundFactory;
		OgreOggSoundManager* mOgreOggSoundManager;
	};
}

#endif

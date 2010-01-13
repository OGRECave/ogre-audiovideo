/************************************************************************************
This source file is part of the Ogre3D Theora Video Plugin
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)

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
#ifndef OGRE_MAC_FRAMEWORK
  #include "OgreRoot.h"
#else
  #include <Ogre/OgreRoot.h>
#endif
#include "OgreVideoManager.h"
#include "TheoraVideoManager.h"

namespace Ogre
{
	// Singleton code
    template<> OgreVideoManager* Singleton<OgreVideoManager>::ms_Singleton = 0;
    OgreVideoManager* OgreVideoManager::getSingletonPtr(void)
    {
        return ms_Singleton;
    }
    OgreVideoManager& OgreVideoManager::getSingleton(void)
    {  
        assert( ms_Singleton );  return ( *ms_Singleton );  
    }

	OgreVideoManager::OgreVideoManager()
	{
		mPlugInName = "TheoraVideoPlugin";
		mDictionaryName = mPlugInName;
		mbInit=false;
		mTechniqueLevel=mPassLevel=mStateLevel=0;

		initialise();
	}

	bool OgreVideoManager::initialise()
	{
		if (mbInit) return false;
		mbInit=true;
		addBaseParams(); // ExternalTextureSource's function

		mVideoMgr=new TheoraVideoManager(1);

		return true;
	}
	
	OgreVideoManager::~OgreVideoManager()
	{
		shutDown();
	}

	void OgreVideoManager::shutDown()
	{
		if (!mbInit) return;

		delete mVideoMgr;

		mbInit=false;
	}
    
    bool OgreVideoManager::setParameter(const String &name,const String &value)
    {
        // Hacky stuff used in situations where you don't have access to OgreVideoManager
        // eg, like when using the plugin in python (and not using the wrapped version by Python-Ogre)
        // these parameters are here temporarily and I don't encourage anyone to use them.
        
        if (name == "destroy")
        {

        }
        
        return ExternalTextureSource::setParameter(name, value);
    }
    
    String OgreVideoManager::getParameter(const String &name) const
    {
        // Hacky stuff used in situations where you don't have access to OgreVideoManager
        // eg, like when using the plugin in python (and not using the wrapped version by Python-Ogre)
        // these parameters are here temporarily and I don't encourage anyone to use them.

        if (name == "started")
        {
            
        }
        else if (name == "finished")
        {
  
        }
        return ExternalTextureSource::getParameter(name);
    }


	void OgreVideoManager::createDefinedTexture(const String& material_name,const String& group_name)
	{

	}

	void OgreVideoManager::destroyAdvancedTexture(const String& material_name,const String& groupName)
	{
		LogManager::getSingleton().logMessage("Destroying ogg_video texture on material: "+material_name);

		LogManager::getSingleton().logMessage("Error destroying ogg_video texture, texture not found!");
	}

	bool OgreVideoManager::frameStarted(const FrameEvent& evt)
	{

		return true;
	}

} // end namespace Ogre

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
#include "OgreRoot.h"
#include "TheoraVideoManager.h"
#include "TheoraWorkerThread.h"
#include "TheoraVideoClip.h"
#include "TheoraAudioInterface.h"

namespace Ogre
{
	// declaring function prototype here so I don't have to put it in a header file
	// it only needs to be used by this plugin and called once
	void createYUVtoRGBtables();

	// Singleton code
    template<> TheoraVideoManager* Singleton<TheoraVideoManager>::ms_Singleton = 0;
    TheoraVideoManager* TheoraVideoManager::getSingletonPtr(void)
    {
        return ms_Singleton;
    }
    TheoraVideoManager& TheoraVideoManager::getSingleton(void)
    {  
        assert( ms_Singleton );  return ( *ms_Singleton );  
    }

	TheoraVideoManager::TheoraVideoManager()
	{
		mPlugInName = "TheoraVideoPlugin";
		mAudioFactory = NULL;
		mDictionaryName = mPlugInName;
		mbInit=false;

	}

	bool TheoraVideoManager::initialise()
	{
		if (mbInit) return false;
		addBaseParams(); // ExternalTextureSource's function

		// create worker threads

		TheoraWorkerThread* t;
		for (int i=0;i<1;i++)
		{
			t=new TheoraWorkerThread();
			t->start();
			mWorkerThreads.push_back(t);
		}

		// for CPU yuv2rgb decoding
		createYUVtoRGBtables();

		mbInit=true;
		return true;
	}
	
	TheoraVideoManager::~TheoraVideoManager()
	{
		shutDown();
	}

	void TheoraVideoManager::shutDown()
	{
		if (!mbInit) return;

		ThreadList::iterator ti;
		for (ti=mWorkerThreads.begin(); ti != mWorkerThreads.end();ti++)
			delete (*ti);
		mWorkerThreads.clear();

		ClipList::iterator ci;
		for (ci=mClips.begin(); ci != mClips.end();ci++)
			delete (*ci);
		mClips.clear();

		mbInit=false;
	}

	TheoraVideoClip* TheoraVideoManager::getVideoClipByName(String name)
	{
		ClipList::iterator ci;
		for (ci=mClips.begin(); ci != mClips.end();ci++)
			if ((*ci)->getName() == name) return *ci;

		return 0;
	}

	void TheoraVideoManager::setAudioInterfaceFactory(TheoraAudioInterfaceFactory* factory)
	{
		mAudioFactory=factory;
	}
	
	TheoraAudioInterfaceFactory* TheoraVideoManager::getAudioInterfaceFactory()
	{
		return mAudioFactory;
	}

	void TheoraVideoManager::createDefinedTexture(const String& material_name,const String& group_name)
	{
		TheoraVideoClip* clip = NULL;

		LogManager::getSingleton().logMessage("Creating ogg_video texture on material: "+material_name);

		clip = new TheoraVideoClip(material_name,64);
		try 
		{
			clip->createDefinedTexture(mInputFileName, material_name, group_name, mTechniqueLevel,
						  mPassLevel, mStateLevel);

			//int n=(mNumPrecachedFrames == -1) ? 16 : mNumPrecachedFrames;
			//clip->setNumPrecachedFrames(n);
			//clip->setOutputMode(mOutputMode);
		}
		catch(...)
		{
			LogManager::getSingleton().logMessage("Error creating ogg_video texture!");
			delete clip;
			return;
		}

		/*
		// reset variables for a new movie
		mNumPrecachedFrames=-1;
		mOutputMode=TH_RGB;
		mInputFileName = "None";
		mTechniqueLevel = mPassLevel = mStateLevel = 0;
		mSeekEnabled = false;
		mAutoUpdate = false;
        */

		// push the clip into the list at the very end, to ensure worker threads
		// don't start decoding until the clip is fully initialised
		mClips.push_back(clip);
	}

	void TheoraVideoManager::destroyAdvancedTexture(const String& material_name,const String& groupName)
	{
		LogManager::getSingleton().logMessage("Destroying ogg_video texture on material: "+material_name);

		for (ClipList::iterator i=mClips.begin();i != mClips.end();i++)
		{
			if ((*i)->getMaterialName() == material_name)
			{
				delete (*i);
				mClips.erase(i);
				return;
			}
		}
		
		LogManager::getSingleton().logMessage("Error destroying ogg_video texture, texture not found!");
	}

	bool TheoraVideoManager::frameStarted(const FrameEvent& evt)
	{

		ClipList::iterator ci;
		for (ci=mClips.begin(); ci != mClips.end();ci++)
		{
			(*ci)->blitFrameCheck(evt.timeSinceLastFrame);
			(*ci)->decodedAudioCheck();
		}
		return true;
	}

	TheoraVideoClip* TheoraVideoManager::requestWork()
	{
		if (mClips.size() == 0) return NULL;
		return mClips.front();
	}


} // end namespace Ogre

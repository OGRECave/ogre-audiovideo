/************************************************************************************
This source file is part of the Ogre3D Theora Video Plugin
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef OGRE_MAC_FRAMEWORK
  #include "OgreRoot.h"
#else
  #include <Ogre/OgreRoot.h>
#endif
#include "OgreVideoManager.h"
#include "OgreTheoraDataStream.h"

#ifndef OGRE_MAC_FRAMEWORK
#include "OgreTextureManager.h"
#include "OgreMaterialManager.h"
#include "OgreMaterial.h"
#include "OgreTechnique.h"
#include "OgreStringConverter.h"
#include "OgreLogManager.h"
#include "OgreHardwarePixelBuffer.h"
#else
#include <Ogre/OgreTextureManager.h>
#include <Ogre/OgreMaterialManager.h>
#include <Ogre/OgreMaterial.h>
#include <Ogre/OgreTechnique.h>
#include <Ogre/OgreStringConverter.h>
#include <Ogre/OgreLogManager.h>
#include <Ogre/OgreHardwarePixelBuffer.h>
#endif

#include "OgreExternalTextureSourceManager.h"
#include "TheoraVideoFrame.h"
#include "TheoraTimer.h"
#include <vector>

#if OGRE_VERSION_MAJOR >= 2 && OGRE_VERSION_MINOR >= 1
#include <OgreHlmsManager.h>
#include <Hlms/Unlit/OgreHlmsUnlit.h>
#include <Hlms/Unlit/OgreHlmsUnlitDatablock.h>
using namespace Ogre::v1;
#endif

namespace Ogre
{
	int nextPow2(int x)
	{
		int y;
		for (y=1;y<x;y*=2);
		return y;
	}

	class ManualTimer : public TheoraTimer
	{
	public:
		void update(float t)
		{
		
		}

		void setTime(float time)
		{
			mTime=time;
		}
	};

	OgreVideoManager::OgreVideoManager(int num_worker_threads) : TheoraVideoManager(num_worker_threads)
	{
		mDictionaryName = "TheoraVideoPlugin";
		mbInit=false;
		mTechniqueLevel=mPassLevel=mStateLevel=0;

		initialise();
	}

	bool OgreVideoManager::initialise()
	{
		if (mbInit) return false;
		mbInit=true;
		addBaseParams(); // ExternalTextureSource's function
		return true;
	}
	
	OgreVideoManager::~OgreVideoManager()
	{
		shutDown();
	}

	void OgreVideoManager::shutDown()
	{
		if (!mbInit) return;

		mbInit=false;
	}
    
    bool OgreVideoManager::setParameter(const String &name,const String &value)
    {
        return ExternalTextureSource::setParameter(name, value);
    }
    
    String OgreVideoManager::getParameter(const String &name) const
    {
        return ExternalTextureSource::getParameter(name);
    }

	void OgreVideoManager::createDefinedTexture(const String& material_name,const String& group_name)
	{
		std::string name=mInputFileName;
		TheoraVideoClip* clip=createVideoClip(new OgreTheoraDataStream(mInputFileName,group_name),TH_RGBA,0,1);
		int w=nextPow2(clip->getWidth()),h=nextPow2(clip->getHeight());

		TexturePtr t = TextureManager::getSingleton().createManual(name,group_name,TEX_TYPE_2D,w,h,1,0,PF_BYTE_RGBA,TU_DYNAMIC_WRITE_ONLY);
		
		if (t->getFormat() != PF_BYTE_RGBA) logMessage("ERROR: Pixel format is not BYTE_RGBA which is what was requested!");
		// clear it to black

		unsigned char* texData=(unsigned char*) t->getBuffer()->lock(HardwareBuffer::HBL_DISCARD);

		memset(texData,0,w*h*4);
		t->getBuffer()->unlock();
		mTextures[name]=t;

#if OGRE_VERSION_MAJOR >= 2 && OGRE_VERSION_MINOR >= 1
		// set it in a datablock
		HlmsUnlitDatablock* ogreDatablock = static_cast<Ogre::HlmsUnlitDatablock*>(
			Root::getSingletonPtr()->getHlmsManager()->getHlms(HLMS_UNLIT)->getDatablock(material_name)
		);
		ogreDatablock->setTexture( 0, 0, t );
		
		// scale tex coords to fit the 0-1 uv range
		Matrix4 mat = Matrix4::IDENTITY;
		mat.setScale(Vector3((float) clip->getWidth()/w, (float) clip->getHeight()/h,1));
		ogreDatablock->setAnimationMatrix( 0, mat );
		ogreDatablock->setEnableAnimationMatrix( 0, true );
#else
		// attach it to a material
		MaterialPtr material = MaterialManager::getSingleton().getByName(material_name);
		TextureUnitState* ts = material->getTechnique(mTechniqueLevel)->getPass(mPassLevel)->getTextureUnitState(mStateLevel);

		//Now, attach the texture to the material texture unit (single layer) and setup properties
		ts->setTextureName(name,TEX_TYPE_2D);
		ts->setTextureFiltering(FO_LINEAR, FO_LINEAR, FO_NONE);
		ts->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);

		// scale tex coords to fit the 0-1 uv range
		Matrix4 mat=Matrix4::IDENTITY;
		mat.setScale(Vector3((float) clip->getWidth()/w, (float) clip->getHeight()/h,1));
		ts->setTextureTransform(mat);
#endif
	}

	void OgreVideoManager::destroyAdvancedTexture(const String& material_name,const String& groupName)
	{
		logMessage("Destroying ogg_video texture on material: "+material_name);

		//logMessage("Error destroying ogg_video texture, texture not found!");
	}

	bool OgreVideoManager::frameStarted(const FrameEvent& evt)
	{
		if (evt.timeSinceLastFrame > 0.3f)
			update(0.3f);
		else
		    update(evt.timeSinceLastFrame);

		// update playing videos
		std::vector<TheoraVideoClip*>::iterator it;
		TheoraVideoFrame* f;
		for (it=mClips.begin();it!=mClips.end();it++)
		{
			f=(*it)->getNextFrame();
			if (f)
			{
				int w=f->getStride(),h=f->getHeight();
				TexturePtr t=mTextures[(*it)->getName()];

				unsigned char *texData=(unsigned char*) t->getBuffer()->lock(HardwareBuffer::HBL_DISCARD);
				unsigned char *videoData=f->getBuffer();

				memcpy(texData,videoData,w*h*4);

				t->getBuffer()->unlock();
				(*it)->popFrame();
			}
		}
		return true;
	}

    static void ogrevideo_log(std::string message)
    {
        Ogre::LogManager::getSingleton().logMessage("OgreVideo: "+message);
    }

    const String& OgreVideoPlugin::getName() const
    {
        static String name = "TheoraVideoPlugin";
        return name;
    }
    void OgreVideoPlugin::initialise()
    {
        TheoraVideoManager::setLogFunction(ogrevideo_log);
        // Create our new External Texture Source PlugIn
        mVideoMgr = new OgreVideoManager();

        // Register with Manager
        ExternalTextureSourceManager::getSingleton().setExternalTextureSource("ogg_video",mVideoMgr);
        Root::getSingleton().addFrameListener(mVideoMgr);
    }
    void OgreVideoPlugin::shutdown()
    {
        Root::getSingleton().removeFrameListener(mVideoMgr);
        delete mVideoMgr;
    }

} // end namespace Ogre

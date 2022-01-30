/************************************************************************************
This source file is part of the Ogre3D Theora Video Plugin
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/

#include <OgreRoot.h>

#include "OgreVideoManager.h"
#include "OgreTheoraDataStream.h"

#include <OgreTextureManager.h>
#include <OgreMaterialManager.h>
#include <OgreMaterial.h>
#include <OgreTechnique.h>
#include <OgreStringConverter.h>
#include <OgreLogManager.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreBitwise.h>
#include <OgreExternalTextureSourceManager.h>

#include "TheoraVideoFrame.h"
#include "TheoraTimer.h"
#include <vector>

#if OGRE_VERSION_MAJOR == 2 && OGRE_VERSION_MINOR >= 1
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

	OgreVideoManager::OgreVideoManager(int num_worker_threads) : TheoraVideoManager(num_worker_threads)
	{
		mDictionaryName = "TheoraVideoPlugin";
		mbInit=false;
		mbPaused=false;
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

	void OgreVideoManager::createDefinedTexture(const String& material_name,const String& group_name)
	{
		createVideoTexture(mInputFileName, material_name, group_name, group_name);
	}
	
	TheoraVideoClip* OgreVideoManager::createVideoTexture(
		const String& video_file_name, const String& material_name,
		const String& video_group_name, const String& group_name
	) {
		const String& name = material_name;
		
		if (mClipsTextures.find(name) != mClipsTextures.end()) {
			logMessage("ERROR: OgreVideo texture \"" + name + "\" exist, destroy old texture first.");
			return NULL;
		}
		
		TheoraVideoClip* clip=createVideoClip(new OgreTheoraDataStream(video_file_name,video_group_name),TH_RGBA,0,1);
		int w=Bitwise::firstPO2From(clip->getWidth()), h=Bitwise::firstPO2From(clip->getHeight());

		// scale tex coords to fit the 0-1 uv range
		Matrix4 mat=Matrix4::IDENTITY;
		mat.setScale(Vector3((float) clip->getWidth()/(w + 0.5), (float) clip->getHeight()/(h + 0.5),1));

		TexturePtr t = TextureManager::getSingleton().createManual(name,group_name,TEX_TYPE_2D,w,h,1,0,PF_BYTE_RGBA,TU_DYNAMIC_WRITE_ONLY);
		
		if (t->getFormat() != PF_BYTE_RGBA) logMessage("ERROR: Pixel format is not BYTE_RGBA which is what was requested!");
		// clear it to black

		unsigned char* texData=(unsigned char*) t->getBuffer()->lock(HardwareBuffer::HBL_DISCARD);

		memset(texData,0,w*h*4);
		t->getBuffer()->unlock();
		mClipsTextures[name]={clip,t};

#if OGRE_VERSION_MAJOR == 2 && OGRE_VERSION_MINOR >= 1
		// set it in a datablock
		HlmsUnlitDatablock* ogreDatablock = static_cast<Ogre::HlmsUnlitDatablock*>(
			Root::getSingletonPtr()->getHlmsManager()->getHlms(HLMS_UNLIT)->getDatablock(material_name)
		);
		ogreDatablock->setTexture( 0, 0, t );
		ogreDatablock->setAnimationMatrix( 0, mat );
		ogreDatablock->setEnableAnimationMatrix( 0, true );
#else
		// attach it to a material
		MaterialPtr material = MaterialManager::getSingleton().getByName(material_name);
		TextureUnitState* ts = material->getTechnique(mTechniqueLevel)->getPass(mPassLevel)->getTextureUnitState(mStateLevel);

		//Now, attach the texture to the material texture unit (single layer) and setup properties
		ts->setTextureName(name,TEX_TYPE_2D);
		ts->setTextureTransform(mat);
#endif

		if(mMode == TextureEffectPause)
			clip->pause();
		else if(mMode == TextureEffectPlay_Looping)
			clip->setAutoRestart(true);

		return clip;
	}

	void OgreVideoManager::destroyAdvancedTexture(const String& material_name,const String& group_name)
	{
		logMessage("Destroying ogg_video texture on material: "+material_name);
		
		std::map<String,ClipTexture>::iterator it = mClipsTextures.find(material_name);
		destroyVideoClip(it->second.clip); // OgreTheoraDataStream will be destroyed in TheoraVideoClip destructor
		mClipsTextures.erase(it);
	}
	
	void OgreVideoManager::destroyAllVideoTextures() {
		for (std::map<String,ClipTexture>::iterator it=mClipsTextures.begin(); it!=mClipsTextures.end(); it++) {
			destroyVideoClip(it->second.clip);
		}
		mClipsTextures.clear();
	}

	bool OgreVideoManager::frameStarted(const FrameEvent& evt)
	{
		if (mbPaused)
			return true;
		
		if (evt.timeSinceLastFrame > 0.3f)
			update(0.3f);
		else
			update(evt.timeSinceLastFrame);

		// update playing videos
		TheoraVideoFrame* f;
		for (std::map<String,ClipTexture>::iterator it=mClipsTextures.begin(); it!=mClipsTextures.end(); it++)
		{
			f=it->second.clip->getNextFrame();
			if (f)
			{
				int w=f->getStride(),h=f->getHeight();
				
				unsigned char *texData=(unsigned char*) it->second.texture->getBuffer()->lock(HardwareBuffer::HBL_DISCARD);
				unsigned char *videoData=f->getBuffer();
				
				memcpy(texData,videoData,w*h*4);
				
				it->second.texture->getBuffer()->unlock();
				it->second.clip->popFrame();
			}
		}
		return true;
	}

	void OgreVideoManager::pauseAllVideoClips() {
		mbPaused = true;
	}
	
	void OgreVideoManager::unpauseAllVideoClips() {
		mbPaused = false;
	}
	
	TheoraVideoClip* OgreVideoManager::getVideoClipByMaterialName(const String& material_name) {
		std::map<String,ClipTexture>::iterator it = mClipsTextures.find(material_name);
		if (it != mClipsTextures.end())
			return it->second.clip;
		else
			return NULL;
	}
	
	static void ogrevideo_log(std::string message)
	{
		Ogre::LogManager::getSingleton().logMessage("OgreVideo: "+message);
	}
	
	OgreVideoManager* OgreVideoPlugin::mVideoMgr = 0;
	
	const String& OgreVideoPlugin::getName() const
	{
		static String name = "TheoraVideoPlugin";
		return name;
	}
	void OgreVideoPlugin::install()
	{
		if (mVideoMgr) {
			Ogre::LogManager::getSingleton().logWarning(
				"OgreVideoPlugin was already been initialized ... ignoring next initialise of plugin"
			);
			return;
		}
		
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
		mVideoMgr = 0;
	}

} // end namespace Ogre

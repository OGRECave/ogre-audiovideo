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

#if AV_OGRE_NEXT_VERSION >= 0x20200
#include <OgreTextureGpuManager.h>
#include <OgreTextureBox.h>
#include <OgreStagingTexture.h>
#else
#include <OgreTextureManager.h>
#include <OgreHardwarePixelBuffer.h>
#endif
#include <OgreMaterialManager.h>
#include <OgreMaterial.h>
#include <OgreTechnique.h>
#include <OgreStringConverter.h>
#include <OgreLogManager.h>
#include <OgreBitwise.h>
#include <OgreExternalTextureSourceManager.h>

#include "TheoraVideoFrame.h"
#include "TheoraTimer.h"
#include <vector>

#if AV_OGRE_NEXT_VERSION >= 0x20100
#include <OgreHlmsManager.h>
#include <Hlms/Unlit/OgreHlmsUnlit.h>
#include <Hlms/Unlit/OgreHlmsUnlitDatablock.h>
using namespace Ogre::v1;
#endif

#if AV_OGRE_NEXT_VERSION > 0x30000
#include <OgreAbiUtils.h>
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
	
#if AV_OGRE_NEXT_VERSION >= 0x20100
	static void fillTexture(TextureGpu* texture, const uint8* data, int xSize, int ySize) {
		TextureGpuManager *textureMgr = Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
		StagingTexture *stagingTexture = textureMgr->getStagingTexture( xSize, ySize, 1, 1, PFG_RGBA8_UNORM );
		
		stagingTexture->startMapRegion();
		TextureBox texBox = stagingTexture->mapRegion( xSize, ySize, 1, 1, PFG_RGBA8_UNORM );
		texBox.copyFrom( data, xSize, ySize, 4 * xSize );
		stagingTexture->stopMapRegion();
		
		stagingTexture->upload( texBox, texture, 0, 0, 0, true );
		textureMgr->removeStagingTexture( stagingTexture );
	}
#endif

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

#if AV_OGRE_NEXT_VERSION >= 0x20200
		TextureGpuManager *textureMgr = Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
		TextureGpu* t = textureMgr->createOrRetrieveTexture(
			name,
			GpuPageOutStrategy::Discard,
			TextureFlags::ManualTexture,
			TextureTypes::Type2D,
			group_name
		);
		t->setPixelFormat(PFG_RGBA8_UNORM);
		t->setNumMipmaps(1u);
		t->setResolution(w, h);
		
		size_t dataSize = 4 * w * h;
		uint8* data = reinterpret_cast<uint8*>(OGRE_MALLOC_SIMD( dataSize, MEMCATEGORY_RENDERSYS ));
		memset( data, 0, dataSize );
		t->_transitionTo( GpuResidency::Resident, reinterpret_cast<uint8*>(data) );
		t->_setNextResidencyStatus( GpuResidency::Resident );
		
		fillTexture(t, data, w, h);
		
		OGRE_FREE_SIMD(data, MEMCATEGORY_RENDERSYS);
		t->notifyDataIsReady();
#else
		TexturePtr t = TextureManager::getSingleton().createManual(name,group_name,TEX_TYPE_2D,w,h,1,0,PF_BYTE_RGBA,TU_DYNAMIC_WRITE_ONLY);
		
		if (t->getFormat() != PF_BYTE_RGBA) logMessage("ERROR: Pixel format is not BYTE_RGBA which is what was requested!");
		// clear it to black

		unsigned char* texData=(unsigned char*) t->getBuffer()->lock(HardwareBuffer::HBL_DISCARD);

		memset(texData,0,w*h*4);
		t->getBuffer()->unlock();
#endif

		mClipsTextures[name]={clip,t};

#if AV_OGRE_NEXT_VERSION >= 0x20100
		// set it in a datablock
		HlmsUnlitDatablock* ogreDatablock = static_cast<Ogre::HlmsUnlitDatablock*>(
			Root::getSingletonPtr()->getHlmsManager()->getHlms(HLMS_UNLIT)->getDatablock(material_name)
		);
	#if AV_OGRE_NEXT_VERSION >= 0x20200
		ogreDatablock->setTexture( 0, name );
	#else
		ogreDatablock->setTexture( 0, 0, t );
	#endif
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
				
#if AV_OGRE_NEXT_VERSION >= 0x20100
				fillTexture(it->second.texture, reinterpret_cast<const uint8*>(f->getBuffer()), w, h);
#else
				unsigned char *texData=(unsigned char*) it->second.texture->getBuffer()->lock(HardwareBuffer::HBL_DISCARD);
				unsigned char *videoData=f->getBuffer();
				
				memcpy(texData,videoData,w*h*4);
				
				it->second.texture->getBuffer()->unlock();
#endif
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
	void OgreVideoPlugin::install
	(
		#if AV_OGRE_NEXT_VERSION > 0x30000
		const NameValuePairList *options
		#endif
	)
	{
		if (mVideoMgr) {
			#if AV_OGRE_NEXT_VERSION >= 0x20000
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL,
				"OgreVideoPlugin was already been initialized ... ignoring next initialise of plugin"
			);
			#else
			Ogre::LogManager::getSingleton().logWarning(
				"OgreVideoPlugin was already been initialized ... ignoring next initialise of plugin"
			);
			#endif
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
	#if AV_OGRE_NEXT_VERSION > 0x30000
	void OgreVideoPlugin::getAbiCookie(AbiCookie &outAbiCookie)
	{
		outAbiCookie = generateAbiCookie();
	}
	#endif

} // end namespace Ogre

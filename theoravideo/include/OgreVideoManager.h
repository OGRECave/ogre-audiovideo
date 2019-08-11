/************************************************************************************
This source file is part of the Ogre3D Theora Video Plugin
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/

#ifndef _OgreVideoManager_h
#define _OgreVideoManager_h

#ifndef OGRE_MAC_FRAMEWORK
  #include "OgreExternalTextureSource.h"
  #include "OgreFrameListener.h"
#else
  #include <Ogre/OgreExternalTextureSource.h>
  #include <Ogre/OgreFrameListener.h>
#endif
#include "OgreVideoExport.h"
#include "TheoraVideoManager.h"
#include "OgrePlugin.h"

#include <string>
#include <map>

namespace Ogre
{
	class _OgreTheoraExport OgreVideoManager : public ExternalTextureSource,
											   public FrameListener,
											   public TheoraVideoManager
	{
	public:
		OgreVideoManager(int num_worker_threads=1);
		~OgreVideoManager();
		
		/**
			@remarks
				Creates video clip and a texture into an already defined material.
				All setting should have been set before calling this.
				Refer to base class ( ExternalTextureSource ) for details
			@param material_name
				Material  you are attaching a movie to.
		*/
		void createDefinedTexture(const String& material_name,
								  const String& group_name = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		
		/**
			@remarks
				Creates video clip and  a texture into an already defined material.
			@param video_file_name
				Video input file name.
			@param material_name
				Material  you are attaching a movie to.
		*/
		TheoraVideoClip* createVideoTexture(const String& video_file_name, const String& material_name,
											const String& video_group_name = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
											const String& group_name = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		
		/**
			@remarks
				Destroys a Video Texture based on material name. Mostly Ogre uses this,
				you should use destroyVideoClip()
			@param material_name
				Material Name you are looking to remove a video clip from
		*/
		void destroyAdvancedTexture(const String& material_name,
									const String& groupName = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		
		/// destroy all video textures
		void destroyAllVideoTextures();
		
		/// pause all video
		void pauseAllVideoClips();
		
		/// unpause all video
		void unpauseAllVideoClips();
		
		/**
			@remarks
				Return video clip based on material name
			@param material_name
				Material Name you are looking to remove a video clip from
		*/
		TheoraVideoClip* getVideoClipByMaterialName(const String& material_name);
		
	private:
		struct ClipTexture {
			TheoraVideoClip*  clip;
			TexturePtr        texture;
		};
		std::map<String,ClipTexture> mClipsTextures;
		bool mbInit;
		bool mbPaused;
		
		// This function is called on start new frame by Ogre - do not call directly
		bool frameStarted(const FrameEvent& evt);
		
		// This function is called to init this plugin - do not call directly
		bool initialise();
		void shutDown();
	};
	
	class _OgreTheoraExport OgreVideoPlugin : public Plugin
	{
		static OgreVideoManager* mVideoMgr;
	public:
		const String& getName() const;
		void install() {}
		void uninstall() {}
		void initialise();
		void shutdown();
	};
}
#endif


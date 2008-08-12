/*
-----------------------------------------------------------------------------
This source file is part of the TheoraVideoSystem ExternalTextureSource PlugIn 
for OGRE (Object-oriented Graphics Rendering Engine)
For the latest info, see www.wreckedgames.com or www.ogre3d.org
*****************************************************************************
				This PlugIn uses the following resources:

Ogre - see above
Ogg / Vorbis / Theora www.xiph.org
C++ Portable Types Library (PTypes - http://www.melikyan.com/ptypes/ )

*****************************************************************************
Copyright © 2008 Kresimir Spes (kreso@cateia.com)
          © 2000-2004 pjcast@yahoo.com

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
***************************************************************************/

#ifndef _TheoraVideoTextureController_H
#define _TheoraVideoTextureController_H

#include "OgreString.h"
#include "OgreExternalTextureSource.h"
#include "OgreExternalTextureSourceManager.h"
#include "OgreFrameListener.h"
#include "TheoraPlayerPreReqs.h"
#include "TheoraExport.h"

namespace Ogre
{
	/**
		A frame listener used by the plugin, it just updates video clips so you don't have to :)
	*/
	class TheoraVideoFrameListener : public FrameListener
	{
	public:
		bool frameStarted(const FrameEvent& evt);
	};







	/**
		Handles "ogg_video" external texture sources from material serializer.
		It is recomended that you also use this class when creating
		textures in code, but it is not required.
	*/
	class _OgreTheoraExport TheoraVideoManager : public ExternalTextureSource, public FrameListener
	{
	public:
		TheoraVideoManager();
		~TheoraVideoManager();

		/**
			@remarks
				Creates a texture into an already defined material
				All setting should have been set before calling this.
				Mainly, movie name and play mode. Refer to base 
				class ( ExternalTextureSource ) for details
			@param sMaterialName
				Material you are attaching a movie to.
		*/
		void createDefinedTexture( const String& sMaterialName,
			const String& groupName = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
		
		/**
			@remarks
				Destroys Video Texture. Currently does not destroy material/texture
			@param sMaterialName
				Material Name you are looking to remove movie form
		*/
		void destroyAdvancedTexture( const String& sMaterialName,
			const String& groupName = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
		
		/**
			@remarks 
				Returns the clip that is holding the Video Material
			@param sMovieName
				Search using movie file name
		*/
		TheoraVideoClip* getMovieNameClip( String sMovieName );

		/**
			@remarks 
				Returns the clip that is holding the Video Material
			@param sMovieName
				Search using material name
		*/
		TheoraVideoClip* getMaterialNameClip( String sMaterialName );
		
		/**
			@remarks
				This function is called to init this plugin - do not call directly
		*/
		bool initialise();
		
		/**
			@remarks
				Called to shut down this plugIn - called from manager class
		*/
		void shutDown();


		/**
			@remarks
				Called to set the Texture FX mode on the next generated texture
		*/
		void setRenderFx( TextureSpecialRenderFX rfx ) { tempTextureFX = rfx; }

		/**
			@remarks
				Called to set if seeking is enabled on the next generated video
		*/
		void setSeekEnabled( bool bEnabled ) { mSeekEnabled = bEnabled; }
		
		void setAutoAudioUpdate( bool autoUpdateAudio ) {mAutoUpdate = autoUpdateAudio;}
        






		/**
			@remarks
				Sets the number of precached frames used in the video
		*/
		class CmdNumPrecachedFrames : public ParamCommand
        {
        public:
			String doGet(const void* target) const;
            void doSet(void* target, const String& val);
			void setThis( TheoraVideoManager* p ) { pThis = p; }
		private:
			TheoraVideoManager* pThis;
        };


		typedef std::vector< TheoraVideoClip* > mtClips;
		//! A list of movie clips
		mtClips mMoviesList;
		// param set by CmdNumPrecachedFrames class
		int mNumPrecachedFrames;

	protected:
		static CmdNumPrecachedFrames msCmdNumPrecachedFrames;

		TextureSpecialRenderFX tempTextureFX;
		bool mSeekEnabled;
		bool mAutoUpdate;
		
		//! A flag indicating whether init has been called 
		bool mbInit;
	};
}
#endif


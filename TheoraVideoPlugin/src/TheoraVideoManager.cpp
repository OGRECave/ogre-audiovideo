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
#include "TheoraVideoManager.h"
#include "TheoraVideoClip.h"

#include "OgreRoot.h"
#include "OgreException.h"
#include "OgreLogManager.h"


namespace Ogre
{

	bool TheoraVideoFrameListener::frameStarted(const FrameEvent& evt)
	{
		TheoraVideoManager* c = (TheoraVideoManager*)
			ExternalTextureSourceManager::getSingleton().getExternalTextureSource("ogg_video");
		for (TheoraVideoManager::mtClips::iterator it=c->mMoviesList.begin();it!=c->mMoviesList.end();it++)
		{
			(*it)->blitFrameCheck();
		}
		return true;
	}




	//Initial static command param method
	TheoraVideoManager::CmdRenderFx TheoraVideoManager::msCmdRenderFx;
	TheoraVideoManager::CmdSeekingEnabled TheoraVideoManager::msCmdSeekingEnabled;
	//----------------------------------------------------------------------------//
	TheoraVideoManager::TheoraVideoManager() : 
		mbInit( false )
	{
		mPlugInName = "TheoraVideoPlugIn";
		mDictionaryName = mPlugInName;
		tempTextureFX = render_normal;
		mSeekEnabled = false;
		mAutoUpdate = false;
	}

	//----------------------------------------------------------------------------//
	TheoraVideoManager::~TheoraVideoManager()
	{
		if(mbInit)
			shutDown();
	}

	//----------------------------------------------------------------------------//
	void TheoraVideoManager::createDefinedTexture( 
		const String& sMaterialName, const String& groupName )
	{
		TheoraVideoClip* newMovie = 0;
		bool bSound = false;

		//Give the possibility of sound... Later we really check for sound
		if( mMode == TextureEffectPause )
			bSound = true;

		newMovie = new TheoraVideoClip();
		
		try 
		{
			newMovie->load( 
				mInputFileName, sMaterialName, groupName, mTechniqueLevel,
				mPassLevel, mStateLevel, bSound, mMode, mSeekEnabled,
				mAutoUpdate );

			mMoviesList.push_back( newMovie );
		}
		catch(...)
		{
			delete newMovie;
		}

		tempTextureFX = render_normal;
		mInputFileName = "None";
		mTechniqueLevel = mPassLevel = mStateLevel = 0;
		mSeekEnabled = false;
		mAutoUpdate = false;
	}

	//----------------------------------------------------------------------------//
	void TheoraVideoManager::destroyAdvancedTexture( 
		const String& sMaterialName, const String& groupName )
	{
		mtClips::iterator i;
		for(i = mMoviesList.begin(); i != mMoviesList.end(); ++i )
		{
			//Perhaps we were sent either movie file name, or material name
			if( (*i)->getMaterialName() == sMaterialName ||
				(*i)->getMovieName() == sMaterialName )
			{
				delete (*i);
				mMoviesList.erase( i );
				return;
			}
		}
		
		LogManager::getSingleton().logMessage( 
			"**Warning** ::>> TheoraVideoManager::DestroyVideoTexture Tried to delete Movie Texture " 
			+ sMaterialName + ". Though, Texture was not anywhere to be found :< " );
	}

	//----------------------------------------------------------------------------//
	TheoraVideoClip* TheoraVideoManager::getMovieNameClip( String sMovieName )
	{
		//Search for an entry that has the searched for movie name
		mtClips::iterator i;
		for( i = mMoviesList.begin(); i != mMoviesList.end(); ++i )
		{
			if( (*i)->getMovieName() == sMovieName )
				return (*i);
		}

		LogManager::getSingleton().logMessage( 
			"**Warning** ::>> TheoraVideoManager::getMovieNameClip Tried to find Movie Texture " 
			+ sMovieName + ". Though, Texture was not anywhere to be found :< " );
		return 0;
	}

	//----------------------------------------------------------------------------//
	TheoraVideoClip* TheoraVideoManager::getMaterialNameClip( String sMaterialName )
	{
		//Search for an entry that has the searched for material name
		mtClips::iterator i;
		for( i = mMoviesList.begin(); i != mMoviesList.end(); ++i )
		{
			if( (*i)->getMaterialName() == sMaterialName )
				return (*i);
		}

		LogManager::getSingleton().logMessage( 
			"**Warning** ::>> TheoraVideoManager::getMovieClip Tried to find Movie Texture " 
			+ sMaterialName + ". Though, Texture was not anywhere to be found :< " );
		return 0;
	}

	//----------------------------------------------------------------------------//
	bool TheoraVideoManager::initialise( )
	{
		if( mbInit )
			return true;

		//Ensure base dictionary is setup
		addBaseParams();
	    
		ParamDictionary* dict = getParamDictionary();
		
		//Add render_fx method
		msCmdRenderFx.setThis( this );
		dict->addParameter(ParameterDef("render_fx", 
			"Defines where/how this movie is decoded to"
			, PT_STRING),
			&TheoraVideoManager::msCmdRenderFx);

		msCmdSeekingEnabled.setThis( this );
		dict->addParameter(ParameterDef("set_seeking", 
			"Sets wether or not seeking is enabled (true/false) Defaults to false"
			, PT_STRING),
			&TheoraVideoManager::msCmdSeekingEnabled);

		mbInit = true;
		return true;
	}

	//----------------------------------------------------------------------------//
	void TheoraVideoManager::shutDown()
	{
		//Destroy all movie clips
		mtClips::iterator i;
		for(i = mMoviesList.begin(); i != mMoviesList.end(); ++i )
			delete (*i);
		mMoviesList.clear();
		
		mbInit = false;
	}

	//----------------------------------------------------------------------------//
	String TheoraVideoManager::CmdRenderFx::doGet(const void* target) const
	{
		return "NA";
	}

	//----------------------------------------------------------------------------//
    void TheoraVideoManager::CmdRenderFx::doSet(void* target, const String& val)
	{
		if( val == "render_to_alpha" )
			static_cast<TheoraVideoManager*>(target)->setRenderFx( render_to_alpha );
		else if( val == "render_to_PF_B8G8R8A8" )
			static_cast<TheoraVideoManager*>(target)->setRenderFx( render_to_PF_B8G8R8A8 );
		else
			static_cast<TheoraVideoManager*>(target)->setRenderFx( render_normal );
	}
	
	//----------------------------------------------------------------------------//
	String TheoraVideoManager::CmdSeekingEnabled::doGet(const void* target) const
	{
		return "NA";
	}

	//----------------------------------------------------------------------------//
    void TheoraVideoManager::CmdSeekingEnabled::doSet(void* target, const String& val)
	{
		if( val == "true" )
			static_cast<TheoraVideoManager*>(target)->setSeekEnabled( true );
		else
			static_cast<TheoraVideoManager*>(target)->setSeekEnabled( false );
	}
} //end namespace Ogre



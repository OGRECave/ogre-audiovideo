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
#include "TheoraVideoController.h"
#include "TheoraVideoClip.h"

#include "OgreRoot.h"
#include "OgreException.h"
#include "OgreLogManager.h"


namespace Ogre
{

	bool TheoraVideoFrameListener::frameStarted(const FrameEvent& evt)
	{
		TheoraVideoController* c = (TheoraVideoController*)
			ExternalTextureSourceManager::getSingleton().getExternalTextureSource("ogg_video");
		for (TheoraVideoController::mtClips::iterator it=c->mMoviesList.begin();it!=c->mMoviesList.end();it++)
		{
			(*it)->blitFrameCheck();
		}
		return true;
	}




	//Initial static command param method
	TheoraVideoController::CmdRenderFx TheoraVideoController::msCmdRenderFx;
	TheoraVideoController::CmdSeekingEnabled TheoraVideoController::msCmdSeekingEnabled;
	//----------------------------------------------------------------------------//
	TheoraVideoController::TheoraVideoController() : 
		mbInit( false )
	{
		mPlugInName = "TheoraVideoPlugIn";
		mDictionaryName = mPlugInName;
		tempTextureFX = render_normal;
		mSeekEnabled = false;
		mAutoUpdate = false;
	}

	//----------------------------------------------------------------------------//
	TheoraVideoController::~TheoraVideoController()
	{
		if(mbInit)
			shutDown();
	}

	//----------------------------------------------------------------------------//
	void TheoraVideoController::createDefinedTexture( 
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
			newMovie->createMovieClip( 
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
	void TheoraVideoController::destroyAdvancedTexture( 
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
			"**Warning** ::>> TheoraVideoController::DestroyVideoTexture Tried to delete Movie Texture " 
			+ sMaterialName + ". Though, Texture was not anywhere to be found :< " );
	}

	//----------------------------------------------------------------------------//
	TheoraVideoClip* TheoraVideoController::getMovieNameClip( String sMovieName )
	{
		//Search for an entry that has the searched for movie name
		mtClips::iterator i;
		for( i = mMoviesList.begin(); i != mMoviesList.end(); ++i )
		{
			if( (*i)->getMovieName() == sMovieName )
				return (*i);
		}

		LogManager::getSingleton().logMessage( 
			"**Warning** ::>> TheoraVideoController::getMovieNameClip Tried to find Movie Texture " 
			+ sMovieName + ". Though, Texture was not anywhere to be found :< " );
		return 0;
	}

	//----------------------------------------------------------------------------//
	TheoraVideoClip* TheoraVideoController::getMaterialNameClip( String sMaterialName )
	{
		//Search for an entry that has the searched for material name
		mtClips::iterator i;
		for( i = mMoviesList.begin(); i != mMoviesList.end(); ++i )
		{
			if( (*i)->getMaterialName() == sMaterialName )
				return (*i);
		}

		LogManager::getSingleton().logMessage( 
			"**Warning** ::>> TheoraVideoController::getMovieClip Tried to find Movie Texture " 
			+ sMaterialName + ". Though, Texture was not anywhere to be found :< " );
		return 0;
	}

	//----------------------------------------------------------------------------//
	bool TheoraVideoController::initialise( )
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
			&TheoraVideoController::msCmdRenderFx);

		msCmdSeekingEnabled.setThis( this );
		dict->addParameter(ParameterDef("set_seeking", 
			"Sets wether or not seeking is enabled (true/false) Defaults to false"
			, PT_STRING),
			&TheoraVideoController::msCmdSeekingEnabled);

		mbInit = true;
		return true;
	}

	//----------------------------------------------------------------------------//
	void TheoraVideoController::shutDown()
	{
		//Destroy all movie clips
		mtClips::iterator i;
		for(i = mMoviesList.begin(); i != mMoviesList.end(); ++i )
			delete (*i);
		mMoviesList.clear();
		
		mbInit = false;
	}

	//----------------------------------------------------------------------------//
	String TheoraVideoController::CmdRenderFx::doGet(const void* target) const
	{
		return "NA";
	}

	//----------------------------------------------------------------------------//
    void TheoraVideoController::CmdRenderFx::doSet(void* target, const String& val)
	{
		if( val == "render_to_alpha" )
			static_cast<TheoraVideoController*>(target)->setRenderFx( render_to_alpha );
		else if( val == "render_to_PF_B8G8R8A8" )
			static_cast<TheoraVideoController*>(target)->setRenderFx( render_to_PF_B8G8R8A8 );
		else
			static_cast<TheoraVideoController*>(target)->setRenderFx( render_normal );
	}
	
	//----------------------------------------------------------------------------//
	String TheoraVideoController::CmdSeekingEnabled::doGet(const void* target) const
	{
		return "NA";
	}

	//----------------------------------------------------------------------------//
    void TheoraVideoController::CmdSeekingEnabled::doSet(void* target, const String& val)
	{
		if( val == "true" )
			static_cast<TheoraVideoController*>(target)->setSeekEnabled( true );
		else
			static_cast<TheoraVideoController*>(target)->setSeekEnabled( false );
	}
} //end namespace Ogre



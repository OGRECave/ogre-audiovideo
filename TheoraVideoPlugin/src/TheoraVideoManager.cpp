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
#include "OgreStringConverter.h"


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
	TheoraVideoManager::CmdNumPrecachedFrames TheoraVideoManager::msCmdNumPrecachedFrames;
	//----------------------------------------------------------------------------//
	TheoraVideoManager::TheoraVideoManager() : 
		mbInit( false ),
		mNumPrecachedFrames(-1)
	{
		mPlugInName = "TheoraVideoPlugin";
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
	void TheoraVideoManager::createDefinedTexture(const String& sMaterialName,const String& groupName)
	{
		TheoraVideoClip* newMovie = 0;
		bool bSound = false;

		//Give the possibility of sound... Later we really check for sound
		if( mMode == TextureEffectPause )
			bSound = true;

		newMovie = new TheoraVideoClip();
		
		try 
		{
			newMovie->load(mInputFileName, sMaterialName, groupName, mTechniqueLevel,
						  mPassLevel, mStateLevel, bSound, mMode, mSeekEnabled, mAutoUpdate );

			int n=(mNumPrecachedFrames == -1) ? 16 : mNumPrecachedFrames;
			newMovie->setNumPrecachedFrames(n);
			mMoviesList.push_back( newMovie );
		}
		catch(...)
		{
			delete newMovie;
		}
		mNumPrecachedFrames=-1;
		tempTextureFX = render_normal;
		mInputFileName = "None";
		mTechniqueLevel = mPassLevel = mStateLevel = 0;
		mSeekEnabled = false;
		mAutoUpdate = false;
	}

	//----------------------------------------------------------------------------//
	void TheoraVideoManager::destroyAdvancedTexture(const String& sMaterialName,const String& groupName)
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
	TheoraVideoClip* TheoraVideoManager::getMovieNameClip(String sMovieName)
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
		msCmdNumPrecachedFrames.setThis( this );
		dict->addParameter(ParameterDef("precache", 
			"Defines how many frames should be precached to smooth video playback"
			, PT_INT),
			&TheoraVideoManager::msCmdNumPrecachedFrames);

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
	String TheoraVideoManager::CmdNumPrecachedFrames::doGet(const void* target) const
	{
		return "NA";
	}

	//----------------------------------------------------------------------------//
    void TheoraVideoManager::CmdNumPrecachedFrames::doSet(void* target, const String& val)
	{
		static_cast<TheoraVideoManager*>(target)->mNumPrecachedFrames=StringConverter::parseInt(val);
	}
} //end namespace Ogre



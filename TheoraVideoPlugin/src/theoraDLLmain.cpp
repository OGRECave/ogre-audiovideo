/*
-----------------------------------------------------------------------------
This source file is part of the ffmpegVideoSystem ExternalTextureSource PlugIn 
for OGRE (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

*****************************************************************************
				This PlugIn uses the following resources:

Ogre - see above
Ogg / Vorbis / Theora www.xiph.org
C++ Portable Types Library (PTypes - http://www.melikyan.com/ptypes/ )

*****************************************************************************

Copyright © 2000-2004 The OGRE Team
Also see acknowledgements in Readme.html

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
-----------------------------------------------------------------------------
/***************************************************************************
theoraDLLmain.cpp  -  
	called from Ogre dynload. Creates the PlugIn and registers it.

-------------------
date                 : Jan 1 2004
email                : pjcast@yahoo.com
***************************************************************************/
#include "OgreExternalTextureSourceManager.h"
#include "TheoraVideoController.h"

#include "TheoraVideoDriver.h"

namespace Ogre
{
	//Pointer Used to register with ExternalTextureSourceManager
	ExternalTextureSource* theoraVideoPlugin;

	//Called from Ogre's dynload when loading plugins
	extern "C" void dllStartPlugin( void )
	{
		// Create our new External Textue Source PlugIn
		theoraVideoPlugin = new TheoraVideoController();

		//Create YUV Lookup tables here
		TheoraVideoDriver::createCoefTables();

		// Register with Manger
		ExternalTextureSourceManager::getSingleton().setExternalTextureSource( "ogg_video", theoraVideoPlugin );
	}

	//Called when unloading plugins
	extern "C" void dllStopPlugin( void )
	{
		//Just delete the system :P
		delete theoraVideoPlugin;
	}
}


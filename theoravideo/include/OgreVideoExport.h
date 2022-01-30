/************************************************************************************
This source file is part of the Ogre3D Theora Video Plugin
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef _OgreVideoExport_H
#define _OgreVideoExport_H

#include <OgrePrerequisites.h>

//-----------------------------------------------------------------------
// Windows Settings
//-----------------------------------------------------------------------

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 
#   ifdef THEORAVIDEO_PLUGIN_EXPORTS 
#       define _OgreTheoraExport __declspec(dllexport) 
#   else 
#       define _OgreTheoraExport __declspec(dllimport) 
#   endif 
#else 
#   define _OgreTheoraExport 
#endif 

#endif


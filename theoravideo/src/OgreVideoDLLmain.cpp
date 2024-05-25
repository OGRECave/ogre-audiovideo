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
#include <stdio.h>

namespace Ogre
{
#ifndef OGRE_STATIC_LIB
    static OgreVideoPlugin* theoraVideoPlugin;

	extern "C" void _OgreTheoraExport dllStartPlugin()
	{
	    theoraVideoPlugin = new OgreVideoPlugin();
		#if AV_OGRE_NEXT_VERSION > 0x30000
	    Root::getSingleton().installPlugin(theoraVideoPlugin, nullptr);
		#endif
	}

	extern "C" void _OgreTheoraExport dllStopPlugin()
	{
	    Root::getSingleton().uninstallPlugin(theoraVideoPlugin);
		delete theoraVideoPlugin;
	}
#endif
}

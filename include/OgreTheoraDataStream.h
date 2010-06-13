/************************************************************************************
This source file is part of the Ogre3D Theora Video Plugin
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/

#ifndef _OgreTheoraDataStream_h
#define _OgreTheoraDataStream_h

#include "TheoraDataSource.h"
#ifndef OGRE_MAC_FRAMEWORK
#include "OgreDataStream.h"
#else
#include <Ogre/OgreDataStream.h>
#endif

namespace Ogre
{

	class OgreTheoraDataStream : public TheoraDataSource
	{
		std::string mName;
		DataStreamPtr mStream;
	public:
		OgreTheoraDataStream(std::string filename,std::string group_name);
		~OgreTheoraDataStream();

		int read(void* output,int nBytes);
		void seek(unsigned long byte_index);
		std::string repr();
		unsigned long size();
		unsigned long tell();
	};
}

#endif


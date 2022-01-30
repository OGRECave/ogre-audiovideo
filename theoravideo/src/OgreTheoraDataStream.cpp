/************************************************************************************
This source file is part of the Ogre3D Theora Video Plugin
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#include <OgreRoot.h>
#include "OgreTheoraDataStream.h"

namespace Ogre
{
	OgreTheoraDataStream::OgreTheoraDataStream(std::string filename,std::string group_name)
	{
		mName=filename;
		mStream = ResourceGroupManager::getSingleton().openResource(filename,group_name);
	}

	OgreTheoraDataStream::~OgreTheoraDataStream()
	{
		if (!(mStream))
		{
			mStream->close();
			mStream.reset();
		}
	}

	int OgreTheoraDataStream::read(void* output,int nBytes)
	{
		return mStream->read( output,nBytes); 
	}

	void OgreTheoraDataStream::seek(unsigned long byte_index)
	{
		mStream->seek(byte_index);
	}

	std::string OgreTheoraDataStream::repr()
	{
		return mName;
	}

	unsigned long OgreTheoraDataStream::size()
	{
		return mStream->size();
	}

	unsigned long OgreTheoraDataStream::tell()
	{
		return mStream->tell();
	}

} // end namespace Ogre

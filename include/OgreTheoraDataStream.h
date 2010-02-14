/************************************************************************************
This source file is part of the Ogre3D Theora Video Plugin
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)

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


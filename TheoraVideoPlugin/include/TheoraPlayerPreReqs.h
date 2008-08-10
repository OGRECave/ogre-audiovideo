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
#ifndef _TheoraPlayerPreReqsHeader_
#define _TheoraPlayerPreReqsHeader_

namespace Ogre
{
	enum TextureSpecialRenderFX
	{
		render_normal = 0,		//Default mode - decodes texture normally
		render_to_alpha,		//Uses Y component and renders to alpha channel only
		render_to_PF_B8G8R8A8	//Same as default, though, has a full alpha texture
	};

	//Forward declare our classes
	class TheoraMovieClip;
	class TheoraAudioDriver;
	class TheoraGenericAudio;
	class TheoraVideoDriver;
	class TheoraVideoController;
}

#endif //_TheoraPlayerPreReqsHeader_



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
#include "TheoraVideoDriver.h"

#include "OgreTimer.h"
#include "OgreTextureManager.h"
#include "OgreMaterialManager.h"
#include "OgreTechnique.h"
#include "OgreHardwarePixelBuffer.h"

//Defines
#define MAX( a, b ) ((a > b) ? a : b)
#define MIN( a, b ) ((a < b) ? a : b)
#define CLIP_RGB_COLOR( rgb_color_test ) \
	MAX( MIN(rgb_color_test, 255), 0 )


namespace Ogre
{
	//Handle static member vars
	unsigned int TheoraVideoDriver::YTable[ 256 ];
	unsigned int TheoraVideoDriver::BUTable[ 256 ];
	unsigned int TheoraVideoDriver::GUTable[ 256 ];
	unsigned int TheoraVideoDriver::GVTable[ 256 ];
	unsigned int TheoraVideoDriver::RVTable[ 256 ];
	
	//------------------------------------------------------------------//
	TheoraVideoDriver::TheoraVideoDriver() :
		mTexture(0),
		mWidth(0),
		mHeight(0)
	{
	}

	//------------------------------------------------------------------//
	TheoraVideoDriver::~TheoraVideoDriver()
	{
		// Grab Our material, then remove the Texture Unit
		MaterialPtr material = MaterialManager::getSingleton().getByName( mMaterialName );
		
		if( !(material.isNull()) )
			material->getTechnique(mTec)->getPass(mPass)->removeTextureUnitState( mUnit );

		//Remove Texture Resource
		mTexture.setNull();
		TextureManager::getSingleton().unload( mTextureName );
		TextureManager::getSingleton().remove( mTextureName );
	}
		
	//------------------------------------------------------------------//
	void TheoraVideoDriver::attachVideoToTextureUnit( 
		const String &sMaterialName, const String &sTextureName, 
		const String &sGroupName, int TechniqueLevel, int PassLevel,
		int TextureUnitStateLevel, int width, int height)
	{
		//Store local copies of sent info
		mTec = TechniqueLevel;
		mPass= PassLevel;
		mUnit= TextureUnitStateLevel;
		mTextureName = sTextureName;
		mMaterialName= sMaterialName;

		mWidth = width;
		mHeight = height;

		// create texture
		mTexture = TextureManager::getSingleton().createManual(sTextureName,sGroupName,TEX_TYPE_2D,
			mWidth,mHeight,1,0,PF_X8R8G8B8,TU_DYNAMIC_WRITE_ONLY);
		// clear to black

		PixelFormat pf=mTexture->getFormat();
		int w=mTexture->getWidth();
		unsigned char* texData=(unsigned char*) mTexture->getBuffer()->lock(HardwareBuffer::HBL_DISCARD);
		memset(texData,0,mWidth*mHeight*4);
		mTexture->getBuffer()->unlock();

		// Grab Our material, then find the Texture Unit
		MaterialPtr material = MaterialManager::getSingleton().getByName( sMaterialName );
		TextureUnitState* t = material->getTechnique(mTec)->getPass(mPass)->getTextureUnitState(mUnit);

		//Now, attach the texture to the material texture unit (single layer) and setup properties
		t->setTextureName(sTextureName,TEX_TYPE_2D);
		t->setTextureFiltering(FO_LINEAR, FO_LINEAR, FO_NONE);
		t->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
	}

	//----------------------------------------------------------------------//
	void TheoraVideoDriver::renderToTexture(unsigned char* buffer)
	{
		unsigned char* texData=(unsigned char*) mTexture->getBuffer()->lock(HardwareBuffer::HBL_DISCARD);
		memcpy(texData,buffer,mWidth*mHeight*4);
		mTexture->getBuffer()->unlock();
	}

	//----------------------------------------------------------------------//
	void TheoraVideoDriver::decodeYtoTexture(yuv_buffer *yuv,unsigned char* xrgb_out)
	{
		unsigned char*ySrc=yuv->y,*ySrc2=yuv->y,*ySrcEnd;
		unsigned int cy,*out=(unsigned int*) xrgb_out;
		int y;

		for (y=0;y<yuv->y_height;y++)
		{
			ySrc=ySrc2; ySrcEnd=ySrc2+yuv->y_width;
			while (ySrc != ySrcEnd)
			{
				cy=*ySrc;
				*out=(((cy << 8) | cy) << 8) | cy;
				out++;
				ySrc++;
			}
			ySrc2+=yuv->y_stride;
		}
	}

	//----------------------------------------------------------------------//
	void TheoraVideoDriver::decodeYUVtoTexture(yuv_buffer *yuv,unsigned char* xrgb_out)
	{
		int t,y;
		unsigned char *ySrc=yuv->y, *ySrc2=yuv->y,*ySrcEnd,
		              *uSrc=yuv->u, *uSrc2=yuv->u,
		              *vSrc=yuv->v, *vSrc2=yuv->v;
		unsigned int *out=(unsigned int*) xrgb_out;

		for (y=0;y<yuv->y_height;y++)
		{
			t=0; ySrc=ySrc2; ySrcEnd=ySrc2+yuv->y_width; uSrc=uSrc2; vSrc=vSrc2;
			while (ySrc != ySrcEnd)
			{
				*out=(((*ySrc << 8) | *uSrc) << 8) | *vSrc;
				out++;
				ySrc++;
				if (t=!t == 1) { uSrc++; vSrc++; }
			}
			ySrc2+=yuv->y_stride;
			if (y%2 == 1) { uSrc2+=yuv->uv_stride; vSrc2+=yuv->uv_stride; }
		}
	}

	//----------------------------------------------------------------------//
	void TheoraVideoDriver::decodeRGBtoTexture(yuv_buffer *yuv,unsigned char* xrgb_out)
	{
		int t,y;
		unsigned char *ySrc=yuv->y, *ySrc2=yuv->y,*ySrcEnd,
		              *uSrc=yuv->u, *uSrc2=yuv->u,
		              *vSrc=yuv->v, *vSrc2=yuv->v;
		unsigned int *out=(unsigned int*) xrgb_out;
		int r, g, b, cu, cv, bU, gUV, rV, rgbY;

		for (y=0;y<yuv->y_height;y++)
		{
			t=0; ySrc=ySrc2; ySrcEnd=ySrc2+yuv->y_width; uSrc=uSrc2; vSrc=vSrc2;
			while (ySrc != ySrcEnd)
			{
				//get corresponding lookup values
				rgbY = YTable[*ySrc];
				if (t=!t == 1)
				{
					cu=*uSrc; cv=*vSrc;
					rV   = RVTable[cv];
					gUV  = GUTable[cu] + GVTable[cv];
					bU   = BUTable[cu];
					uSrc++; vSrc++;
				}
				//scale down - brings are values back into the 8 bits of a byte
				r = CLIP_RGB_COLOR((rgbY + rV ) >> 13);
				g = CLIP_RGB_COLOR((rgbY - gUV) >> 13);
				b = CLIP_RGB_COLOR((rgbY + bU ) >> 13);
				*out=(((r << 8) | g) << 8) | b;
				out++;
				ySrc++;
			}
			ySrc2+=yuv->y_stride;
			if (y%2 == 1) { uSrc2+=yuv->uv_stride; vSrc2+=yuv->uv_stride; }
		}
	}

	//----------------------------------------------------------------------//
	void TheoraVideoDriver::createCoefTables()
	{
		//used to bring the table into the high side (scale up) so we
		//can maintain high precision and not use floats (FIXED POINT)
		int scale = 1L << 13,
			temp;
		
		for ( unsigned int i = 0; i < 256; i++ )
		{
			temp = i - 128;
			
			YTable[i]  = (unsigned int)((1.164 * scale + 0.5) * (i - 16));	//Calc Y component
			
			RVTable[i] = (unsigned int)((1.596 * scale + 0.5) * temp);		//Calc R component
			
			GUTable[i] = (unsigned int)((0.391 * scale + 0.5) * temp);		//Calc G u & v components
			GVTable[i] = (unsigned int)((0.813 * scale + 0.5) * temp);
			
			BUTable[i] = (unsigned int)((2.018 * scale + 0.5) * temp);		//Calc B component
		}
	}
} //end Namespace

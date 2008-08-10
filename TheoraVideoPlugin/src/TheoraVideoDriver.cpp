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

//int rgb_color_test, unsigned char rgb_char_buffer - can never be negative
//#define CLIP_RGB_COLOR( rgb_color_test, rgb_char_buffer ) \
//	if( rgb_color_test > 255 )							  \
//		rgb_char_buffer = 255;							  \
//	else if( rgb_color_test >= 0 )						  \
//		rgb_char_buffer = rgb_color_test;				  \
//	else												  \
//		rgb_char_buffer = 0

#define CLIP_RGB_COLOR( rgb_color_test, rgb_char_buffer ) \
	rgb_char_buffer = MAX( MIN(rgb_color_test, 255), 0 )


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
		int x,y;

		unsigned char* ySrc=yuv->y;
		unsigned char* ySrc2=yuv->y;

		for (y=0;y<yuv->y_height;y++)
		{
			ySrc=ySrc2;
			for (x=0;x<yuv->y_width;x++)
			{
				xrgb_out[0]=xrgb_out[1]=xrgb_out[2]=*ySrc;
				xrgb_out[3]=255;
				xrgb_out+=4;
				ySrc++;
			}
			ySrc2+=yuv->y_stride;
		}
	}

	//----------------------------------------------------------------------//
	void TheoraVideoDriver::decodeYUVtoTexture(yuv_buffer *yuv,unsigned char* xrgb_out)
	{
		//Convert 4:2:0 YUV YCrCb to an X8R8G8B8 Bitmap
/*
		unsigned char *y=yuv->y, *yLineEnd=yuv->y+yuv->y_width,*yStrideEnd=yuv->y+yuv->y_stride;
		unsigned char *u=yuv->u;
		unsigned char *v=yuv->v, cy,cu,cv;
		int h=0,x_inc=0,y_inc=0;

		while (h < yuv->y_height)
		{
			while (y < yLineEnd)
			{
				//R = Y + 1.140V
				//G = Y - 0.395U - 0.581V
				//B = Y + 2.032U
				cy=*y; cu=*u; cv=*v;

				xrgb_out[0]=cy+2.032*cu;
				xrgb_out[1]=cy-0.395f*cu - 0.581f*cv;
				xrgb_out[2]=cy+1.14f*cv;
				xrgb_out[3]=255;
				xrgb_out+=4;
				y++;
				x_inc=!x_inc;
				if (x_inc == 0) { u++; v++; }
			}
			y=yStrideEnd;			yStrideEnd+=yuv->y_stride;			yLineEnd=y+yuv->y_width;
			
			y_inc=!y_inc;
			u-=yuv->uv_width; v-=yuv->uv_width;
			if (y_inc == 0) { u+=yuv->uv_stride; v+=yuv->uv_stride; }


			h++;
		}
*/
		//convenient pointers

		mBytesPerPixel=4; // temp hack

		unsigned char *dstBitmap=xrgb_out;
		unsigned char *dstBitmapOffset = xrgb_out + (mBytesPerPixel * mWidth);

		unsigned char *ySrc = (unsigned char*)yuv->y,
					  *uSrc = (unsigned char*)yuv->u,
					  *vSrc = (unsigned char*)yuv->v,
					  *ySrc2 = ySrc + yuv->y_stride;
		
		//Calculate buffer offsets
		unsigned int dstOff = mWidth * mBytesPerPixel;//( mWidth*6 ) - ( yuv->y_width*3 );
		int yOff = (yuv->y_stride * 2) - yuv->y_width;
			
		
		//Check if upside down, if so, reverse buffers and offsets
		if ( yuv->y_height < 0 )
		{
			yuv->y_height = -yuv->y_height;
			ySrc		 += (yuv->y_height - 1) * yuv->y_stride;
			
			uSrc += ((yuv->y_height / 2) - 1) * yuv->uv_stride;
			vSrc += ((yuv->y_height / 2) - 1) * yuv->uv_stride;
			
			ySrc2 = ySrc - yuv->y_stride;
			yOff  = -yuv->y_width - ( yuv->y_stride * 2 );
			
			yuv->uv_stride = -yuv->uv_stride;
		}

		//Cut width and height in half (uv field is only half y field)
		yuv->y_height = yuv->y_height >> 1;
		yuv->y_width = yuv->y_width >> 1;

		//Convientient temp vars
		signed int r, g, b, u, v, bU, gUV, rV, rgbY;
		int x;
		
		//Loop does four blocks per iteration (2 rows, 2 pixels at a time)
		for (int y = yuv->y_height; y > 0; --y)
		{
			for (x = 0; x < yuv->y_width; ++x) 
			{
				//Get uv pointers for row
				u = uSrc[x]; 
				v = vSrc[x];
				
				//get corresponding lookup values
				rgbY= YTable[*ySrc];				
				rV  = RVTable[v];
				gUV = GUTable[u] + GVTable[v];
				bU  = BUTable[u];
				++ySrc;

				//scale down - brings are values back into the 8 bits of a byte
				r = (rgbY + rV ) >> 13;
				g = (rgbY - gUV) >> 13;
				b = (rgbY + bU ) >> 13;
				
				//Clip to RGB values (255 0)
				CLIP_RGB_COLOR( r, dstBitmap[2] );
				CLIP_RGB_COLOR( g, dstBitmap[1] );
				CLIP_RGB_COLOR( b, dstBitmap[0] );
				
				//And repeat for other pixels (note, y is unique for each
				//pixel, while uv are not)
				rgbY = YTable[*ySrc];
				r = (rgbY + rV)  >> 13;
				g = (rgbY - gUV) >> 13;
				b = (rgbY + bU)  >> 13;
				CLIP_RGB_COLOR( r, dstBitmap[mBytesPerPixel+2] );
				CLIP_RGB_COLOR( g, dstBitmap[mBytesPerPixel+1] );
				CLIP_RGB_COLOR( b, dstBitmap[mBytesPerPixel+0] );
				++ySrc;

				rgbY = YTable[*ySrc2];
				r = (rgbY + rV)  >> 13;
				g = (rgbY - gUV) >> 13;
				b = (rgbY + bU)  >> 13;
				CLIP_RGB_COLOR( r, dstBitmapOffset[2] );
				CLIP_RGB_COLOR( g, dstBitmapOffset[1] );
				CLIP_RGB_COLOR( b, dstBitmapOffset[0] );
				++ySrc2;
				
				rgbY = YTable[*ySrc2];
				r = (rgbY + rV)  >> 13;
				g = (rgbY - gUV) >> 13;
				b = (rgbY + bU)  >> 13;
				CLIP_RGB_COLOR( r, dstBitmapOffset[mBytesPerPixel+2] );
				CLIP_RGB_COLOR( g, dstBitmapOffset[mBytesPerPixel+1] );
				CLIP_RGB_COLOR( b, dstBitmapOffset[mBytesPerPixel+0] );
				++ySrc2;

				//Advance inner loop offsets
				dstBitmap += mBytesPerPixel << 1;
				dstBitmapOffset += mBytesPerPixel << 1;
			} // end for x

			//Advance destination pointers by offsets
			dstBitmap		+= dstOff;
			dstBitmapOffset += dstOff;
			ySrc			+= yOff;
			ySrc2			+= yOff;
			uSrc			+= yuv->uv_stride;
			vSrc			+= yuv->uv_stride;
		} //end for y

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

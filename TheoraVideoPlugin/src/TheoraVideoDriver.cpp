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
Copyright © 2000-2004 pjcast@yahoo.com

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
		m_RGBBitmap(0),
		mTexture(0),
		m_Width(0),
		m_Height(0),
		mYUVConvertTime(0),
		mBlitTime(0)
	{
	}

	//------------------------------------------------------------------//
	TheoraVideoDriver::~TheoraVideoDriver()
	{
		// Grab Our material, then remove the Texture Unit
		MaterialPtr material = MaterialManager::getSingleton().getByName( mMaterialName );
		
		if( !(material.isNull()) )
			material->getTechnique(m_Tec)->getPass(m_Pass)->removeTextureUnitState( m_Unit );

		//Remove Texture Resource
		mTexture.setNull();
		TextureManager::getSingleton().unload( mTextureName );
		TextureManager::getSingleton().remove( mTextureName );
				
		delete [] m_RGBBitmap;
		m_RGBBitmap = 0;
	}
		
	//------------------------------------------------------------------//
	void TheoraVideoDriver::attachVideoToTextureUnit( 
		const String &sMaterialName, const String &sTextureName, 
		const String &sGroupName, int TechniqueLevel, int PassLevel,
		int TextureUnitStateLevel, int width, int height,
		TextureSpecialRenderFX renderMode )
	{
		//Store local copies of sent info
		m_Tec = TechniqueLevel;
		m_Pass= PassLevel;
		m_Unit= TextureUnitStateLevel;
		mTextureName = sTextureName;
		mMaterialName= sMaterialName;

		m_Width = width;
		m_Height = height;
		mRenderModeFx = renderMode;

		unsigned int newTextureSize = width * height;
		PixelFormat pFormat;
		
		switch( renderMode ) {
			case render_normal: 
				m_BytesPerPixel = 3;
				newTextureSize *=3;		//RGB Texture
				pFormat = PF_B8G8R8;	//PF_R8G8B8;
				break;
			case render_to_alpha:		//PF_A8R8G8B8;//PF_A8;//Alpha only texture	
			case render_to_PF_B8G8R8A8: //Normal render, but has an alpha cannel
				m_BytesPerPixel = 4;
				newTextureSize *=4;
				pFormat = PF_B8G8R8A8;
				break;
		}

		//Create our member bitmap memory		
		m_RGBBitmap = new unsigned char[ newTextureSize ];
		memset( m_RGBBitmap, 0, newTextureSize );

		// Set image class to dynamic memory
		m_Image.loadDynamicImage( m_RGBBitmap, m_Width, m_Height, pFormat );
		
		// Now setup texture -- only use 0 mipaps, anything else results
		//in problems under d3d
		mTexture = TextureManager::getSingleton().loadImage( sTextureName, sGroupName, m_Image, TEX_TYPE_2D, 0, 1.0f );
		
		// Grab Our material, then find the Texture Unit
		MaterialPtr material = MaterialManager::getSingleton().getByName( sMaterialName );
		TextureUnitState* t = material->getTechnique(m_Tec)->getPass(m_Pass)->getTextureUnitState(m_Unit);

		//Now, attach the texture to the material texture unit (single layer)
		t->setTextureName( sTextureName, TEX_TYPE_2D);
		t->setTextureFiltering(FO_LINEAR, FO_LINEAR, FO_NONE);
		t->setTextureAddressingMode( TextureUnitState::TAM_CLAMP );
	/*	
		//Perform texture scaling (: thanks to tuan kuranes :)
		Real tempwidth = mTexture->getWidth (); 
		Real tempheight = mTexture->getHeight (); 
		Real widthratio = m_Width - tempwidth; 
		Real heightratio = m_Height - tempheight;

		if (widthratio)
			tempwidth = (widthratio / tempwidth) / 2;
		else
			tempwidth = 0;

		if (heightratio)
			tempheight = (heightratio / tempheight) / 2;
		else 
			tempheight = 0;

		t->setTextureScroll (tempwidth, tempheight);
		t->setTextureScale ( ((Real) mTexture->getWidth())/m_Width, ((Real) mTexture->getHeight())/m_Height); 
	*/
	}
	
	//----------------------------------------------------------------------//
	void TheoraVideoDriver::randomizeTexture( )
	{
		//This creates a little fuzzy grey static video
		unsigned char* temp = m_RGBBitmap;
		int i;

		for( unsigned int x = 0; x < m_Width; x++ )
		{
			for( unsigned int y = 0; y < m_Height; y++ )
			{
				i = 155 + (rand() % 100);
				*temp++ = 255;//(unsigned char)i;
				*temp++ = 255;//(unsigned char)i;
				*temp++ = 255;//(unsigned char)i;
				*temp++ = (unsigned char)i;
			}
		}
	}

	//----------------------------------------------------------------------//
	void TheoraVideoDriver::renderToTexture( yuv_buffer *buffer )
	{
		//Dispatch to appropriate optimized renderer
		unsigned int time=GetTickCount();
		switch( mRenderModeFx ) {
			case render_normal:
			case render_to_PF_B8G8R8A8:
				decodeYUVtoTexture( buffer );
				break;
			case render_to_alpha:
				decodeYtoTexture( buffer );
				break;
		}
		mYUVConvertTime=GetTickCount()-time;
		//Blit bitmap to texture XXX - todo - replace with lock/unlock
		Box b( 0,0,0,m_Width,m_Height,1);
		time=GetTickCount();
		mTexture->getBuffer()->blitFromMemory( m_Image.getPixelBox(), b );
		mBlitTime=GetTickCount()-time;
	}

	//----------------------------------------------------------------------//
	void TheoraVideoDriver::decodeYtoTexture( yuv_buffer *yuv )
	{
		//Converts 4:2:0 YUV YCrCb to an Alpha Channel (PF_A8)
		unsigned char *dstBitmap		= m_RGBBitmap,
					  *dstBitmapOffset  = m_RGBBitmap + (m_Width * m_BytesPerPixel),
					  *ySrc				= reinterpret_cast<unsigned char*>(yuv->y),
					  *ySrc2			= ySrc + yuv->y_stride;
		
		//Calculate buffer offsets
		unsigned int dstOff = m_Width * m_BytesPerPixel;
		int yOff = yuv->y_stride - yuv->y_width;
			
		
		//Check if upside down, if so, reverse buffers and offsets
		if ( yuv->y_height < 0 )
		{
			yuv->y_height = -yuv->y_height;
			ySrc		 += (yuv->y_height - 1) * yuv->y_stride;
						
			ySrc2 = ySrc - yuv->y_stride;
			yOff  = -yuv->y_width - ( yuv->y_stride * 2 );
		}

		//Cut width and height in half
		//yuv->y_height = yuv->y_height >> 1;
		//yuv->y_width = yuv->y_width >> 1;

		char cAlpha;

	//	randomizeTexture();
	//	return;
		//Loop does two rows at a time
		for (int y = 0; y < yuv->y_height; ++y )
		{
			for (int x = 0; x < yuv->y_width; ++x) 
			{
				cAlpha = MAX( MIN((YTable[*ySrc] >> 13), 255), 0 );
				++ySrc;
				*dstBitmap++ = 255;
				*dstBitmap++ = 255;
				*dstBitmap++ = 255;
				*dstBitmap++ = cAlpha;
				
//				dstBitmap += 8;
//				dstBitmapOffset += 8;
			} // end for x

			//Advance destination pointers by offsets
//			dstBitmap		+= dstOff;
//			dstBitmapOffset += dstOff;
			ySrc			+= yOff;
//			ySrc2			+= yOff;
		} //end for y
	}

	//----------------------------------------------------------------------//
	void TheoraVideoDriver::decodeYUVtoTexture( yuv_buffer *yuv )
	{
		//Convert 4:2:0 YUV YCrCb to an RGB24 Bitmap
		//convenient pointers
		unsigned char *dstBitmap = m_RGBBitmap;
		unsigned char *dstBitmapOffset = m_RGBBitmap + (m_BytesPerPixel * m_Width);

		unsigned char *ySrc = (unsigned char*)yuv->y,
					  *uSrc = (unsigned char*)yuv->u,
					  *vSrc = (unsigned char*)yuv->v,
					  *ySrc2 = ySrc + yuv->y_stride;
		
		//Calculate buffer offsets
		unsigned int dstOff = m_Width * m_BytesPerPixel;//( m_Width*6 ) - ( yuv->y_width*3 );
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
				CLIP_RGB_COLOR( r, dstBitmap[0] );
				CLIP_RGB_COLOR( g, dstBitmap[1] );
				CLIP_RGB_COLOR( b, dstBitmap[2] );
				
				//And repeat for other pixels (note, y is unique for each
				//pixel, while uv are not)
				rgbY = YTable[*ySrc];
				r = (rgbY + rV)  >> 13;
				g = (rgbY - gUV) >> 13;
				b = (rgbY + bU)  >> 13;
				CLIP_RGB_COLOR( r, dstBitmap[m_BytesPerPixel] );
				CLIP_RGB_COLOR( g, dstBitmap[m_BytesPerPixel+1] );
				CLIP_RGB_COLOR( b, dstBitmap[m_BytesPerPixel+2] );
				++ySrc;

				rgbY = YTable[*ySrc2];
				r = (rgbY + rV)  >> 13;
				g = (rgbY - gUV) >> 13;
				b = (rgbY + bU)  >> 13;
				CLIP_RGB_COLOR( r, dstBitmapOffset[0] );
				CLIP_RGB_COLOR( g, dstBitmapOffset[1] );
				CLIP_RGB_COLOR( b, dstBitmapOffset[2] );
				++ySrc2;
				
				rgbY = YTable[*ySrc2];
				r = (rgbY + rV)  >> 13;
				g = (rgbY - gUV) >> 13;
				b = (rgbY + bU)  >> 13;
				CLIP_RGB_COLOR( r, dstBitmapOffset[m_BytesPerPixel] );
				CLIP_RGB_COLOR( g, dstBitmapOffset[m_BytesPerPixel+1] );
				CLIP_RGB_COLOR( b, dstBitmapOffset[m_BytesPerPixel+2] );
				++ySrc2;

				//Advance inner loop offsets
				dstBitmap += m_BytesPerPixel << 1;
				dstBitmapOffset += m_BytesPerPixel << 1;
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

/************************************************************************************
This source file is part of the TheoraVideoPlugin ExternalTextureSource PlugIn 
for OGRE3D (Object-oriented Graphics Rendering Engine)
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright © 2008-2009 Kresimir Spes (kreso@cateia.com)

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
#include "TheoraVideoFrame.h"
#include "TheoraVideoClip.h"

namespace Ogre
{
	#define MAX( a, b ) ((a > b) ? a : b)
	#define MIN( a, b ) ((a < b) ? a : b)
	#define CLIP_RGB_COLOR( rgb_color_test ) MAX( MIN(rgb_color_test, 255), 0 )

	unsigned int YTable [256];
	unsigned int BUTable[256];
	unsigned int GUTable[256];
	unsigned int GVTable[256];
	unsigned int RVTable[256];

	TheoraVideoFrame::TheoraVideoFrame(TheoraVideoClip* parent)
	{
		mReady=mInUse=false;
	
		mParent=parent;
		mBuffer=new unsigned char[mParent->mTexWidth * mParent->mHeight * 4];
	}

	TheoraVideoFrame::~TheoraVideoFrame()
	{
		if (mBuffer) delete mBuffer;
	}

	unsigned char* TheoraVideoFrame::getBuffer()
	{
		return mBuffer;
	}

	void TheoraVideoFrame::fillBackColour(unsigned int colour)
	{
		unsigned int* p=(unsigned int*) mBuffer;
		int x,y;
		for (y=0;y<mParent->mHeight;y++)
		{
			p+=mParent->mWidth;
			for (x=mParent->mWidth;x<mParent->mTexWidth;x++)
			{
				*p=colour;
				p++;
			}
		}
	}

	void TheoraVideoFrame::decodeRGB(th_ycbcr_buffer yuv)
	{
		int t,y;
		unsigned char *ySrc=yuv[0].data, *ySrc2=yuv[0].data,*ySrcEnd,
					  *uSrc=yuv[1].data, *uSrc2=yuv[1].data,
		              *vSrc=yuv[2].data, *vSrc2=yuv[2].data;
		unsigned int *out=(unsigned int*) mBuffer;
		int r, g, b, cu, cv, bU, gUV, rV, rgbY;

		for (y=0;y<yuv[0].height;y++)
		{
			t=0; ySrc=ySrc2; ySrcEnd=ySrc2+yuv[0].width; uSrc=uSrc2; vSrc=vSrc2;
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
			if (mParent->mTexWidth-mParent->mWidth)
			{
				out+=mParent->mTexWidth-mParent->mWidth;
			}
			ySrc2+=yuv[0].stride;
			if (y%2 == 1) { uSrc2+=yuv[1].stride;; vSrc2+=yuv[2].stride; }
		}
	}
	
	void TheoraVideoFrame::decodeGrey(th_ycbcr_buffer yuv)
	{
		unsigned char*ySrc=yuv[0].data,*ySrc2=yuv[0].data,*ySrcEnd;
		unsigned int cy;
		unsigned char* out=mBuffer;
		int y;

		for (y=0;y<yuv[0].height;y++)
		{
			ySrc=ySrc2; ySrcEnd=ySrc2+yuv[0].width;
			while (ySrc != ySrcEnd)
			{
				cy=*ySrc;
				out[0]=out[1]=out[2]=cy;
				out+=4;
				ySrc++;
			}
			if (mParent->mTexWidth-mParent->mWidth)
			{
				out+=(mParent->mTexWidth-mParent->mWidth)*4;
			}
			ySrc2+=yuv[0].stride;
		}
	}
	
	void TheoraVideoFrame::decodeYUV(th_ycbcr_buffer yuv)
	{
		int t,y;
		unsigned char *ySrc=yuv[0].data, *ySrc2=yuv[0].data,*ySrcEnd,
					  *uSrc=yuv[1].data, *uSrc2=yuv[1].data,
		              *vSrc=yuv[2].data, *vSrc2=yuv[2].data;
		unsigned int *out=(unsigned int*) mBuffer;

		for (y=0;y<yuv[0].height;y++)
		{
			t=0; ySrc=ySrc2; ySrcEnd=ySrc2+yuv[0].width; uSrc=uSrc2; vSrc=vSrc2;
			while (ySrc != ySrcEnd)
			{
				*out=(((*ySrc << 8) | *uSrc) << 8) | *vSrc;
				out++;
				ySrc++;
				if (t=!t == 1) { uSrc++; vSrc++; }
			}
			if (mParent->mTexWidth-mParent->mWidth)
			{
				out+=mParent->mTexWidth-mParent->mWidth;
			}
			ySrc2+=yuv[0].stride;
			if (y%2 == 1) { uSrc2+=yuv[1].stride;; vSrc2+=yuv[2].stride; }
		}
	}

	void TheoraVideoFrame::decode(th_ycbcr_buffer yuv)
	{
		TheoraOutputMode mode=mParent->getOutputMode();
		if      (mode == TH_RGB)  decodeRGB(yuv);
		else if (mode == TH_GREY) decodeGrey(yuv);
		else if (mode == TH_YUV)  decodeYUV(yuv);
		mReady=true;
	}

	void createYUVtoRGBtables()
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

} // end namespace Ogre

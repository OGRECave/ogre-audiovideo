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
#include "TheoraFrameQueue.h"
#include "TheoraVideoFrame.h"

namespace Ogre
{
	TheoraFrameQueue::TheoraFrameQueue(int n,TheoraVideoClip* parent):
		mSize(0),
		mQueue(0)
	{
		mParent=parent;
		setSize(n);
	}
	
	TheoraFrameQueue::~TheoraFrameQueue()
	{
		if (mQueue)
		{
			for (int i=0;i<mSize;i++)
				delete mQueue[i];
			delete mQueue;
		}
	}

	void TheoraFrameQueue::setSize(int n)
	{
		if (mQueue)
		{
			// todo: copy frames
			//       and delete each frame
			delete mQueue;
		}
		mQueue=new TheoraVideoFrame*[n];
		for (int i=0;i<n;i++)
			mQueue[i]=new TheoraVideoFrame(mParent);

		mSize=n;
	}

	int TheoraFrameQueue::getSize()
	{
		return mSize;
	}

	TheoraVideoFrame* TheoraFrameQueue::getFirstAvailableFrame()
	{
		if (mQueue[0]->mReady) return mQueue[0];
		return 0;
	}

	void TheoraFrameQueue::pop()
	{
		TheoraVideoFrame* first=mQueue[0];

		for (int i=0;i<mSize-1;i++)
		{
			mQueue[i]=mQueue[i+1];
		}
		mQueue[mSize-1]=first;

		first->mInUse=false;
		first->mReady=false;

	}
		
	TheoraVideoFrame* TheoraFrameQueue::requestEmptyFrame()
	{
		for (int i=0;i<mSize;i++)
		{
			if (!mQueue[i]->mInUse)
			{
				mQueue[i]->mInUse=true;
				mQueue[i]->mReady=false;
				return mQueue[i];
			}
		}
		return 0;
	}

	void TheoraFrameQueue::fillBackColour(unsigned int colour)
	{
		mBackColour=colour;
		for (int i=0;i<mSize;i++) mQueue[i]->fillBackColour(colour);
	}

	unsigned int TheoraFrameQueue::getBackColour()
	{
		return mBackColour;
	}


} // end namespace Ogre

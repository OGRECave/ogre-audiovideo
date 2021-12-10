/**
* @author  Ian Stangoe
*
* LICENSE:
* 
* This source file is part of OgreOggSound, an OpenAL wrapper library for   
* use with the Ogre Rendering Engine.										 
*                                                                           
* Copyright (c) 2017 Ian Stangoe
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE. 	 
*
*/

#include "OgreOggListener.h"
#include "OgreOggSound.h"
#include <OgreMovableObject.h>

namespace OgreOggSound
{
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggListener::setVelocity(const Ogre::Vector3 &vel)
	{
		mVelocity = vel;	
		alListener3f(AL_VELOCITY, vel.x, vel.y, vel.z);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggListener::update()
	{
		#if OGRE_VERSION_MAJOR != 2
		if(mLocalTransformDirty)
		{
			if ( mParentNode )
			{
			#if OGGSOUND_THREADED
				OGRE_WQ_LOCK_MUTEX(mMutex);
			#endif
				auto pos = mParentNode->_getDerivedPosition();

				auto q = mParentNode->_getDerivedOrientation();
				auto vDirection = q.zAxis();
				auto vUp = q.yAxis();
				mOrientation[0] = -vDirection.x;
				mOrientation[1] = -vDirection.y;
				mOrientation[2] = -vDirection.z;
				mOrientation[3] = vUp.x;
				mOrientation[4] = vUp.y;
				mOrientation[5] = vUp.z;

				alListenerfv(AL_ORIENTATION,mOrientation);
				alListener3f(AL_POSITION,pos.x,pos.y,pos.z);
			}
			mLocalTransformDirty=false;
		}
		#else
		if (mParentNode) {
			Ogre::Vector3    pos    = mParentNode->_getDerivedPosition();
			if (pos != mPosition) {
				alListener3f(AL_POSITION,pos.x,pos.y,pos.z);
				mPosition = pos;
			}
			
			Ogre::Quaternion q = mParentNode->_getDerivedOrientation();
			if (q != mOrient) {
				auto vDirection = q.zAxis();
				auto vUp = q.yAxis();
				mOrientation[0] = -vDirection.x;
				mOrientation[1] = -vDirection.y;
				mOrientation[2] = -vDirection.z;
				mOrientation[3] = vUp.x;
				mOrientation[4] = vUp.y;
				mOrientation[5] = vUp.z;
				alListenerfv(AL_ORIENTATION,mOrientation);
				mOrient = q;
			}
		}
		#endif
	}
	/*/////////////////////////////////////////////////////////////////*/
	const Ogre::AxisAlignedBox& OgreOggListener::getBoundingBox(void) const
	{
		static Ogre::AxisAlignedBox aab;
		return aab;
	}
	/*/////////////////////////////////////////////////////////////////*/
	float OgreOggListener::getBoundingRadius(void) const
	{
		return 0;
	}
	/*/////////////////////////////////////////////////////////////////*/
	const Ogre::String& OgreOggListener::getMovableType(void) const
	{
		return OgreOggSoundFactory::FACTORY_TYPE_NAME;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggListener::_notifyAttached(
		Ogre::Node* node
		#if OGRE_VERSION_MAJOR != 2
		, bool isTagPoint
		#endif
	)
	{
		// Call base class notify
		Ogre::MovableObject::_notifyAttached(
			node
			#if OGRE_VERSION_MAJOR != 2
			, isTagPoint
			#endif
		);

		// Immediately set position/orientation when attached
		if (mParentNode)
		{
			mLocalTransformDirty = true;
			update();
		}

		return;
	}
	/*/////////////////////////////////////////////////////////////////*/
	#if OGRE_VERSION_MAJOR != 2
	void OgreOggListener::_notifyMoved(void) 
	{ 
		// Call base class notify
		Ogre::MovableObject::_notifyMoved();

		mLocalTransformDirty=true; 
	}
	#endif
}

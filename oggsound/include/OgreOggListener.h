/**
* @file OgreOggSoundListener.h
* @author  Ian Stangoe
* @version v1.26
*
* @section LICENSE
* 
* This source file is part of OgreOggSound, an OpenAL wrapper library for   
* use with the Ogre Rendering Engine.										 
*                                                                           
* Copyright (c) 2013 Ian Stangoe
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
* @section DESCRIPTION
* 
* Listener object (The users 'ears')
*/

#pragma once

#include "OgreOggSoundPrereqs.h"
#include <Ogre.h>
#include <OgreMovableObject.h>

#if OGGSOUND_THREADED
#	ifdef POCO_THREAD
#		include "Poco/Mutex.h"
#	else 
#		include <boost/thread/recursive_mutex.hpp>
#	endif
#endif

namespace OgreOggSound
{
	//! Listener object (Users ears)
	/** Handles properties associated with the listener.
	*/
	class  _OGGSOUND_EXPORT OgreOggListener : public Ogre::MovableObject
	{

	public:		

		/** Constructor
		@remarks
			Creates a listener object to act as the ears of the user. 
		 */
		OgreOggListener(
			#if OGRE_VERSION_MAJOR == 2
			Ogre::IdType id, Ogre::SceneManager *scnMgr, Ogre::ObjectMemoryManager *objMemMgr, Ogre::uint8 renderQueueId
			#else
			Ogre::SceneManager* scnMgr = NULL
			#endif
		): 
			#if OGRE_VERSION_MAJOR == 2 && OGRE_VERSION_MINOR == 0
				MovableObject(id, objMemMgr, renderQueueId),
			#elif OGRE_VERSION_MAJOR == 2 && OGRE_VERSION_MINOR > 0
				MovableObject(id, objMemMgr, scnMgr, renderQueueId),
			#endif
			  mPosition(Ogre::Vector3::ZERO)
			, mVelocity(Ogre::Vector3::ZERO)
			#if OGRE_VERSION_MAJOR != 2
			, mLocalTransformDirty(false)
			#endif
			, mSceneMgr(scnMgr)
		{
			for (int i=0; i<6; ++i ) mOrientation[i]=0.f;
			mName = "OgreOggListener";
			#if OGRE_VERSION_MAJOR == 2
			setLocalAabb(Ogre::Aabb::BOX_NULL);
			setQueryFlags(0);
			#endif
		};
		/** Sets the position of the listener.
		@remarks
			Sets the 3D position of the listener. This is a manual method,
			if attached to a SceneNode this will automatically be handled 
			for you.
			@param
				x/y/z position.
		*/
		void setPosition(ALfloat x,ALfloat y, ALfloat z);
		/** Sets the position of the listener.
		@remarks
			Sets the 3D position of the listener. This is a manual method,
			if attached to a SceneNode this will automatically be handled 
			for you.
			@param
				pos Vector position.
		*/
		void setPosition(const Ogre::Vector3 &pos);
		/** Gets the position of the listener.
		*/
		Ogre::Vector3 getPosition() const;
		/** Sets the orientation of the listener.
		@remarks
			Sets the 3D orientation of the listener. This is a manual method,
			if attached to a SceneNode this will automatically be handled 
			for you.
			@param
				x/y/z direction.
			@param
				upx/upy/upz up.
		 */
		void setOrientation(ALfloat x, ALfloat y, ALfloat z, ALfloat upx, ALfloat upy, ALfloat upz);
		/** Sets the orientation of the listener.
		@remarks
			Sets the 3D orientation of the listener. This is a manual method,
			if attached to a SceneNode this will automatically be handled 
			for you.
			@param
				q Orientation quaternion.
		 */
		void setOrientation(const Ogre::Quaternion &q);
		/** Gets the orientation of the listener.
		*/
		Ogre::Vector3 getOrientation() const;
		/** Sets sounds velocity.
		@param
			vel 3D x/y/z velocity
		 */
		void setVelocity(float velx, float vely, float velz);
		/** Sets sounds velocity.
		@param
			vel 3D vector velocity
		 */
		void setVelocity(const Ogre::Vector3 &vel);
		/** Updates the listener.
		@remarks
			Handles positional updates to the listener either automatically
			through the SceneGraph attachment or manually using the 
			provided functions.
		 */
		void update();
		/** Gets the movable type string for this object.
		@remarks
			Overridden function from MovableObject, returns a 
			Sound object string for identification.
		 */
		virtual const Ogre::String& getMovableType(void) const;
		/** Gets the bounding box of this object.
		@remarks
			Overridden function from MovableObject, provides a
			bounding box for this object.
		 */
		virtual const Ogre::AxisAlignedBox& getBoundingBox(void) const;
		/** Gets the bounding radius of this object.
		@remarks
			Overridden function from MovableObject, provides the
			bounding radius for this object.
		 */
		virtual float getBoundingRadius(void) const;
		/** Updates the RenderQueue for this object
		@remarks
			Overridden function from MovableObject.
		 */
		virtual void _updateRenderQueue(Ogre::RenderQueue *queue);
		#if OGRE_VERSION_MAJOR != 2 || OGRE_VERSION_MINOR == 0
		/** Renderable callback
		@remarks
			Overridden function from MovableObject.
		 */
		virtual void visitRenderables(Ogre::Renderable::Visitor* visitor, bool debugRenderables);
		#endif
		/** Attach callback
		@remarks
			Overridden function from MovableObject.
		 */
		virtual void _notifyAttached(
			Ogre::Node* node
			#if OGRE_VERSION_MAJOR != 2
			, bool isTagPoint = false
			#endif
		);
		#if OGRE_VERSION_MAJOR != 2
		/** Moved callback
		@remarks
			Overridden function from MovableObject.
		 */
		virtual void _notifyMoved(void);
		#else
		/** do nothing but need for derived from MovableObject
		 */
		virtual void _updateRenderQueue(Ogre::RenderQueue *queue, Ogre::Camera *camera, const Ogre::Camera *lodCamera);
		#endif
		/** Returns scenemanager which created this listener.
		 */
		Ogre::SceneManager* getSceneManager() { return mSceneMgr; }
		#if OGRE_VERSION_MAJOR != 2
		/** Sets scenemanager which created this listener.
		 */
		void setSceneManager(Ogre::SceneManager& m) { mSceneMgr=&m; }
		#endif
	private:

#if OGGSOUND_THREADED
#	if POCO_THREAD
		static Poco::Mutex mMutex;
#	else
		static boost::recursive_mutex mMutex;
#	endif
#endif

		/**
		 * Positional variables
		 */
		Ogre::Vector3 mPosition;		// 3D position
		Ogre::Vector3 mVelocity;		// 3D velocity
		float mOrientation[6];			// 3D orientation
		#if OGRE_VERSION_MAJOR == 2
		Ogre::Quaternion mOrient;		// 3D orientation as Quaternion
		#else
		bool mLocalTransformDirty;		// Dirty transforms flag
		#endif
		Ogre::SceneManager* mSceneMgr;	// Creator 

	};
}
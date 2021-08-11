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
* DESCRIPTION: Pre-requisites for building the library
*/
#pragma once 

#include <Ogre.h>
#include <OgreDataStream.h>
#include <OgreMovableObject.h>
#include <OgreLogManager.h>

/**
 * HAVE_EFX: Specifies whether EFX enhancements are supported
 * 0 - EFX not supported
 * 1 - Enable EFX support with Creative OpenAL SDK 1.1
 * 2 - Enable EFX support with OpenAL Soft SDK
 */
#ifndef HAVE_EFX
#	define HAVE_EFX 2
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32

#	pragma warning( disable : 4244 )

#	include "al.h"
#	include "alc.h"

#	if HAVE_EFX == 1
#		include "efx.h"
#		include "efx-util.h"
#		include "efx-creative.h"
#		include "xram.h"
#	elif HAVE_EFX == 2
#		include "efx.h"
#		include "efx-presets.h"
#	endif

#	if OGRE_COMPILER == OGRE_COMPILER_MSVC
#		ifdef OGGSOUND_EXPORT
#			define _OGGSOUND_EXPORT __declspec(dllexport)
#		else
#			define _OGGSOUND_EXPORT __declspec(dllimport)
#		endif
#	else
#		define _OGGSOUND_EXPORT
#	endif

#elif OGRE_COMPILER == OGRE_COMPILER_GNUC || OGRE_COMPILER == OGRE_COMPILER_CLANG
#   if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#		include <al.h>
#		include <alc.h>
#   else
#		include <AL/al.h>
#		include <AL/alc.h>
#		if HAVE_EFX == 2
#			include "AL/efx.h"
#			include "AL/efx-presets.h"
#		endif
#	endif
#	if defined(OGGSOUND_EXPORT) && OGRE_COMP_VER >= 400
#		define _OGGSOUND_EXPORT __attribute__ ((visibility("default")))
#	else
#		define _OGGSOUND_EXPORT
#	endif
#else // Other Compilers
#	include <OpenAL/al.h>
#	include <OpenAL/alc.h>
#	include "xram.h"
#	define _OGGSOUND_EXPORT
#endif
	
namespace OgreOggSound
{
	typedef std::vector<ALuint> BufferList; // A list of the IDs of all OpenAL buffers being used by a sound. This is a vector so that it can be passed around as an array to the various OpenAL functions.
	typedef Ogre::SharedPtr<BufferList> BufferListPtr; // An Ogre::SharedPtr to the list of buffer IDs. This makes it easier to pass the values to multiple OgreOggISound instances without having to manage memory.

	//! Holds information about a static shared audio buffer.
	struct sharedAudioBuffer
	{
		sharedAudioBuffer() :
			 mAudioBuffer(AL_NONE)
			,mRefCount(0)
			,mBuffers()
			,mPlayTime(0.0)
			,mFormat(AL_NONE)
		{ }

		ALuint mAudioBuffer;	/// OpenAL buffer
		unsigned int mRefCount;	/// Reference counter
		BufferListPtr mBuffers;	/// The cached common buffers to use between all sounds using this shared audio buffer.
		float mPlayTime;		/// The cached play time of the audio buffer.
		ALenum mFormat;			/// The cached format of the audio buffer.
	};

	typedef std::map<std::string, sharedAudioBuffer*> SharedBufferList;
};

/**
 * Specifies whether to use threads for streaming
 * 0 - No multithreading
 * 1 - OGRE-native multithreading
 */
#ifndef OGGSOUND_THREADED
	#define OGGSOUND_THREADED 0
#endif



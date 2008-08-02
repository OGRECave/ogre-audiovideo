// Modified From Portable Audio:
// Put Methods into Classes & namespace
// Visit www.wreckedgames.com or www.ogre3d.org
// Original File Name and copyright info:
//
/*
 * $Id: TheoraRingBuffer.h,v 1.3 2004-08-21 22:19:36 pjcast Exp $
 * ringbuffer.h
 * Ring Buffer utility..
 *
 * Author: Phil Burk, http://www.softsynth.com
 *
 * This program is distributed with the PortAudio Portable Audio Library.
 * For more information see: http://www.audiomulch.com/portaudio/
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef Theora_Player_RingBuffer
#define Theora_Player_RingBuffer

#include "TheoraExport.h"

namespace Ogre
{
	class _OgreTheoraExport RingBuffer
	{
	public:
		RingBuffer() : bufferSize(0), writeIndex(0), readIndex(0), 
					   bigMask(0), smallMask(0), buffer(0)	{}
		
		~RingBuffer() {}
		/** Clear buffer. Should only be called when buffer is NOT being read. */
		void flush( );

		/** Initialize FIFO.
			numBytes must be power of 2, returns -1 if not. */
		bool init( long numBytes, void *dataPtr );

		/** Return number of bytes available for reading. */
		long getReadAvailable( );

		/** Return number of bytes available for writing. */
		long getWriteAvailable( );

		/** Returns bytes written. */
		long write( void *data, long numBytes );
		
		/** Returns bytes read */
		long read( void *data, long numBytes );

	protected:
		long advanceWriteIndex( long numBytes );

		/** Get address of region(s) to which we can write data.
			If the region is contiguous, size2 will be zero.
			If non-contiguous, size2 will be the size of second region.
			Returns room available to be written or numBytes, whichever is smaller. */
		long getWriteRegions( long numBytes, void **dataPtr1, long *sizePtr1,
							  void **dataPtr2, long *sizePtr2 );
		
		/** Get address of region(s) from which we can read data.
			If the region is contiguous, size2 will be zero.
			If non-contiguous, size2 will be the size of second region.
			Returns room available to be written or numBytes, whichever is smaller. */
		long getReadRegions( long numBytes, void **dataPtr1, long *sizePtr1,
							 void **dataPtr2, long *sizePtr2 );

		long advanceReadIndex( long numBytes );
		
		// These are declared volatile because they are written by a 
		// different thread than the reader.
		//! Index of next writable byte. Set by RingBuffer_AdvanceWriteIndex.
		volatile long writeIndex; 
		//! Index of next readable byte. Set by RingBuffer_AdvanceReadIndex.
		volatile long readIndex;

	public:
		//Number of bytes in FIFO. Power of 2. Set by RingBuffer_Init
		long bufferSize;

		//! Used for wrapping indices with extra bit to distinguish full/empty.
		long bigMask;
		// Used for fitting indices to buffer.
		long smallMask;
		char *buffer;
	};
}
#endif //Theora_Player_RingBuffer


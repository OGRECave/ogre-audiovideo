// Modified From Portable Audio:
// Put Methods into Classes & namespace
// Original Copyright Notice:
//
/*
 * PortAudio Portable Real-Time Audio Library
 * Latest version at: http://www.audiomulch.com/portaudio/
 * <platform> Implementation
 * Copyright (c) 1999-2000 <author(s)>
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
#include <memory.h>
#include "TheoraRingBuffer.h"

namespace Ogre
{
	//---------------------------------------------------------------//
	bool RingBuffer::init( long numBytes, void *dataPtr )
	{
		if( ((numBytes-1) & numBytes) != 0) 
			return false; //Not Power of two.

		bufferSize = numBytes;
		buffer = (char *)dataPtr;
		
		flush( );
		
		bigMask = (numBytes*2)-1;
		smallMask = (numBytes)-1;

		return true;
	}

	//---------------------------------------------------------------//
	long RingBuffer::getReadAvailable( )
	{
		return ( (writeIndex - readIndex) & bigMask );
	}

	//---------------------------------------------------------------//
	long RingBuffer::getWriteAvailable( )
	{
		return ( bufferSize - getReadAvailable());
	}

	//---------------------------------------------------------------//
	long RingBuffer::getWriteRegions( long numBytes, void **dataPtr1, long *sizePtr1,
									 void **dataPtr2, long *sizePtr2 )
	{
		long   index;
		long   available = getWriteAvailable( );
		if( numBytes > available ) 
			numBytes = available;
		
		//Check to see if write is not contiguous.
		index = writeIndex & smallMask;
		if( (index + numBytes) > bufferSize )
		{
			//Write data in two blocks that wrap the buffer.
			long   firstHalf = bufferSize - index;
			*dataPtr1 = &buffer[index];
			*sizePtr1 = firstHalf;
			*dataPtr2 = &buffer[0];
			*sizePtr2 = numBytes - firstHalf;
		}
		else
		{
			*dataPtr1 = &buffer[index];
			*sizePtr1 = numBytes;
			*dataPtr2 = 0;
			*sizePtr2 = 0;
		}
		return numBytes;
	}
	
	//---------------------------------------------------------------//
	long RingBuffer::getReadRegions( long numBytes,
									void **dataPtr1, long *sizePtr1,
									void **dataPtr2, long *sizePtr2 )
	{
		long   index;
		long   available = getReadAvailable( );
		if( numBytes > available ) numBytes = available;
		/* Check to see if read is not contiguous. */
		index = readIndex & smallMask;
		if( (index + numBytes) > bufferSize )
		{
			/* Write data in two blocks that wrap the buffer. */
			long firstHalf = bufferSize - index;
			*dataPtr1 = &buffer[index];
			*sizePtr1 = firstHalf;
			*dataPtr2 = &buffer[0];
			*sizePtr2 = numBytes - firstHalf;
		}
		else
		{
			*dataPtr1 = &buffer[index];
			*sizePtr1 = numBytes;
			*dataPtr2 = 0;
			*sizePtr2 = 0;
		}
		return numBytes;
	}

	//---------------------------------------------------------------//
	long RingBuffer::advanceWriteIndex( long numBytes )
	{
		return writeIndex = (writeIndex + numBytes) & bigMask;
	}

	//---------------------------------------------------------------//
	long RingBuffer::advanceReadIndex( long numBytes )
	{
		return readIndex = (readIndex + numBytes) & bigMask;
	}

	//---------------------------------------------------------------//
	long RingBuffer::write( void *data, long numBytes )
	{
		long size1, size2, numWritten;
		void *data1, *data2;
		numWritten = getWriteRegions( numBytes, &data1, &size1, &data2, &size2 );
		if( size2 > 0 )
		{

			memcpy( data1, data, size1 );
			data = ((char *)data) + size1;
			memcpy( data2, data, size2 );
		}
		else
		{
			memcpy( data1, data, size1 );
		}
		advanceWriteIndex( numWritten );
		return numWritten;
	}

	//---------------------------------------------------------------//
	long RingBuffer::read( void *data, long numBytes )
	{
		long size1, size2, numRead;
		void *data1, *data2;
		numRead = getReadRegions( numBytes, &data1, &size1, &data2, &size2 );
		if( size2 > 0 )
		{
			memcpy( data, data1, size1 );
			data = ((char *)data) + size1;
			memcpy( data, data2, size2 );
		}
		else
		{
			memcpy( data, data1, size1 );
		}
		advanceReadIndex( numRead );
		return numRead;
	}
	//---------------------------------------------------------------//
	void RingBuffer::flush( )
	{
		writeIndex = readIndex = 0;
	}
}



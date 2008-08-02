//Modified file:
// Put in Class
// Removed PA Specific Types
//Original Copyright Notice:
//
/********************************************************************
 *                                                                  *
 * THIS FILE WAS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'License.txt'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.   *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2003                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************/
#include "TheoraAudioDriver.h"

namespace Ogre
{
	//---------------------------------------------------------------//
	bool TheoraAudioDriver::initFIFO( long numFrames )
	{
		long numBytes = numFrames * bytesPerFrame;
		
		char *buffer = new char[ numBytes ];
		
		if( !buffer ) 
			return false;

		memset( buffer, 0xCC, numBytes );

		return outFIFO.init( numBytes, buffer );
	}

	//---------------------------------------------------------------//
	void TheoraAudioDriver::termFIFO( )
	{
		if( outFIFO.buffer ) 
			delete [] outFIFO.buffer;

		outFIFO.buffer = 0;
	}

	//---------------------------------------------------------------//
	TheoraAudioDriver::TheoraAudioDriver()
	{
		stream = 0;
		bytesPerFrame = 0;
		samplesPerFrame = 0;
		/////
		samples = 0;	//local buffer for samples
		latency_sec = 0;
	}

	//---------------------------------------------------------------//
	bool TheoraAudioDriver::open( vorbis_info *vorbisInfo, unsigned int newMilliSeconds )
	{
		//this will open one circular audio stream
		//build on top of portaudio routines
		//implementation based on file pastreamio.c
		int numSamples;
		int numBytes;

		m_minNumBuffers = 2 * LATENCY_BUFFERS;
		m_numFrames = m_minNumBuffers * FRAMES_PER_BUFFER;
		m_numFrames = roundUpToNextPowerOf2( m_numFrames );

		numSamples = m_numFrames * vorbisInfo->channels;
		numBytes = numSamples * sizeof( short );

		samples = new short[ numBytes ];

		/* store our latency calculation here */
		latency_sec =  (double) m_numFrames / vorbisInfo->rate / vorbisInfo->channels;
		std::cout << "Latency: " << latency_sec << std::endl;

	//////////
		unsigned int bytesPerSample = 2;
		samplesPerFrame = 2;
		bytesPerFrame = bytesPerSample * samplesPerFrame;

		//Initialize Ring Buffer
		if( initFIFO( m_numFrames ) == false )
			return false;

		//Make Write FIFO appear full initially. 
		//	numBytes = aOutStream->outFIFO.getWriteAvailable( );
		//	aOutStream->outFIFO.advanceWriteIndex( numBytes );

		//XXX: Fix buffersize using define or such
		if(openAudioStream(vorbisInfo->rate, SF_16Bit, 1024, THEORA_AUDIOSTREAM_STEREO, newMilliSeconds))
 			return true;
		
		//Else we close everything
		closeAudioStream( );
		std::cout << "An error occured while opening the portaudio stream" << std::endl;
		return false;
	}

	//---------------------------------------------------------------//
	bool TheoraAudioDriver::close()
	{
		//Call abstracted close method
		closeAudioStream( );

		termFIFO();
    
		delete [] samples;
		return true;
	}

	//---------------------------------------------------------------//
	//Write data to ring buffer.
	//Will not return until all the data has been written.
	unsigned int TheoraAudioDriver::writeAudioStream( unsigned int numFrames )
	{
		//XXX: modify not to sleep
		long bytesWritten;
		char *p = (char*)samples;
		long numBytes = bytesPerFrame * numFrames;
		while( numBytes > 0)
		{
			bytesWritten = outFIFO.write( p, numBytes );
			numBytes -= bytesWritten;
			p += bytesWritten;
			//if( numBytes > 0) Sleep(10);
		}
		return numFrames;
	}

	//---------------------------------------------------------------//
	// Return the number of frames that could be written to the stream without
	// having to wait.
	//
	unsigned int TheoraAudioDriver::getAudioStreamWriteable()
	{
		unsigned int bytesEmpty = outFIFO.getWriteAvailable( );
		return bytesEmpty / bytesPerFrame;
	}

	//---------------------------------------------------------------//
	unsigned int TheoraAudioDriver::roundUpToNextPowerOf2( unsigned int n )
	{
		long numBits = 0;
		if( ((n-1) & n) == 0) 
			return n; //Already Power of two.

		while( n > 0 )
		{
			n= n>>1;
			numBits++;
		}
		return (1<<numBits);
	}
} //end Ogre

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
#ifndef _TheoraAudioDriver_H
#define _TheoraAudioDriver_H

#include "OgrePrerequisites.h"
#include "theora/theora.h"
#include "vorbis/codec.h"
#include "TheoraRingBuffer.h"
#include "TheoraExport.h"

#define FRAMES_PER_BUFFER (256)

//Values for flags for OpenAudioStream().
#define THEORA_AUDIOSTREAM_READ			(1<<0)
#define THEORA_AUDIOSTREAM_WRITE		(1<<1)
#define THEORA_AUDIOSTREAM_READ_WRITE   (THEORA_AUDIOSTREAM_READ|THEORA_AUDIOSTREAMPABLIO_WRITE)
#define THEORA_AUDIOSTREAM_MONO			(1<<2)
#define THEORA_AUDIOSTREAM_STEREO		(1<<3)

//portaudio related global types
#define SAMPLE_SILENCE  (0)

//Used to set number of buffers (adjust as neccessary)
#define LATENCY_BUFFERS (200 << 1)


namespace Ogre
{
	enum eAudioSampleFormat
	{
		SF_8Bit = 0,
		SF_16Bit,
		SF_32Bit
	};

	/**
		This class needs to be subclasses and registered to the movie class
		to use audio with movies.
	*/
	class _OgreTheoraExport TheoraAudioDriver
	{
	public:
		TheoraAudioDriver();
		~TheoraAudioDriver() {}

		/** 
			@remarks
				Pauses/Resumes audio
			@param
				True for pause, false otherwise
		*/
		virtual void setAudioStreamPause( bool pause ) = 0;

		/** 
			@remarks
				Starts Playing Audio Clip... Must be overridden - implementation specific
			@returns
				True for success
		*/
		virtual bool startAudioStream() = 0;
		/** 
			@remarks 
				Gets audio stream time in Milliseconds - implementation specific
			@returns
				Milliseconds
		*/
		virtual float getAudioStreamTime() = 0;
		/** 
			@remarks 
				Open stream and sets up stuff - implementation specific
			@returns
				True for success
		*/
		virtual bool openAudioStream( unsigned int sampleRate, 
									  eAudioSampleFormat sf,
									  unsigned int bufferSize,
									  unsigned int flags,
									  unsigned int newMilliSeconds ) = 0;
		/** 
			@remarks 
				Closes stream and cleans up - implementation specific
			@returns
				True for success
		*/
		virtual bool closeAudioStream( ) = 0;

		virtual void autoUpdate() {}
		
		bool open( vorbis_info *vorbisInfo, unsigned int newMilliSeconds = 0 );
		bool close();

		//! Writes the sample buffer to the RingBuffer
		unsigned int writeAudioStream( unsigned int numFrames );

		unsigned int getAudioStreamWriteable();

		double getLatencySec() { return latency_sec; }

		//! Buffer for decoder to write to
		short	*samples;

		//added from upper class todo
		bool initFIFO( long numFrames );
		void termFIFO( );

		RingBuffer outFIFO;
		void *stream;
		int bytesPerFrame;
		int samplesPerFrame;

	protected:
		unsigned int roundUpToNextPowerOf2( unsigned int n );
		///////////////////////////////////////////////////////////////////
		unsigned int m_minNumBuffers;
		unsigned int m_numFrames;

		//! A latency measurement
		double	latency_sec;

	};
}
#endif //_TheoraAudioDriver_H


# %Ogre Video and Audio Plugins {#mainpage}

This is the API reference for the Ogre Video and Audio plugins; contained within are the 
specifications for each class and the methods on those classes which you can 
refer to when using the library in your code. 

# OggSound 

The %OgreOggSound library is designed to provide a simple, quick and easy method
of adding audio to an OGRE based application. Its primarily a wrapper around the 
OpenAL audio library, but its design focus is seamless integration with OGRE 
based applications. 

It hides all the mundane setup and updating procedures needed by OpenAL, and 
condenses the functionality down to a clean and simple OGRE-fied interface.

## Features

* .ogg file format support
* uncompressed .wav file support
* In memory and streaming support
	* Load whole sound into memory
	* Stream sound from a file
* Optional multithreaded streaming
* Multichannel audio support
* Full 2D/3D audio support
	* spatialized sound support using mono sound files
	* 2D/multichannel support 
* Full control over 3D parameters
	* All 3D properties exposed for customisation
	* Global attenuation model configurable
	* Global sound speed configurable
	* Global doppler effect configurable
* Playback seeking 
* Cue points - Set 'jump-to' points within sounds
* Configurable loop points
	* By default a sound would loop from start to end, however a user can customise this by offsetting the start point of the loop per sound.
* Temporary sounds
	* Allows creation and automatic destruction of single-play/infrequent sounds.
* Source management
	* Sources are pooled 
	* Sources are automatically re-used when a sound requests to play
	* Sounds are re-activated if temporarily stopped.
* OGRE integration support
	* Sound objects are derived from MovableObject
	* can be attached directly into scene graph via SceneNodes
	* Automatically updates transformations
* Audio capturing support to WAV file.
* XRAM hardware buffer support 
	* Currently experimental
* EFX effect support
	* Support for attaching EFX filters/effects to sounds if hardware supported
	* Support for EAX room reverb presets
* Volume control
* Pitch control
* Loop control
* Fully Documented

# Theora Video Plugin

The Theora Video plugin allows referencing theora streams as @ref External-Texture-Sources.

## Features

* Integrates into Ogre3D as a FrameListener
* Supports loading videos from material files
* optional yuv->rgb conversion via shader
  

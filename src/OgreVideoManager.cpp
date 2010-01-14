/************************************************************************************
This source file is part of the Ogre3D Theora Video Plugin
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)

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
#ifndef OGRE_MAC_FRAMEWORK
  #include "OgreRoot.h"
#else
  #include <Ogre/OgreRoot.h>
#endif
#include "OgreVideoManager.h"
#include "OgreTheoraDataStream.h"

#ifndef OGRE_MAC_FRAMEWORK
#include "OgreTextureManager.h"
#include "OgreMaterialManager.h"
#include "OgreMaterial.h"
#include "OgreTechnique.h"
#include "OgreStringConverter.h"
#include "OgreLogManager.h"
#include "OgreHardwarePixelBuffer.h"
#else
#include <Ogre/OgreTextureManager.h>
#include <Ogre/OgreMaterialManager.h>
#include <Ogre/OgreMaterial.h>
#include <Ogre/OgreTechnique.h>
#include <Ogre/OgreStringConverter.h>
#include <Ogre/OgreLogManager.h>
#include <Ogre/OgreHardwarePixelBuffer.h>
#endif

#include "TheoraVideoFrame.h"
#include <vector>

namespace Ogre
{
	void ogrevideo_log(std::string message)
	{
		Ogre::LogManager::getSingleton().logMessage("OgreVideo: "+message);
	}

	int nextPow2(int x)
	{
		int y;
		for (y=1;y<x;y*=2);
		return y;
	}

	OgreVideoManager::OgreVideoManager(int num_worker_threads) : TheoraVideoManager(num_worker_threads)
	{

		mPlugInName = "TheoraVideoPlugin";
		mDictionaryName = mPlugInName;
		mbInit=false;
		mTechniqueLevel=mPassLevel=mStateLevel=0;

		setLogFunction(ogrevideo_log);

		initialise();
	}

	bool OgreVideoManager::initialise()
	{
		if (mbInit) return false;
		mbInit=true;
		addBaseParams(); // ExternalTextureSource's function
		return true;
	}
	
	OgreVideoManager::~OgreVideoManager()
	{
		shutDown();
	}

	void OgreVideoManager::shutDown()
	{
		if (!mbInit) return;

		mbInit=false;
	}
    
    bool OgreVideoManager::setParameter(const String &name,const String &value)
    {
        // Hacky stuff used in situations where you don't have access to OgreVideoManager
        // eg, like when using the plugin in python (and not using the wrapped version by Python-Ogre)
        // these parameters are here temporarily and I don't encourage anyone to use them.
        
        if (name == "destroy")
        {

        }
        
        return ExternalTextureSource::setParameter(name, value);
    }
    
    String OgreVideoManager::getParameter(const String &name) const
    {
        // Hacky stuff used in situations where you don't have access to OgreVideoManager
        // eg, like when using the plugin in python (and not using the wrapped version by Python-Ogre)
        // these parameters are here temporarily and I don't encourage anyone to use them.

        if (name == "started")
        {
            
        }
        else if (name == "finished")
        {
  
        }
        return ExternalTextureSource::getParameter(name);
    }

	void OgreVideoManager::createDefinedTexture(const String& material_name,const String& group_name)
	{
		std::string name=mInputFileName;
		TheoraVideoClip* clip=createVideoClip(new OgreTheoraDataStream(mInputFileName,group_name),TH_RGB);
		int w=nextPow2(clip->getWidth()),h=nextPow2(clip->getHeight());

		TexturePtr t = TextureManager::getSingleton().createManual(name,group_name,TEX_TYPE_2D,w,h,1,0,PF_X8R8G8B8,TU_DYNAMIC_WRITE_ONLY);
		
		if (t->getFormat() != PF_X8R8G8B8) ogrevideo_log("ERROR: Pixel format is not X8R8G8B8 which is what we requested!");
		// clear it to black

		unsigned char* texData=(unsigned char*) t->getBuffer()->lock(HardwareBuffer::HBL_DISCARD);
		memset(texData,0,w*h*4);
		t->getBuffer()->unlock();
		mTextures[name]=t;

		// attach it to a material
		MaterialPtr material = MaterialManager::getSingleton().getByName(material_name);
		TextureUnitState* ts = material->getTechnique(mTechniqueLevel)->getPass(mPassLevel)->getTextureUnitState(mStateLevel);

		//Now, attach the texture to the material texture unit (single layer) and setup properties
		ts->setTextureName(name,TEX_TYPE_2D);
		ts->setTextureFiltering(FO_LINEAR, FO_LINEAR, FO_NONE);
		ts->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);

		// scale tex coords to fit the 0-1 uv range
		Matrix4 mat=Matrix4::IDENTITY;
		mat.setScale(Vector3((float) clip->getWidth()/w, (float) clip->getHeight()/h,1));
		ts->setTextureTransform(mat);

	}

	void OgreVideoManager::destroyAdvancedTexture(const String& material_name,const String& groupName)
	{
		ogrevideo_log("Destroying ogg_video texture on material: "+material_name);

		//ogrevideo_log("Error destroying ogg_video texture, texture not found!");
	}

	bool OgreVideoManager::frameStarted(const FrameEvent& evt)
	{
		update(evt.timeSinceLastFrame);
		// update playing videos
		std::vector<TheoraVideoClip*>::iterator it;
		TheoraVideoFrame* f;
		for (it=mClips.begin();it!=mClips.end();it++)
		{
			f=(*it)->getNextFrame();
			if (f)
			{
				//ogrevideo_log("decoded frame!");
				int w=f->getWidth(),tw=nextPow2(f->getWidth()), h=f->getHeight(), s=f->getStride();
				TexturePtr t=mTextures[(*it)->getName()];

				unsigned char *texData=(unsigned char*) t->getBuffer()->lock(HardwareBuffer::HBL_DISCARD);
				unsigned char *data=f->getBuffer();
				unsigned char *line_end,*img_end=data+s*h*3;

				for (;data!=img_end;texData+=(tw-s)*4)
				{
					for (line_end=data+s*3;data != line_end;texData+=4,data+=3)
					{
						texData[0]=data[2];
						texData[1]=data[1];
						texData[2]=data[0];
						
					}
				}
				
				t->getBuffer()->unlock();
				(*it)->popFrame();
			}
		}
		return true;
	}

} // end namespace Ogre

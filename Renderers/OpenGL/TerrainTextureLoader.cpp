// Texture loader.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Renderers/OpenGL/TerrainTextureLoader.h>
#include <Meta/OpenGL.h>
#include <Logging/Logger.h>

namespace OpenEngine {
namespace Renderers {
namespace OpenGL {

    TerrainTextureLoader::TerrainTextureLoader()
        : TextureLoader() {

    }

    TerrainTextureLoader::~TerrainTextureLoader(){

    }

    /**
     * Load a texture resource with mipmapping enabled.
     *
     * @param tex Texture resource pointer.
     */
    void TerrainTextureLoader::LoadTextureWithMipmapping(ITextureResourcePtr& tex){
        if (tex == NULL) return;
        if(tex->GetID() == 0) {
            tex->Load();

            GLuint texid;
            glGenTextures(1, &texid);
            tex->SetID(texid);
            
            glBindTexture(GL_TEXTURE_2D, texid);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            
            glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            
            GLuint depth = 0;
            switch (tex->GetDepth()) {
            case 8:  depth = GL_LUMINANCE; break;
            case 24: depth = GL_RGB;   break;
            case 32: depth = GL_RGBA;  break;
            default: logger.warning << "Unsupported color depth: " 
                                    << tex->GetDepth() << logger.end;
            }
            glTexImage2D(GL_TEXTURE_2D, 0, depth, tex->GetWidth(), tex->GetHeight(), 0,
                         depth, GL_UNSIGNED_BYTE, tex->GetData());

            glBindTexture(GL_TEXTURE_2D, 0);
            tex->Unload();
        }
    }

} // NS OpenGL
} // NS Renderers
} // NS OpenEngine

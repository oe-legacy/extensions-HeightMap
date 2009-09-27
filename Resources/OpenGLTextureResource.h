// OpenGL texture resource.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _OPENGL_TEXTURE_RESOURCE
#define _OPENGL_TEXTURE_RESOURCE

#include <Resources/ITextureResource.h>

namespace OpenEngine {
    namespace Resources {

        class OpenGLTextureResource : public ITextureResource {
        private:
            GLuint id;
        public:
            OpenGLTextureResource(GLuint id) : id(id) {}
            ~OpenGLTextureResource() {}
            void Load() {}
            void Unload() {}
            int GetID() { return id; }
            void SetID(int id) { throw Exception("OpenGL textures can not change identifiers."); }
            unsigned int GetWidth() { return 400; }
            unsigned int GetHeight() { return 300; }
            unsigned int GetDepth() { return 32; }
            unsigned char* GetData() { throw Exception("Cannot extract data from OpenGL."); }
            ColorFormat GetColorFormat() { return RGBA; }
        };
        
    }
}

#endif

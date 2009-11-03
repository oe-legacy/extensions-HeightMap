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
            unsigned int width, height, depth;
        public:
            OpenGLTextureResource(GLuint id, unsigned int w, unsigned int h, unsigned int d = 24)
                : id(id), width(w), height(h), depth(d) {
            }
            ~OpenGLTextureResource() {}
            void Load() {}
            void Unload() {}
            int GetID() { return id; }
            void SetID(int id) { throw Exception("OpenGL textures can not change identifiers."); }
            unsigned int GetWidth() { return width; }
            unsigned int GetHeight() { return height; }
            unsigned int GetDepth() { return depth; }
            unsigned char* GetData() { throw Exception("Cannot extract data from OpenGL."); }
            ColorFormat GetColorFormat() { 
                switch (depth){
                case 8: return LUMINANCE;
                case 24: return RGB;
                    //case 32: return RGBA;
                default: return RGBA;
                }
            }
        };
        
    }
}

#endif

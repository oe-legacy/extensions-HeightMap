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
        public:
            OpenGLTextureResource(GLuint id, unsigned int w, unsigned int h, unsigned int c = 3)
                : ITextureResource() {
                this->id = id;
                this->width = w;
                this->width = h;
                this->channels = c;
                
                switch(this->channels){
                case 1:
                    format = LUMINANCE;
                    break;
                case 3:
                    format = RGB;
                    break;
                case 4:
                    format = RGBA;
                    break;
                default:
                    throw Exception("unknown color format");
                }

            }
            ~OpenGLTextureResource() {}
            void Load() {}
        };
        
    }
}

#endif

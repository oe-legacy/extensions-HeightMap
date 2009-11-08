// Empty texture resource.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _EMPTY_TEXTURE_RESOURCE_H_
#define _EMPTY_TEXTURE_RESOURCE_H_

#include <Resources/ITextureResource.h>

namespace OpenEngine {
    namespace Resources {

        class EmptyTextureResource : public ITextureResource {
        private:
            unsigned int width;
            unsigned int height;
            unsigned int depth;
            unsigned char* data;
            int id;
        public:
            EmptyTextureResource(unsigned int w, unsigned int h, unsigned int d) 
                : width(w), height(h), depth(d), data(NULL), id(0) {                
            }

            ~EmptyTextureResource() { delete [] data; }
            void Load() { if (!data) data = new unsigned char[width * height * depth / 8]; }
            void Unload() { delete data; }
            int GetID() { return id; }
            void SetID(int id) { this->id = id; }
            unsigned int GetWidth() { return width; }
            unsigned int GetHeight() { return height; }
            unsigned int GetDepth() { return depth; }
            unsigned char* GetData() { return data; }
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

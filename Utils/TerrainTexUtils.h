// Terrain Texture Utils.
// -------------------------------------------------------------------
// Copyright (C) 2010 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _TERRAIN_TEXTURE_UTILS_H_
#define _TERRAIN_TEXTURE_UTILS_H_

#include <Resources/Texture2D.h>

using namespace OpenEngine::Resources;

namespace OpenEngine {
    namespace Utils {
        FloatTexture2DPtr ConvertTex(UCharTexture2DPtr tex);

        UIntTexture2DPtr Combine(UCharTexture2DPtr t1, 
                                 UCharTexture2DPtr t2 = UCharTexture2DPtr(), 
                                 UCharTexture2DPtr t3 = UCharTexture2DPtr(), 
                                 UCharTexture2DPtr t4 = UCharTexture2DPtr());

        template <class T>
        void Empty(Texture2DPtr(T) t){
            t->Load();
            for (unsigned int x = 0; x < t->GetWidth(); ++x)
                for (unsigned int z = 0; z < t->GetWidth(); ++z)
                    for (unsigned int c = 0; c < t->GetChannels(); ++c)
                        t->GetPixel(x, z)[c] = 0;
        }

        template <class T>
        Texture2DPtr(T) ChangeChannels(Texture2DPtr(T) t, unsigned int channels){
            t->Load();
            
            unsigned int w = t->GetWidth();
            unsigned int h = t->GetHeight();
            unsigned int tc = t->GetChannels();

            Texture2DPtr(T) tex = Texture2DPtr(T)(new Texture2D<T>(w, h, channels));
            tex->Load();

            for (unsigned int x = 0; x < w; ++x){
                for (unsigned int z = 0; z < h; ++z){
                    for (unsigned int c = 0; c < channels; ++c){
                        if (c < tc)
                            tex->GetPixel(x, z)[c] = t->GetPixel(x, z)[c];
                        else
                            tex->GetPixel(x, z)[c] = 0;
                    }
                }
            }

            return tex;
        }
    }
}

#endif

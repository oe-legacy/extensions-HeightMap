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

        

        UIntTexture2DPtr Merge(UCharTexture2DPtr t1, 
                               UCharTexture2DPtr t2 = UCharTexture2DPtr(), 
                               UCharTexture2DPtr t3 = UCharTexture2DPtr(), 
                               UCharTexture2DPtr t4 = UCharTexture2DPtr());

        /**
         * Box bluring. Runs in time O(texSize^2 * 2halfsize). While
         * linear time is possible for box bluring, specialized
         * functions would be needed to handle overflow.
         */
        template <class T>
        void BoxBlur(Texture2DPtr(T) tex, int halfsize = 1){
            unsigned int width = tex->GetWidth();
            unsigned int height = tex->GetHeight();
            unsigned char channels = tex->GetChannels();

            Texture2D<T> temp = Texture2D<T>(width, height, channels);
            temp.Load();

            for (unsigned int x = 0; x < width; ++x){
                for (unsigned int y = 0; y < height; ++y){
                    T* pixel = temp.GetPixel(x, y);
                    for (unsigned char c = 0; c < channels; ++c){
                        pixel[c] = 0;
                        for (int X = -halfsize; X <= halfsize; ++X){
                            pixel[c] += tex->GetPixel(x + X, y)[c];
                        }
                        pixel[c] /= (halfsize * 2 + 1);
                    }
                }
            }

            for (unsigned int x = 0; x < width; ++x){
                for (unsigned int y = 0; y < height; ++y){
                    T* pixel = tex->GetPixel(x, y);
                    for (unsigned char c = 0; c < channels; ++c){
                        pixel[c] = 0;
                        for (int Y = -halfsize; Y <= halfsize; ++Y){
                            pixel[c] += temp.GetPixel(x, y + Y)[c];
                        }
                        pixel[c] /= (halfsize * 2 + 1);
                    }
                }
            }
        }

        template <class T>
        void Empty(Texture2DPtr(T) t){
            t->Load();
            for (unsigned int x = 0; x < t->GetWidth(); ++x)
                for (unsigned int z = 0; z < t->GetWidth(); ++z)
                    for (unsigned int c = 0; c < t->GetChannels(); ++c)
                        t->GetPixel(x, z)[c] = 0;
        }

    }
}

#endif

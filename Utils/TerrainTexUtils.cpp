// Terrain Texture Utils.
// -------------------------------------------------------------------
// Copyright (C) 2010 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Utils/TerrainTexUtils.h>
#include <Logging/Logger.h>

namespace OpenEngine {
    namespace Utils {

        FloatTexture2DPtr ConvertTex(UCharTexture2DPtr tex){
            tex->Load();
            
            unsigned int width = tex->GetWidth();
            unsigned int height = tex->GetHeight();
            unsigned int channels = tex->GetChannels();

            logger.info << "w: " << width << ", h: " << height << ", c: " << channels << logger.end;

            FloatTexture2DPtr ret = FloatTexture2DPtr(new FloatTexture2D(width, height, channels));
            ret->Load();

            // Fill the data array
            for (unsigned int x = 0; x < width; ++x){
                for (unsigned int z = 0; z < height; ++z){
                    for (unsigned int c = 0; c < channels; ++c){
                        ret->GetPixel(x, z)[c] = tex->GetPixel(x, z)[c];
                    }   
                }
            }

            // Set other options
            ret->SetColorFormat(tex->GetColorFormat());
            ret->SetWrapping(tex->GetWrapping());
            ret->SetMipmapping(tex->UseMipmapping());

            return ret;
        }        

        UIntTexture2DPtr Combine(UCharTexture2DPtr t1,
                                 UCharTexture2DPtr t2,
                                 UCharTexture2DPtr t3, 
                                 UCharTexture2DPtr t4){
            // Check that the dimensions are the same. 

            //@TODO could be replaced by some sort of interpolation of
            // the smallest element.

            unsigned int width = t1->GetWidth();
            unsigned int height = t1->GetHeight();
            
            if (width != t2->GetWidth() ||
                height != t2->GetHeight()){
                logger.warning << "Trying to combine textures of different dimensions. Aborting." << logger.end;
                return UIntTexture2DPtr();
            }
            
            UIntTexture2DPtr ret = UIntTexture2DPtr(new UIntTexture2D(width, height, 2));

            for (unsigned int x = 0; x < width; ++x){
                for (unsigned int y = 0; y < height; ++y){
                    unsigned char* p1 = t1->GetPixel(x, y);
                    ret->GetPixel(x, y)[0] = p1[0];
                    for (int c = 1; c < t1->GetChannels(); ++c)
                        ret->GetPixel(x, y)[0] += p1[c] << (c*8);

                    if (t2 != NULL){
                        unsigned char* p1 = t1->GetPixel(x, y);
                        ret->GetPixel(x, y)[0] = p1[0];
                        for (int c = 1; c < t1->GetChannels(); ++c)
                            ret->GetPixel(x, y)[0] += p1[c] << (c*8);
                    }

                    if (t3 != NULL){
                        unsigned char* p1 = t1->GetPixel(x, y);
                        ret->GetPixel(x, y)[0] = p1[0];
                        for (int c = 1; c < t1->GetChannels(); ++c)
                            ret->GetPixel(x, y)[0] += p1[c] << (c*8);
                    }
                    
                    if (t4 != NULL){
                        unsigned char* p1 = t1->GetPixel(x, y);
                        ret->GetPixel(x, y)[0] = p1[0];
                        for (int c = 1; c < t1->GetChannels(); ++c)
                            ret->GetPixel(x, y)[0] += p1[c] << (c*8);
                    }
                }
            }       

            return ret;
        }

    }
}

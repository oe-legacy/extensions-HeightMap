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
    }
}

#endif

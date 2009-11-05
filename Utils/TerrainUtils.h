// Terrain util functions
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _TERRAIN_UTIL_FUNCTIONS_H_
#define _TERRAIN_UTIL_FUNCTIONS_H_

#include <Resources/ITextureResource.h>

using namespace OpenEngine::Resources;

namespace OpenEngine {
    namespace Scene {
        class HeightFieldNode;
    }
    namespace Utils {

        ITextureResourcePtr CreateHeightMap();
        ITextureResourcePtr CreateNormalMap(ITextureResourcePtr heightMap, float heightScale = 1.0, float widthScale = 1.0, bool padding = true);

        void SmoothTerrain(Scene::HeightFieldNode* heightfield, float persistence = 0.5);
        
    }
}

#endif

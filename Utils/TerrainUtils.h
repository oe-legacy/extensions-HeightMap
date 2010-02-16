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

#include <Resources/Texture2D.h>

using namespace OpenEngine::Resources;

namespace OpenEngine {
    namespace Scene {
        class HeightFieldNode;
    }
    namespace Utils {

        /**
         * Uses cosine to create a smooth and soothing terrain.
         */
        FloatTexture2DPtr CreateSmoothTerrain(FloatTexture2DPtr tex, 
                                              unsigned int steps = 1000, int radius = 10, float disp = 5);
        
        /**
         * Will make the heightmap into a plateu. 'Steals' the margin
         * of the pixels along the sides and uses them for the cliffs.
         *
         * If the transformation yields negative height, the terrain
         * is moved up.
         */
        void MakePlateau(UCharTexture2DPtr terrain, float height, unsigned int margin);

        /**
         * Smoothens terrain.
         */
        void SmoothTerrain(Scene::HeightFieldNode* heightfield, float persistence = 0.5);
        
    }
}

#endif

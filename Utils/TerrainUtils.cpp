// Terrain util functions
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Utils/TerrainUtils.h>

#include <Resources/EmptyTextureResource.h>
#include <Scene/HeightFieldNode.h>
#include <Scene/HeightFieldPatchNode.h>
#include <Math/Vector.h>
#include <Logging/Logger.h>

using namespace OpenEngine::Scene;
using namespace OpenEngine::Math;

namespace OpenEngine {
    namespace Utils {

        void SmoothTerrain(HeightFieldNode* heightfield, float persistence){
            int depth = heightfield->GetVerticeDepth();
            int width = heightfield->GetVerticeWidth();

            int numberOfVertices = depth * width;

            float temp[numberOfVertices];

            // Calculate the average values
            for (int x = 0; x < depth; ++x){
                for (int z = 0; z < width; ++z){
                    float y = heightfield->GetVertex(x, z)[1];
                    float averageY = 0;
                    for (int i = -1; i <= 1; ++i)
                        for (int j = -1; j <= 1; ++j)
                            averageY = heightfield->GetVertex(x + i, z + j)[1];

                    int indice = heightfield->GetIndice(x, z);
                    temp[indice] = persistence * y + (1 - persistence) * averageY;
                }
            }
            
            // Set the values
            for (int x = 0; x < depth; ++x){
                for (int z = 0; z < width; ++z){
                    float* vertex = heightfield->GetVertex(x, z);
                    int indice = heightfield->GetIndice(x, z);
                    vertex[1] = temp[indice];
                }
            }
        }
        
    }
}

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

        inline float HeightLookup(unsigned int x, unsigned int z, ITextureResourcePtr tex, float heightScale){
            int numberOfCharsPrColor = tex->GetDepth() / 8;
            unsigned char* data = tex->GetData();
            if (x < tex->GetWidth() && z < tex->GetHeight()){
                int index = z + x * tex->GetHeight() * numberOfCharsPrColor;
                return heightScale * data[index + numberOfCharsPrColor - 1] ;
            }else
                return 0.0f;
        }

        ITextureResourcePtr CreateHeightMap(){
            return ITextureResourcePtr();
        }

        ITextureResourcePtr CreateNormalMap(ITextureResourcePtr heightMap, float heightScale, float widthScale, bool padding){
            unsigned int mapWidth = heightMap->GetWidth();
            unsigned int mapHeight = heightMap->GetHeight();

            unsigned int width;
            unsigned int height;
            if (padding){
                int patchWidth = HeightFieldPatchNode::PATCH_EDGE_SQUARES;
                int widthRest = (mapWidth - 1) % patchWidth;
                width = widthRest ? mapWidth + 32 - widthRest : mapWidth;
                
                int heightRest = (mapHeight - 1) % patchWidth;
                height = heightRest ? mapHeight + patchWidth - heightRest : mapHeight;
            }else{
                width = mapWidth;
                height = mapHeight;
            }

            ITextureResourcePtr normalTex = ITextureResourcePtr(new EmptyTextureResource(width, height, 24));
            normalTex->Load();

            unsigned char* mapData = heightMap->GetData();
            unsigned char* normalData = normalTex->GetData();

#define YCoordLookup(x, z)                                              \
            ((x) < mapWidth && (z) < mapHeight) ? ((float)mapData[((z) + (x) * mapHeight) * 3 + 1]) * heightScale : 0.0f
            
            for (unsigned int x = 0; x < width; ++x){
                for (unsigned int z = 0; z < height; ++z) {
                    int index = (z + x * height) * 3;
                    int ns = 0;
                    float vHeight = YCoordLookup(x, z);
                    //float vHeight = HeightLookup(x, z, heightMap, heightScale);

                    Vector<3, float> normal = Vector<3, float>(0.0f);
                    
                    // Point to the right
                    if (x + 1 < width){
                        float wHeight = YCoordLookup(x+1,z);
                        //float wHeight = HeightLookup(x+1,z, heightMap, heightScale);
                        normal[0] += vHeight - wHeight;
                        ++ns;
                    }

                    // Point to the left
                    if (x > 1){
                        float wHeight = YCoordLookup(x-1,z);
                        //float wHeight = HeightLookup(x-1,z, heightMap);
                        normal[0] += wHeight - vHeight;
                        ++ns;
                    }

                    // Point above
                    if (z + 1 < height){
                        float wHeight = YCoordLookup(x,z+1);
                        //float wHeight = HeightLookup(x,z+1, heightMap);
                        normal[2] += vHeight - wHeight;
                        ++ns;
                    }

                    // Point below
                    if (z > 1){
                        float wHeight = YCoordLookup(x,z-1);
                        //float wHeight = HeightLookup(x,z-1, heightMap);
                        normal[2] += wHeight - vHeight;
                        ++ns;
                    }

                    normal[1] = widthScale * ns;

                    normal.Normalize();

                    normal = (normal + 1) * 0.5f;

                    normalData[index] = (unsigned char) (255 * normal[0]);
                    normalData[index+1] = (unsigned char) (255 * normal[1]);
                    normalData[index+2] = (unsigned char) (255 * normal[2]);
                }
            }

            return normalTex;
            
        }

        ITextureResourcePtr CreateNormalMap(Scene::HeightFieldNode* terrain){
            const int DIMENSIONS = 3;

            int depth = terrain->GetVerticeDepth();
            int width = terrain->GetVerticeWidth();
            float widthScale = terrain->GetWidthScale();
            
            ITextureResourcePtr normalTex = ITextureResourcePtr(new EmptyTextureResource(depth, width, 24));
            normalTex->Load();
            
            unsigned char* normalData = normalTex->GetData();
            
            for (int x = 0; x < depth; ++x){
                for (int z = 0; z < width; ++z) {

                    int index = (z + x * width) * DIMENSIONS;

                    float vHeight = terrain->GetVertex(x, z)[1];

                    Vector<3, float> normal = Vector<3, float>(0.0f);

                    // Point to the right
                    float wHeight = terrain->GetVertex(x+1, z)[1];
                    normal[0] += vHeight - wHeight;

                    // point to the left
                    wHeight = terrain->GetVertex(x-1, z)[1];
                    normal[0] += wHeight - vHeight;

                    // Point above
                    wHeight = terrain->GetVertex(x, z+1)[1];
                    normal[2] += vHeight - wHeight;

                    // Point below
                    wHeight = terrain->GetVertex(x, z-1)[1];
                    normal[2] += wHeight - vHeight;

                    normal[1] = 4 * widthScale;

                    normal.Normalize();

                    normal = (normal + 1) * 0.5f;

                    normalData[index] = (unsigned char) (255 * normal[0]);
                    normalData[index+1] = (unsigned char) (255 * normal[1]);
                    normalData[index+2] = (unsigned char) (255 * normal[2]);
                }
            }

            return normalTex;
        }

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

// Terrain util functions
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Utils/TerrainUtils.h>

#include <Scene/HeightMapNode.h>
#include <Scene/HeightMapPatchNode.h>
#include <Math/Vector.h>
#include <Math/RandomGenerator.h>
#include <Logging/Logger.h>

using namespace OpenEngine::Scene;
using namespace OpenEngine::Math;

namespace OpenEngine {
    namespace Utils {

        /**
         * http://www.lighthouse3d.com/opengl/terrain/index.php3?circles
         */
        FloatTexture2DPtr CreateSmoothTerrain(FloatTexture2DPtr tex, 
                                              unsigned int steps, int radius, float disp){
            tex->Load();

            int width = tex->GetWidth();
            int height = tex->GetHeight();

            RandomGenerator ran;
            ran.SeedWithTime();

            float d = disp / 2.0f;

            for (unsigned int i = 0; i < steps; ++i){
                // Pick a random point
                Vector<2, int> c = Vector<2, int>(ran.UniformInt(0, width), ran.UniformInt(0, height));

                //logger.info << "Iteration: " << i << " yielded centrum " << c.ToString() << logger.end;

                // Iterate through each point in the circle by
                // iterating through a square around the circle.
                int xStart = c[0] - radius < 0 ? 0 : c[0] - radius;
                int zStart = c[1] - radius < 0 ? 0 : c[1] - radius;
                int xEnd = c[0] + radius > width-1 ? width-1 : c[0] + radius;
                int zEnd = c[1] + radius > height-1 ? height-1 : c[1] + radius;

                for (int x = xStart; x <= xEnd; ++x){
                    for (int z = zStart; z <= zEnd; ++z){
                        Vector<2, int> p = Vector<2, int>(x, z);
                        float dist = (p - c).GetLength();
                        if (dist <= radius){
                            float pd = dist / radius;
                            float displacement = d + cos(pd*PI) * d;
                            tex->GetPixel(x, z)[0] += displacement;
                        }
                    }   
                }

            }
            
            return tex;
        }

        void MakePlateau(UCharTexture2DPtr terrain, float height, unsigned int margin){
            // Use normals for guessing good places to erode?

            // Use log and cosine to create the dropoff? Smoother drop :)
        }

        void SmoothTerrain(HeightMapNode* heightfield, float persistence){
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

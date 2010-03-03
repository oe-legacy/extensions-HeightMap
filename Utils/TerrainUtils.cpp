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

        FloatTexture2DPtr MakePlateau(FloatTexture2DPtr tex, float disp, unsigned int margin){
            tex->Load();
            
            int width = tex->GetWidth();
            int height = tex->GetHeight();

            float d = disp / 2.0f;
            // Use normals for guessing good places to erode?

            // Use log and cosine to create the dropoff? Smoother drop :)

            // Check margin is not higher than half height or width.

            // Move the cornors.

            for (unsigned int x = 0; x < margin; ++x)
                for (unsigned int z = 0; z < margin; ++z){
                    float pdz = (float) z / (2 * margin);
                    float displacementz = d + cos(pdz*2*PI) * d;

                    float pdx = (float) x / (2 * margin);
                    float displacementx = d + cos(pdx*2*PI) * d;
                    float displacement = std::max(displacementx, displacementz);

                    tex->GetPixel(x, z)[0] += displacement;

                    tex->GetPixel(x, height - z - 1)[0] += displacement;

                    tex->GetPixel(width - x - 1, height - z - 1)[0] += displacement;

                    tex->GetPixel(width - x - 1, z)[0] += displacement;
                }

            // Move the sides
            for (unsigned int x = margin; x < height - margin; ++x)
                for (unsigned int z = 0; z < margin; ++z){
                    float pd = (float) z / margin;
                    float displacement = d + cos(pd*PI) * d;
                    tex->GetPixel(x, z)[0] += displacement;

                    tex->GetPixel(x, height - z - 1)[0] += displacement;
                }

            for (unsigned int z = margin; z < width - margin; ++z)
                for (unsigned int x = 0; x < margin; ++x){
                    float pd = (float) x / (2 * margin);
                    float displacement = d + cos(pd*2*PI) * d;
                    tex->GetPixel(x, z)[0] += displacement;

                    tex->GetPixel(width - x - 1, z)[0] += displacement;
                }


            return tex;
        }

        FloatTexture2DPtr CreateBubble(FloatTexture2DPtr tex, 
                                       Vector<2, int> c,
                                       int radius, float disp){
            tex->Load();
            
            int width = tex->GetWidth();
            int height = tex->GetHeight();
            
            float d = disp / 2.0f;

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

            return tex;
        }

    }
}

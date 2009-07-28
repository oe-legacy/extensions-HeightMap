// Height map node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Scene/HeightMapNode.h>
#include <Resources/ITextureResource.h>
#include <Meta/OpenGL.h>
#include <Logging/Logger.h>

namespace OpenEngine {
    namespace Scene {
        
        HeightMapNode::HeightMapNode(ITextureResource* tex, float widthScale, float heightScale) {
            width = tex->GetWidth();
            breath = tex->GetHeight();
            sunCoords = new float[3];
            sunCoords[0] = sunCoords[2] = 0; sunCoords[1] = 400;
            lightDirection = new float[3];
            CalcLightDirection(sunCoords);
            this->widthScale = widthScale;
            unsigned int depth = tex->GetDepth();
            entries = width * breath;
            map = new HeightMapPoint[entries];

            int numberOfCharsPerColor = depth / 8;
            unsigned char* data = tex->GetData();

            unsigned int i = 0;
            while (i < entries * numberOfCharsPerColor){
                unsigned char r = data[i++];
                unsigned char g = data[i++];
                unsigned char b = data[i++];
                
                unsigned char h;
                if (numberOfCharsPerColor == 4)
                    h = data[i++];
                else
                    h = ((unsigned int)r + (unsigned int)g + (unsigned int)b) / 3;
                h *= heightScale;

                map[(i-1) / numberOfCharsPerColor] = HeightMapPoint(h, r, g, b);
            }

            for (unsigned int z = 0; z < breath; ++z)
                for (unsigned int x = 0; x < width; ++x){
                    map[x * width + z].x = x * widthScale;
                    map[x * width + z].z = z * widthScale;
                    CalcNormal(x, z);
                }
        }
        
        HeightMapNode::~HeightMapNode(){
            if (map)
                delete[] map;
        }

        void HeightMapNode::SetHeightScaling(float scale){
            for (unsigned int i = 0; i < entries; ++i){
                map[i].height *= scale;
            }
        }

        /**
         * Calculates the normal of of the entry'eth height map point
         * by averaging over the surrounding surface normals.
         */
        void HeightMapNode::CalcNormal(unsigned int x, unsigned int z){
            int normals = 0;
            int entry = x * breath + z;
            HeightMapPoint p = map[entry];
            HeightMapPoint q;

            float normal[] = {0, 0, 0};

            // Calc "normal" to point above
            if (x + 1 < width){
                q = map[entry + breath];
                float dy = p.height - q.height;
                normal[0] += dy;
                ++normals;
            }

            // Calc "normal" to point below
            if (x > 1){
                q = map[entry - breath];
                float dy = q.height - p.height;
                normal[0] += dy;
                ++normals;
            }

            // point to the right
            if (z + 1 < breath){
                q = map[entry + 1];
                float dy = p.height - q.height;
                normal[2] += dy;
                ++normals;
            }

            // point to the left
            if (z > 1){
                q = map[entry - 1];
                float dy = q.height - p.height;
                normal[2] += dy;
                ++normals;
            }

            // Calc average
            map[entry].normal[0] = normal[0];
            map[entry].normal[1] = widthScale * normals;
            map[entry].normal[2] = normal[2];

            // normalize normal muahahaha I made a funny
            float norm = sqrt(map[entry].normal[0] * map[entry].normal[0]
                              + map[entry].normal[1] * map[entry].normal[1]
                              + map[entry].normal[2] * map[entry].normal[2]);
            
            map[entry].normal[0] /= norm;
            map[entry].normal[1] /= norm; 
            map[entry].normal[2] /= norm; 
        }

        /**
         * Shade vertices with normals facing away from the
         * sun. Faster than calcRayShading but not as snazzy looking.
         *
         * Uses the height maps sun as a lightsource.
         */
        void HeightMapNode::CalcSimpleShadow() {
            CalcSimpleShadow(sunCoords[0], sunCoords[1], sunCoords[2]);
        }
        /**
         * Shade vertices with normals facing away from the
         * sun. Faster than calcRayShading but not as snazzy looking.
         *
         * @param sun The coords of the lightsource casting light on
         * the heightmap.
         */
        void HeightMapNode::CalcSimpleShadow(float lightx, float lighty, float lightz){
            CalcLightDirection(lightx, lighty, lightz);
            for (unsigned int i = 0; i < entries; ++i){
                if (sunCoords[1] > - map[i].height){
                    float t = map[i].normal[0] * lightDirection[0]
                        + map[i].normal[1] * lightDirection[1]
                        + map[i].normal[2] * lightDirection[2];
                    
                    if (t < 0){
                        map[i].shadow = 0.05;
                    }else{
                        map[i].shadow = 1.0;
                    }
                }else{
                    map[i].shadow = 0.05;
                }
            }
        }

        /**
         * Shade vertices that isn't being lit up by the "sun".
         */
        void HeightMapNode::CalcRayTracedShadow(){
            CalcRayTracedShadow(sunCoords[0], sunCoords[1], sunCoords[2]);
        }
        void HeightMapNode::CalcRayTracedShadow(float lightx, float lighty, float lightz){
            CalcLightDirection(lightx, lighty, lightz);
            
        }

        /**
         * Applies the latest calculated shadows to the height maps
         * vertices.
         */
        void HeightMapNode::ApplyShadow(){
            for (unsigned int i = 0; i < entries; ++i)
                ApplyShadow(i);
        }
        /**
         * Applies the latest calculated shadows to the entry'eth node
         * in the height map.
         *
         * @param entry The entry'eth node
         */
        inline void HeightMapNode::ApplyShadow(unsigned int entry){
            float shadow = map[entry].shadow;
            map[entry].shadedColor[0] = map[entry].color[0] * shadow;
            map[entry].shadedColor[1] = map[entry].color[1] * shadow;
            map[entry].shadedColor[2] = map[entry].color[2] * shadow;
        }

        /**
         * Box Blur the shadows.
         */
        void HeightMapNode::BoxBlurShadow(unsigned int boxWidth, unsigned int boxBreath){
            float precomp[entries];
            
            // Fill the precomputed array, first the side, then the middle
            precomp[0] = map[0].shadow;
            for (unsigned int z = 1; z < breath; ++z){
                precomp[z] = map[z].shadow + precomp[z - 1];
            }
            for (unsigned int x = 1; x < breath; ++x){
                int i = x * width;
                precomp[i] = map[i].shadow + precomp[i - width];
            }
            for (unsigned int x = 1; x < breath; ++x){
                for (unsigned int z = 1; z < breath; ++z){
                    int i = z + x * width;
                    precomp[i] = map[i].shadow + precomp[i - 1] + precomp[i - width] - precomp[i - 1 - width];
                }
            }

            for (unsigned int z = 0; z < breath; ++z){
                for (unsigned int x = 0; x < width; ++x){
                    int startx = (x > boxWidth + 1) ? x - boxWidth - 1 : 0;
                    int endx = (x + boxWidth < width) ? x + boxWidth : width - 1;
                    int startz = (z > boxBreath + 1) ? z - boxBreath - 1 : 0;
                    int endz = (z + boxBreath < breath) ? z + boxBreath : breath - 1;
                    
                    float accShadow = precomp[endz + endx * width] 
                        + precomp[startz + startx * width]
                        - precomp[endz + startx * width]
                        - precomp[startz + endx * width];

                    map[z + x * width].shadow = accShadow / ((endx - startx) * (endz - startz));
                }
            }
        }

        /**
         * Color the vertices in the heightmap depending on their
         * height. Also (re)applies any shadow that has been
         * calculated.
         */
        void HeightMapNode::ColorFromHeight(){
            for (unsigned int i = 0; i < entries; ++i){
                ColorFromHeight(i);
            }
        }
        inline void HeightMapNode::ColorFromHeight(unsigned int entry){
            if (map[entry].height < WATERLEVEL){
                // Draw sand
                map[entry].color[0] = 255;
                map[entry].color[1] = 200;
                map[entry].color[2] = 150;
            }else if (false){ // cliff
                map[entry].color[0] = 100;
                map[entry].color[1] = 100;
                map[entry].color[2] = 100;
            }else if (map[entry].height < SNOWLEVEL){
                map[entry].color[0] = 0;
                map[entry].color[1] = 155;
                map[entry].color[2] = 0;
            }else{
                // Snowlevel
                map[entry].color[0] = 255;
                map[entry].color[1] = 255;
                map[entry].color[2] = 230;
            }
            
            ApplyShadow(entry);
        }
        
        /**
         * Calculates direction from the light source (sun) to the
         * heightmap.
         *
         * @param lightCoord The coordinates of the lightsource.
         */
        inline void HeightMapNode::CalcLightDirection(float* lightCoord){
            CalcLightDirection(lightCoord[0], lightCoord[1], lightCoord[2]);
        }

        inline void HeightMapNode::CalcLightDirection(float lightx, float lighty, float lightz){
            lightDirection[0] = lightx - breath / 2;
            lightDirection[1] = lighty - WATERLEVEL;
            lightDirection[2] = lightz - width / 2;
        }
    }
}

// Heightmap node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _HEIGHT_MAP_NODE_H_
#define _HEIGHT_MAP_NODE_H_

#include <Scene/ISceneNode.h>
#include <Resources/ITextureResource.h>
#include <Math/Vector.h>

using namespace OpenEngine::Resources;
using namespace OpenEngine::Math;

namespace OpenEngine {
    namespace Scene {

        struct HeightMapPoint {
            float x, height, z;
            unsigned char* color;
            float* normal;
            float shadow;
            unsigned char* shadedColor;
            HeightMapPoint() {init(0,0,0,0);}
            HeightMapPoint(int h, unsigned char r, unsigned char g, unsigned char b){
                init(h, r, g, b);
            }
            void init(int h, unsigned char r, unsigned char g, unsigned char b){
                height = h;
                color = new unsigned char[3];
                shadedColor = new unsigned char[3];
                shadedColor[0] = color[0] = r;
                shadedColor[1] = color[1] = g;
                shadedColor[2] = color[2] = b;
                shadow = 1;
                normal = new float[3];
                normal[0] = 0; normal[1] = 1; normal[2] = 0;
            }
        };

        /**
         * Height Map class for generating heightmap.
         *
         * Indexing into the heightmap is as follows.
         *
         * (breath,0) ___ (breath, width)
         *          | | |
         *          |-+-|
         *          |_|_|
         *     (0,0)     (0, width)
         *
         * @class HeightMapNode HeightMapNode.h Scene/HeightMapNode.h 
         */
        class HeightMapNode : public ISceneNode {
            OE_SCENE_NODE(HeightMapNode, ISceneNode)
            
        private:
            static const unsigned int WATERLEVEL = 20;
            static const unsigned int SNOWLEVEL = 70;

            unsigned int width;
            unsigned int breath;
            unsigned int entries;
            float widthScale;
            // Array of size width * breath containing the points
            HeightMapPoint* map;

            float* sunCoords;
            float* lightDirection;

        public:
            HeightMapNode() : width(0), breath(0), map(0){}
            // Create a heightmap from a texture
            HeightMapNode(ITextureResource* tex, float widthScale, float heightScale);
            ~HeightMapNode();

            unsigned int GetWidth() { return width; }
            unsigned int GetDepth() { return breath; }
            HeightMapPoint* GetHeightMap() { return map; }
            float* GetSunPos(){ return sunCoords; }

            void SetHeightScaling(float scale);
            void SetSunCoords(float* coords) {sunCoords = coords;}

            void CalcSimpleShadow();
            void CalcSimpleShadow(float lightx, float lighty, float lightz);
            void CalcRayTracedShadow();
            void CalcRayTracedShadow(float lightx, float lighty, float lightz);
            void ApplyShadow();
            void BoxBlurShadow(unsigned int boxWidth, unsigned int boxHeight);

            void ColorFromHeight();

            void VisitSubNodes(ISceneNodeVisitor& visitor) {};

        private:
            void CalcNormal(unsigned int x, unsigned int z);
            void ApplyShadow(unsigned int entry);
            void ColorFromHeight(unsigned int entry);
            void CalcLightDirection(float* lightCoord);
            void CalcLightDirection(float lightx, float lighty, float lightz);
        };

    } // ns Scene
} // ns OpenEngine

#endif _HEIGHT_MAP_NODE_H_

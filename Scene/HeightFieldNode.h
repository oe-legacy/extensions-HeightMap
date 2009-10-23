// Heightfield node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _HEIGHTFIELD_NODE_H_
#define _HEIGHTFIELD_NODE_H_

#include <Scene/ISceneNode.h>
#include <Core/IListener.h>
#include <Renderers/IRenderer.h>
#include <Resources/IShaderResource.h>
#include <Resources/ITextureResource.h>

using namespace OpenEngine::Core;
using namespace OpenEngine::Renderers;
using namespace OpenEngine::Resources;

namespace OpenEngine {
    namespace Scene {
        class SunNode;

        /**
         * A class for creating landscapes through heightmaps
         *
         * The landscape starts in (0, 0) with it's lower left cornor
         * and has it's depth along the x-axis and width along the
         * z-axis.
         *
         *   X
         * D A
         * E |
         * P |
         * T |
         * H |
         *  -+----->Z
         *    WIDTH
         */
        class HeightFieldNode : public ISceneNode, public IListener<RenderingEventArg> {
            OE_SCENE_NODE(HeightFieldNode, ISceneNode)

        public:
            static const int DIMENSIONS = 4;
            static const int TEXCOORDS = 2;

        private:
            int* map; // mapping (x, z) coords to the index where the information is located
            
            int numberOfVertices;

            float* vertices;
            unsigned int verticeBOID;

            float* texCoords;
            unsigned int texCoordBOID;

            float* normalMapCoords;
            unsigned int normalMapCoordBOID;

            float* geoMorphScaleCoords;
            unsigned int geoMorphScaleCoordOffset;

            float* verticeLOD;
            unsigned int verticeLODOffset;
            
            unsigned int numberOfIndices;
            unsigned int indiceId;
            unsigned int* indices;

            int width;
            int depth;
            int entries;
            float widthScale;
            float heightScale;
            float waterlevel;

            SunNode* sun;

            float texDetail;

            // Distances for changing the LOD
            float baseDistance;
            float incrementalDistance;
            float* lodDistanceSquared;

            ITextureResourcePtr tex;
            ITextureResourcePtr normalmap;
            IShaderResourcePtr landscapeShader;

        public:
            HeightFieldNode() {}
            HeightFieldNode(ITextureResourcePtr tex);
            ~HeightFieldNode() {}

            void Load();

            void Render();

            void VisitSubNodes(ISceneNodeVisitor& visitor);

            void Handle(RenderingEventArg arg);

            // *** Get/Set methods ***

            unsigned int GetVerticeBufferID() const { return verticeBOID; }
            unsigned int GetTexCoordBufferID() const { return texCoordBOID; }
            unsigned int GetNormalMapCoordBufferID() const { return normalMapCoordBOID; }
            unsigned int GetIndiceID() const { return indiceId; }
            unsigned int GetNumberOfIndices() const { return numberOfIndices; }
            int GetNumberOfVertices() const { return numberOfVertices; }

            void SetHeightScale(const float scale) { heightScale = scale; }
            void SetWidthScale(const float scale) { widthScale = scale; }
            int GetWidth() const { return width * widthScale; }
            int GetDepth() const { return depth * widthScale; }

            SunNode* GetSun() const { return sun; }
            void SetSun(SunNode* s) { sun = s; }

            void SetTextureDetail(float detail);

            void SetLandscapeShader(IShaderResourcePtr shader) { landscapeShader = shader; }
            IShaderResourcePtr GetLandscapeShader() const { return landscapeShader; }

        protected:
            inline void InitArrays();
            inline void SetupNormalMap();
            inline void SetupTerrainTexture();
            inline void CalcTexCoords(int x, int z);
            inline void ComputeIndices();

            inline int CoordToIndex(int x, int z) const;
            inline float* GetVertice(int x, int z) const;
            inline float* GetTexCoord(int x, int z) const;
            inline float* GetNormalMapCoord(int x, int z) const;
            inline float YCoord(int x, int z) const;

        };
    }
} 

#endif

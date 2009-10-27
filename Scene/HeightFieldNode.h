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
#include <Scene/HeightFieldPatchNode.h>
#include <Core/IListener.h>
#include <Renderers/IRenderer.h>
#include <Resources/IShaderResource.h>
#include <Resources/ITextureResource.h>

using namespace OpenEngine::Core;
using namespace OpenEngine::Renderers;
using namespace OpenEngine::Resources;

namespace OpenEngine {
    namespace Display {
        class IViewingVolume;
    }
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
            unsigned int verticeBufferId;

            float* texCoords;
            unsigned int texCoordBufferId;

            float* normalMapCoords;
            unsigned int normalMapCoordBufferId;

            float* geoMorphScaleCoords;
            unsigned int geoMorphScaleCoordBufferId;

            float* verticeLOD;
            unsigned int verticeLODBufferId;
            
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

            // Patch variables
            int patchGridWidth, patchGridDepth, numberOfPatches;
            HeightFieldPatchNode** patchNodes;

            // Distances for changing the LOD
            float baseDistance;
            float incrementalDistance;

            ITextureResourcePtr tex;
            ITextureResourcePtr normalmap;
            IShaderResourcePtr landscapeShader;

        public:
            HeightFieldNode() {}
            HeightFieldNode(ITextureResourcePtr tex);
            ~HeightFieldNode() {}

            void Load();

            void CalcLOD(Display::IViewingVolume* view);
            void Render();

            void VisitSubNodes(ISceneNodeVisitor& visitor);

            void Handle(RenderingEventArg arg);

            // *** Get/Set methods ***

            unsigned int GetVerticeBufferID() const { return verticeBufferId; }
            unsigned int GetTexCoordBufferID() const { return texCoordBufferId; }
            unsigned int GetNormalMapCoordBufferID() const { return normalMapCoordBufferId; }
            unsigned int GetVerticeLODBufferID() const { return verticeLODBufferId; }
            unsigned int GetIndiceID() const { return indiceId; }
            unsigned int GetNumberOfIndices() const { return numberOfIndices; }
            int GetNumberOfVertices() const { return numberOfVertices; }
            int GetIndice(int x, int z);
            float* GetVertex(int x, int z);

            void SetHeightScale(const float scale) { heightScale = scale; }
            void SetWidthScale(const float scale) { widthScale = scale; }
            int GetWidth() const { return width * widthScale; }
            int GetDepth() const { return depth * widthScale; }

            void SetLODSwitchDistance(const float base, const float inc);
            float GetLODBaseDistance() const { return baseDistance; }
            float GetLODIncDistance() const { return incrementalDistance; }

            SunNode* GetSun() const { return sun; }
            void SetSun(SunNode* s) { sun = s; }

            void SetTextureDetail(const float detail);
            void SetLandscapeShader(IShaderResourcePtr shader) { landscapeShader = shader; }
            IShaderResourcePtr GetLandscapeShader() const { return landscapeShader; }

        protected:
            inline void InitArrays();
            inline void SetupNormalMap();
            inline void SetupTerrainTexture();
            inline void CalcTexCoords(int x, int z);
            inline void CalcVerticeLOD();
            inline void CalcGeomorphHeight(int x, int z);
            inline void ComputeIndices();
            inline void SetupPatches();

            inline int CoordToIndex(int x, int z) const;
            inline float* GetVertice(int x, int z) const;
            inline float* GetTexCoord(int x, int z) const;
            inline float* GetNormalMapCoord(int x, int z) const;
            inline float* GetVerticeLOD(int x, int z) const;
        };
    }
} 

#endif

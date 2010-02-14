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
    namespace Display {
        class IViewingVolume;
    }
    namespace Scene {
        class SunNode;
        class HeightFieldPatchNode;

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
            int numberOfVertices;

            float* vertices;
            unsigned int verticeBufferId;

            float* normals;
            unsigned int normalsBufferId;

            float* texCoords;
            unsigned int texCoordBufferId;

            float* normalMapCoords;
            unsigned int normalMapCoordBufferId;

            float* geomorphValues; // {PatchCenterX, PatchCenterZ, LOD}
            unsigned int geomorphBufferId;

            char* deltaValues;

            //char

            unsigned int numberOfIndices;
            unsigned int indiceId;
            unsigned int* indices;

            int width;
            int depth;
            int entries;
            float widthScale;
            float heightScale;
            float waterlevel;
            Vector<3, float> offset;

            SunNode* sun;

            float texDetail;

            // Patch variables
            int patchGridWidth, patchGridDepth, numberOfPatches;
            HeightFieldPatchNode** patchNodes;

            // Distances for changing the LOD
            float baseDistance;
            float invIncDistance;

            ITextureResourcePtr tex;
            ITextureResourcePtr normalmap;
            IShaderResourcePtr landscapeShader;

        public:
            HeightFieldNode() {}
            HeightFieldNode(ITextureResourcePtr tex);
            ~HeightFieldNode();

            void Load();

            void CalcLOD(Display::IViewingVolume* view);
            void Render(Display::IViewingVolume* view);
            void RenderBoundingGeometry();

            void VisitSubNodes(ISceneNodeVisitor& visitor);

            void Handle(RenderingEventArg arg);

            // *** Get/Set methods ***

            float GetHeight(Vector<3, float> point);
            float GetHeight(float x, float z);

            unsigned int GetVerticeBufferID() const { return verticeBufferId; }
            ITextureResourcePtr GetNormalMapID() const { return normalmap; }
            unsigned int GetGeomorphBufferID() const { return geomorphBufferId; }
            unsigned int GetTexCoordBufferID() const { return texCoordBufferId; }
            unsigned int GetNormalMapCoordBufferID() const { return normalMapCoordBufferId; }
            unsigned int GetIndiceID() const { return indiceId; }
            unsigned int GetNumberOfIndices() const { return numberOfIndices; }
            int GetNumberOfVertices() const { return numberOfVertices; }

            int GetIndice(int x, int z);
            float* GetVertex(int x, int z);
            void SetVertex(int x, int z, float value);
            void SetVertices(int x, int z, int width, int depth, float* values);
            Vector<3, float> GetNormal(int x, int z);

            void SetHeightScale(const float scale) { heightScale = scale; }
            void SetWidthScale(const float scale) { widthScale = scale; }
            int GetWidth() const { return width * widthScale; }
            int GetDepth() const { return depth * widthScale; }
            int GetVerticeWidth() const { return width; }
            int GetVerticeDepth() const { return depth; }
            float GetWidthScale() const { return widthScale; }
            void SetOffset(Vector<3, float> o) { offset = o; }
            Vector<3, float> GetOffset() const { return offset; }

            void SetLODSwitchDistance(const float base, const float inc);
            float GetLODBaseDistance() const { return baseDistance; }
            float GetLODIncDistance() const { return 1.0f / invIncDistance; }
            float GetLODInverseIncDistance() const { return invIncDistance; }

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
            inline float CalcGeomorphHeight(int x, int z);
            inline void ComputeIndices();
            inline void SetupPatches();

            inline int CoordToIndex(const int x, const int z) const;
            inline float* GetVertice(const int x, const int z) const;
            inline float* GetVertice(const int index) const;
            inline float* GetNormals(const int x, const int z) const;
            inline float* GetNormals(const int index) const;
            inline float* GetTexCoord(const int x, const int z) const;
            inline float* GetNormalMapCoord(const int x, const int z) const;
            inline float* GetGeomorphValues(const int x, const int z) const;
            inline float& GetVerticeLOD(const int x, const int z) const;
            inline float& GetVerticeLOD(const int index) const;
            inline char& GetVerticeDelta(const int x, const int z) const;
            inline char& GetVerticeDelta(const int index) const;
            inline int GetPatchIndex(const int x, const int z) const;
            inline HeightFieldPatchNode* GetPatch(const int x, const int z) const;
        };
    }
} 

#endif

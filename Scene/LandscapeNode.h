// Landscape node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _LANDSCAPE_NODE_H_
#define _LANDSCAPE_NODE_H_

#include <Display/IViewingVolume.h>
#include <Scene/ISceneNode.h>
#include <Core/IListener.h>
#include <Renderers/IRenderer.h>
#include <Scene/LandscapePatchNode.h>
#include <Resources/IShaderResource.h>
#include <Resources/ITextureResource.h>
#include <Meta/OpenGL.h>

using namespace OpenEngine::Core;
using namespace OpenEngine::Renderers;
using namespace OpenEngine::Resources;
using namespace OpenEngine::Display;

namespace OpenEngine {
    namespace Scene {
        
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
        class LandscapeNode : public ISceneNode, public IListener<RenderingEventArg> {
            OE_SCENE_NODE(LandscapeNode, ISceneNode)
            
        private:
            static const int DIMENSIONS = 3;
            static const int TEXCOORDS = 2;
            static const int WATERLEVEL = 10;

            GLfloat* vertices;
            GLubyte* colors;
            GLfloat* normals;
            GLfloat* texCoords;
            int * verticeLOD;
            GLfloat* originalValues; // {normal, height}
            GLfloat* morphedValues;

            LandscapePatchNode* patchNodes;

            int width;
            int depth;
            int patchGridWidth;
            int patchGridDepth;
            int numberOfVertices;
            int numberOfPatches;
            int entries;
            float widthScale;

            float* sunPos;

            int texDetail;
            // Distances for changing the LOD
            float baseDistance;
            float incrementalDistance;
            float* lodDistanceSquared;

            IShaderResourcePtr landscapeShader;

        public:
            LandscapeNode() {};
            LandscapeNode(ITextureResourcePtr tex, IShaderResourcePtr shader = IShaderResourcePtr(), float heightscale = 1.0, float widthScale = 1.0);
            ~LandscapeNode();

            void CalcLOD(IViewingVolume* view);
            void RenderPatches();
            void RenderNormals();

            void VisitSubNodes(ISceneNodeVisitor& visitor);

            void Handle(RenderingEventArg arg);

            // *** Get/Set methods ***

            GLfloat* GetVerticeArray() const { return vertices; }
            GLubyte* GetColorArray() const { return colors; }
            GLfloat* GetNormalArray() const { return normals; }
            GLfloat* GetTextureCoordArray() const { return texCoords; }
            IShaderResourcePtr GetLandscapeShader() const { return landscapeShader; }
            int GetNumberOfVertices() const { return numberOfVertices; }
            int GetWidth() const { return width; }
            int GetDepth() const { return depth; }

            void GetCoords(int index, float &x, float &y, float &z) const;
            void GetYCoord(int index, float &y) const;
            void GeoMorphCoord(int x, int z, int LOD, float scale);

            float* GetSunPos() const { return sunPos; }
            void SetSunPos(float* sun) { sunPos = sun; }
            
            void SetTextureDetail(int pixelsPrEdge);
            void SetLODSwitchDistance(const float base, const float inc);
            float* GetLODSwitchArray() const { return lodDistanceSquared; }
            float GetLODBaseDistance() const { return baseDistance; }
            float GetLODBaseIncDistance() const { return incrementalDistance; }
            
            void SetLandscapeShader(IShaderResourcePtr shader) { landscapeShader = shader; }

        private:
            inline void CalcNormal(int x, int z);
            inline void SetupTerrainTexture();
            inline void CalcTexCoords(int x, int z);
            inline void SetupGeoMorphing();
            inline void CalcGeoMorphing(int x, int z);
            inline void CalcLODSwitchDistances();
            inline void EntryToCoord(int entry, int &x, int &z) const;
            inline int CoordToEntry(int x, int z) const;
            inline float XCoord(int x, int z) const;
            inline float YCoord(int x, int z) const;
            inline float ZCoord(int x, int z) const;
            inline int LODLevel(int x, int z) const;
            inline void SetYCoord(const int x, const int z, float value);
        };
        
    }
}

#endif

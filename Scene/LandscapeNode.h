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
#include <Scene/LandscapePatchNode.h>
#include <Resources/GLSLResource.h>
#include <Resources/ITextureResource.h>
#include <Meta/OpenGL.h>

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
        class LandscapeNode : public ISceneNode {
            OE_SCENE_NODE(LandscapeNode, ISceneNode)
            
        private:
            static const int DIMENSIONS = 3;
            static const int TEXCOORDS = 2;

            bool initialized;

            GLfloat* vertices;
            GLubyte* colors;
            GLfloat* normals;
            GLfloat* texCoords;

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

            // Shader for geomorphing
            IShaderResourcePtr landscapeShader;

        public:
            LandscapeNode() {};
            LandscapeNode(ITextureResourcePtr tex, IShaderResourcePtr shader, float heightscale = 1.0, float widthScale = 1.0);
            ~LandscapeNode();

            bool IsInitialized() const { return initialized; }
            void Initialize();

            void CalcLOD(IViewingVolume* view);
            void RenderPatches();
            void RenderNormals();

            void VisitSubNodes(ISceneNodeVisitor& visitor) {}

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

            float* GetSunPos() const { return sunPos; }
            void SetSunPos(float* sun) { sunPos = sun; }
            
            void SetTextureDetail(int pixelsPrEdge);
            void SetLandscapeShader(IShaderResourcePtr shader) { landscapeShader = shader; }

        private:
            inline void CalcNormal(int x, int z);
            inline void SetupTerrainTexture();
            inline void CalcTexCoords(int x, int z);
            inline void EntryToCoord(int entry, int &x, int &z) const;
            inline int CoordToEntry(int x, int z) const;
            inline float XCoord(int x, int z) const;
            inline float YCoord(int x, int z) const;
            inline float ZCoord(int x, int z) const;
        };
        
    }
}

#endif

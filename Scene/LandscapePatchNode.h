// Landscape patch node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _LANDSCAPE_PATCH_NODE_H_
#define _LANDSCAPE_PATCH_NODE_H_

#include <Display/IViewingVolume.h>
#include <Math/Vector.h>
#include <Meta/OpenGL.h>
#include <Scene/LandscapeNode.h>
#include <Scene/ISceneNode.h>

using namespace OpenEngine::Display;

namespace OpenEngine {
    namespace Scene {

        struct LODstruct {
            int LOD;

            GLuint* bodyIndices;
            int numberOfBodyIndices;

            int numberOfStichingHigherIndices;
            int numberOfStichingEqualIndices;
            int numberOfStichingLowerIndices;

            GLuint* rightStitchingHigherIndices; // The patch to the right has a higher LOD
            GLuint* rightStitchingEqualIndices; // The patch to the right has the same LOD
            GLuint* rightStitchingLowerIndices; // The patch to the right has a lower LOD

            GLuint* upperStitchingHigherIndices;
            GLuint* upperStitchingEqualIndices;
            GLuint* upperStitchingLowerIndices;
        };

        class LandscapePatchNode : public ISceneNode {
            OE_SCENE_NODE(LandscapePatchNode, ISceneNode)
            
        public:
            static const int PATCH_EDGE_SQUARES = 32;
            static const int PATCH_EDGE_VERTICES = PATCH_EDGE_SQUARES + 1;
            static const int MAX_LODS = 4;

        private:
            int LOD; // a LOD of 0 means the patch is outside the frustum
            float geoMorphingScale;
            int xStart, xEnd, zStart, zEnd;
            Box* boundingBox;
            Vector<3, float> patchCenter;
            LandscapeNode* landscape;
            int landscapeWidth;

            LODstruct* LODs;
            LODstruct* currentLOD;

            LandscapePatchNode* upper;
            int upperPatchLOD; 

            LandscapePatchNode* right;
            int rightPatchLOD;

        public:
            LandscapePatchNode() {}
            LandscapePatchNode(int xOffset, int zOffset, LandscapeNode* land);
            ~LandscapePatchNode();

            // Render functions
            void CalcLOD(IViewingVolume* view);
            void Render();
            void RenderNormals();

            void VisitSubNodes(ISceneNodeVisitor& visitor) {};

            // *** Get/Set methods ***

            void SetUpperNeighbor(LandscapePatchNode* u) {upper = u; }
            void SetRightNeighbor(LandscapePatchNode* r) {right = r; }
            float GetLOD() const { return LOD; }
            
        private:
            inline void ComputeIndices();
            inline void ComputeBodyIndices(LODstruct* lods);
            inline void ComputeStitchingIndices(LODstruct* lods);
            inline void ComputeRightStichingIndices(GLuint* indices, int LOD, int rightLOD);
            inline void ComputeUpperStichingIndices(GLuint* indices, int LOD, int upperLOD);

            inline void GeoMorph();
        };

    }
}

#endif

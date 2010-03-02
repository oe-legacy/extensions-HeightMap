// Heightfield patch node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _HEIGHTFIELD_PATCH_NODE_H_
#define _HEIGHTFIELD_PATCH_NODE_H_

#include <Scene/ISceneNode.h>
#include <Geometry/Box.h>

using namespace OpenEngine::Geometry;

namespace OpenEngine {
    namespace Resources {
        class IndexBufferObject;
        typedef boost::shared_ptr<IndexBufferObject> IndexBufferObjectPtr;
    }
    namespace Display{
        class IViewingVolume;
    }
    namespace Scene {
        class HeightMapNode;

        struct LODstruct {
            int numberOfIndices;
            unsigned int* indices;
            void* indiceBufferOffset;
        };
        
        class HeightMapPatchNode : public ISceneNode {
            OE_SCENE_NODE(HeightMapPatchNode, ISceneNode)

        public:
            static const int PATCH_EDGE_SQUARES = 64;
            static const int PATCH_EDGE_VERTICES = PATCH_EDGE_SQUARES + 1;
            static const int MAX_LODS = 4;
            static const int MAX_DELTA = 8; //pow(2, MAX_LODS-1);
            static const int MIN_LOD = 1;

            enum LODrelation { LOWER = 0, SAME = 1, HIGHER = 2 };
            
        private:
            HeightMapNode* terrain;

            unsigned int LOD;
            float geomorphingScale;
            bool visible;
            
            int xStart, zStart, xEnd, zEnd, xEndMinusOne, zEndMinusOne;
            Vector<3, float> patchCenter;
            Geometry::Box boundingBox;
            Vector<3, float> min, max;

            Resources::IndexBufferObjectPtr indexBuffer;
            LODstruct LODs[MAX_LODS][3][3];

            HeightMapPatchNode* upperNeighbour;
            int upperLOD;
            
            HeightMapPatchNode* rightNeighbour;
            int rightLOD;

        public:            
            HeightMapPatchNode() {}
            HeightMapPatchNode(int xStart, int zStart, HeightMapNode* t);
            ~HeightMapPatchNode();

            void UpdateBoundingGeometry();
            void UpdateBoundingGeometry(float height);

            // Render functions
            void CalcLOD(Display::IViewingVolume* view);
            void Render() const;
            void RenderBoundingGeometry() const;

            void VisitSubNodes(ISceneNodeVisitor& visitor) {};

            // *** Get/Set methods ***

            void SetIndexBuffer(IndexBufferObjectPtr i) { indexBuffer = i; }
            void SetUpperNeighbor(HeightMapPatchNode* u) {upperNeighbour = u; }
            void SetRightNeighbor(HeightMapPatchNode* r) {rightNeighbour = r; }
            int GetLOD() const { return LOD; }
            float GetGeomorphingScale() const { return geomorphingScale; }
            LODstruct& GetLodStruct(const int lod, const int rightlod, const int upperlod) { return (LODs[lod][rightlod][upperlod]); }
            Vector<3, float> GetCenter() const { return patchCenter; }

        protected:
            inline void ComputeIndices();
            inline unsigned int* ComputeBodyIndices(int& indices, int LOD);
            inline unsigned int* ComputeRightStichingIndices(int& indices, int LOD, LODrelation rightLOD);
            inline unsigned int* ComputeUpperStichingIndices(int& indices, int LOD, LODrelation upperLOD);

            inline void SetupBoundingBox();
            inline void UpdateBoundingBox();
        };
        
    }
}

#endif

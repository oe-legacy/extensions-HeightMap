// Heightfield patch.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _HEIGHTFIELD_PATCH_H_
#define _HEIGHTFIELD_PATCH_H_

#include <Geometry/Box.h>

using namespace OpenEngine::Geometry;

namespace OpenEngine {
    namespace Resources {
        class Indices;
        typedef boost::shared_ptr<Indices > IndicesPtr;
    }
    namespace Display{
        class IViewingVolume;
    }
    namespace Scene {
        class HeightMapNode;

        struct LODstruct {
            int numberOfIndices;
            unsigned int* indices;
            unsigned int indiceBufferOffset;
        };
        
        class HeightMapPatch {

        public:
            static const int PATCH_EDGE_SQUARES = 32;
            static const int PATCH_EDGE_VERTICES = PATCH_EDGE_SQUARES + 1;
            static const int MAX_LODS = 3;
            static const int MAX_DELTA = 4; //pow(2, MAX_LODS-1);

            enum LODrelation { LOWER = 0, SAME = 1, HIGHER = 2 };
            
        private:
            HeightMapNode* terrain;

            unsigned int LOD, upperLOD, rightLOD;
            float geomorphingScale, rightGeomorphingScale, upperGeomorphingScale;
            bool visible;
            
            int xStart, zStart, xEnd, zEnd, xEndMinusOne, zEndMinusOne;
            Vector<3, float> patchCenter;
            Geometry::Box boundingBox;
            Vector<3, float> min, max;
            float edgeLength;

            Resources::IndicesPtr indexBuffer;
            LODstruct LODs[MAX_LODS][3][3];
            
        public:            
            HeightMapPatch() {}
            HeightMapPatch(int xStart, int zStart, HeightMapNode* t);
            ~HeightMapPatch();

            void UpdateBoundingGeometry();
            void UpdateBoundingGeometry(float height);

            // Render functions
            void CalcLOD(Display::IViewingVolume* view);
            void Render() const;
            void RenderBoundingGeometry() const;

            // *** Get/Set methods ***

            void SetDataIndices(IndicesPtr i) { indexBuffer = i; }
            int GetLOD() const { return LOD; }
            inline bool IsVisible() const { return visible; }
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

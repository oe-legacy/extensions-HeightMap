// Height field patch.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Scene/HeightMapPatch.h>
#include <Scene/HeightMapNode.h>
#include <Meta/OpenGL.h>
#include <Display/IViewingVolume.h>
#include <Logging/Logger.h>
#include <math.h>
#include <Resources/DataBlock.h>

#include <cstring>

using namespace OpenEngine::Display;
using namespace OpenEngine::Resources;

namespace OpenEngine {
    namespace Scene {
        
        HeightMapPatch::HeightMapPatch(int xStart, int zStart, HeightMapNode* t)
            : terrain(t), LOD(1), geomorphingScale(1), visible(false), 
              xStart(xStart), zStart(zStart) {

            xEnd = xStart + PATCH_EDGE_VERTICES;
            zEnd = zStart + PATCH_EDGE_VERTICES;
            xEndMinusOne = xEnd - 1;
            zEndMinusOne = zEnd - 1;

            edgeLength = (xEndMinusOne - xStart) * t->GetWidthScale();

            ComputeIndices();
            
            SetupBoundingBox();
        }

        HeightMapPatch::~HeightMapPatch(){
            //delete [] LODs;
        }

        void HeightMapPatch::UpdateBoundingGeometry(){
            for (int x = xStart; x < xEnd; ++x){
                for (int z = zStart; z < zEnd; ++z){
                    float y = terrain->GetVertex(x, z)[1];
                    min[1] = y < min[1] ? y : min[1];
                    max[1] = y > max[1] ? y : max[1];
                }
            }

            UpdateBoundingBox();
        }

        void HeightMapPatch::UpdateBoundingGeometry(float h){
            if (h > max[1]){
                // The vertex is above the box and the box should be
                // updated to the new height.
                max[1] = h;
            }else if (h < max[1]){
                // Otherwise if the height is below the box check if
                // the roof of the box is still supported.
                float tempHeight = h;
                bool roofSupport = false;
                for (int x = xStart; x < xEnd && !roofSupport; ++x){
                    for (int z = zStart; z < zEnd && !roofSupport; ++z){
                        float y = terrain->GetVertex(x, z)[1];
                        roofSupport = y == max[1];
                        if (y > tempHeight)
                            tempHeight = y;
                    }
                }
                max[1] = tempHeight;
            }

            if (h < min[1]){
                min[1] = h;
            }else if(h > min[1]){
                float tempHeight = h;
                bool floorSupport = false;
                for (int x = xStart; x < xEnd && !floorSupport; ++x){
                    for (int z = zStart; z < zEnd && !floorSupport; ++z){
                        float y = terrain->GetVertex(x, z)[1];
                        floorSupport = y == min[1];
                        if (y < tempHeight)
                            tempHeight = y;
                    }
                }
                min[1] = tempHeight;
            }

            UpdateBoundingBox();
        }
        
        void HeightMapPatch::CalcLOD(IViewingVolume* view){
            visible = view->IsVisible(boundingBox);
            if (!visible) return;

            Vector<3, float> viewPos = view->GetPosition();
            float baseDistance = terrain->GetLODBaseDistance();
            float invIncDistance = terrain->GetLODInverseIncDistance();

            // Calculate own LOD
            float distance = (viewPos - patchCenter).GetLength();
            distance -= baseDistance;

            geomorphingScale = distance * invIncDistance;

            if (geomorphingScale < 1)
                geomorphingScale = 1;
            else if (geomorphingScale > MAX_LODS)
                geomorphingScale = MAX_LODS;

            LOD = floor(geomorphingScale) - 1;

            // Calculate upper LOD
            distance = (viewPos - (patchCenter + Vector<3, float>(edgeLength, 0, 0))).GetLength();
            distance -= baseDistance;

            upperGeomorphingScale = distance * invIncDistance;
            if (upperGeomorphingScale < 1)
                upperGeomorphingScale = 1;
            else if (upperGeomorphingScale > MAX_LODS)
                upperGeomorphingScale = MAX_LODS;

            upperLOD = floor(upperGeomorphingScale) - 1;

            // Calculate right LOD
            distance = (viewPos - (patchCenter + Vector<3, float>(0, 0, edgeLength))).GetLength();
            distance -= baseDistance;

            rightGeomorphingScale = distance * invIncDistance;
            if (rightGeomorphingScale < 1)
                rightGeomorphingScale = 1;
            else if (rightGeomorphingScale > MAX_LODS)
                rightGeomorphingScale = MAX_LODS;

            rightLOD = floor(rightGeomorphingScale) - 1;
        }

        void HeightMapPatch::Render() const{
            if (visible){
                int rightLODdiff = rightLOD - LOD + 1;
                int upperLODdiff = upperLOD - LOD + 1;

                unsigned int numberOfIndices = LODs[LOD][rightLODdiff][upperLODdiff].numberOfIndices;
                unsigned int offset = LODs[LOD][rightLODdiff][upperLODdiff].indiceBufferOffset;
                if (indexBuffer->GetID() != 0)
                    glDrawElements(GL_TRIANGLE_STRIP, numberOfIndices, GL_UNSIGNED_INT, (void*)(offset * sizeof(GLuint)));
                else
                    glDrawElements(GL_TRIANGLE_STRIP, numberOfIndices, GL_UNSIGNED_INT, indexBuffer->GetData() + offset);
            }
        }

        void HeightMapPatch::RenderBoundingGeometry() const{
            glBegin(GL_LINES);
            Vector<3, float> center = boundingBox.GetCenter();
            glColor3f(0, 1, 0);

            for (int i = 0; i < 8; ++i){
                Vector<3, float> ic = boundingBox.GetCorner(i);
                for (int j = i+1; j < 8; ++j){
                    Vector<3, float> jc = boundingBox.GetCorner(j);
                    glVertex3f(ic[0], ic[1], ic[2]);
                    glVertex3f(jc[0], jc[1], jc[2]);
                }
            }
            glEnd();
        }

        // **** inlined functions ****

        void HeightMapPatch::ComputeIndices(){

            for (int i = 0; i < MAX_LODS; ++i){
                int bodyIndices;
                unsigned int* body = ComputeBodyIndices(bodyIndices, i);
                
                for (int j = 0; j < 3; ++j){
                    int rightIndices;
                    unsigned int* right = ComputeRightStichingIndices(rightIndices, i, (LODrelation) j);

                    for (int k = 0; k < 3; ++k){
                        int upperIndices;
                        unsigned int* upper = ComputeUpperStichingIndices(upperIndices, i, (LODrelation) k);

                        if (rightIndices > 0 && upperIndices > 0){
                            LODs[i][j][k].numberOfIndices = bodyIndices + rightIndices + upperIndices + 4;
                            LODs[i][j][k].indices = new unsigned int[LODs[i][j][k].numberOfIndices];
                            
                            int c = 0; // a counter into the array of indices
                            // Copy the body of the lod
                            memcpy(LODs[i][j][k].indices, body, bodyIndices * sizeof(unsigned int));
                            c += bodyIndices;
                            
                            // Add indices to draw 2 degenerate triangles
                            LODs[i][j][k].indices[c++] = body[bodyIndices-1];
                            LODs[i][j][k].indices[c++] = right[0];
                            
                            // Copy the right stitching
                            memcpy(LODs[i][j][k].indices + c, right, rightIndices * sizeof(unsigned int));
                            c += rightIndices;
                            
                            // Add indices to draw 2 degenerate triangles
                            LODs[i][j][k].indices[c++] = right[rightIndices-1];
                            LODs[i][j][k].indices[c++] = upper[0];
                            
                            // Copy the upper stitching
                            memcpy(LODs[i][j][k].indices + c, upper, upperIndices * sizeof(unsigned int));
                        }else{
                            LODs[i][j][k].numberOfIndices = 0;
                            LODs[i][j][k].indices = NULL;
                        }
                        delete[] upper;
                    }

                    delete[] right;
                }

                delete[] body;
            }
        }
        
        unsigned int* HeightMapPatch::ComputeBodyIndices(int& indices, int LOD){
            int delta = pow(2, LOD);
            
            int xs = PATCH_EDGE_SQUARES / delta - 1;
            int zs = PATCH_EDGE_SQUARES / delta;
            indices = 2 * xs * zs + 2 * xs - 2;

            unsigned int* ret = new unsigned int[indices];

            int i = 0;
            for (int x = xStart; x < xEnd - 2 * delta; x += delta){
                for (int z = zEndMinusOne - delta; z >= zStart; z -= delta){
                    ret[i++] = terrain->GetIndice(x, z);
                    ret[i++] = terrain->GetIndice(x+delta, z);
                }
                if (x < xEnd - 3 * delta){
                    ret[i++] = terrain->GetIndice(x+delta, zStart);
                    ret[i++] = terrain->GetIndice(x+delta, zEndMinusOne - delta);
                }
            }

            return ret;
        }

        unsigned int* HeightMapPatch::ComputeRightStichingIndices(int& indices, int LOD, LODrelation rightLOD){
            int delta = pow(2, LOD);

            int i = 0;

            switch (rightLOD){
            case LOWER:
                {
                    if (delta > 1){
                        int rightDelta = delta / 2;
                        
                        indices = 4 * PATCH_EDGE_SQUARES / delta + 1;
                        unsigned int* ret = new unsigned int[indices];
                        
                        for (int x = xEndMinusOne - delta; x >= xStart; x -= delta){
                            ret[i++] = terrain->GetIndice(x + delta, zEndMinusOne);
                            ret[i++] = terrain->GetIndice(x, zEndMinusOne - delta);
                            ret[i++] = terrain->GetIndice(x + rightDelta, zEndMinusOne);
                            ret[i++] = terrain->GetIndice(x, zEndMinusOne - delta);
                        }
                        ret[i++] = terrain->GetIndice(xStart, zEndMinusOne);

                        return ret;
                    }else{
                        indices = 0;
                        return NULL;
                    }
                }
            case SAME:
                {
                    indices = 2 * PATCH_EDGE_SQUARES / delta + 1;
                    unsigned int* ret = new unsigned int[indices];

                    ret[i++] = terrain->GetIndice(xEndMinusOne, zEndMinusOne);
                    for (int x = xEndMinusOne - delta; x >= xStart; x -= delta){
                        ret[i++] = terrain->GetIndice(x, zEndMinusOne - delta);
                        ret[i++] = terrain->GetIndice(x, zEndMinusOne);
                    }

                    return ret;
                }
            default: // HIGHER
                {
                    indices = 2 * PATCH_EDGE_SQUARES / delta + 1;
                    unsigned int* ret = new unsigned int[indices];
                    
                    for (int x = xEndMinusOne - 2 * delta; x >= xStart; x -= 2 * delta){
                        ret[i++] = terrain->GetIndice(x + 2 * delta, zEndMinusOne);
                        ret[i++] = terrain->GetIndice(x + delta, zEndMinusOne - delta);
                        ret[i++] = terrain->GetIndice(x + 2 * delta, zEndMinusOne);
                        ret[i++] = terrain->GetIndice(x, zEndMinusOne - delta);
                    }
                    ret[i++] = terrain->GetIndice(xStart, zEnd - 1);

                    return ret;
                }
            }
        }

        unsigned int* HeightMapPatch::ComputeUpperStichingIndices(int& indices, int LOD, LODrelation upperLOD){
            int delta = pow(2, LOD);

            int i = 0;

            switch (upperLOD){
            case LOWER:
                {
                    if (delta > 1){
                        int upperDelta = delta / 2;
                        
                        indices = 4 * PATCH_EDGE_SQUARES / delta + 1;
                        unsigned int* ret = new unsigned int[indices];
                        
                        for (int z = zEndMinusOne - delta; z >= zStart; z -= delta){
                            ret[i++] = terrain->GetIndice(xEndMinusOne, z + delta);
                            ret[i++] = terrain->GetIndice(xEndMinusOne - delta, z);
                            ret[i++] = terrain->GetIndice(xEndMinusOne, z + upperDelta);
                            ret[i++] = terrain->GetIndice(xEndMinusOne - delta, z);
                        }
                        ret[i++] = terrain->GetIndice(xEndMinusOne, zStart);
                        
                        return ret;
                    }else{
                        indices = 0;
                        return NULL;
                    }

                }
            case SAME:
                {
                    indices = 2 * PATCH_EDGE_SQUARES / delta + 1;
                    unsigned int* ret = new unsigned int[indices];


                    ret[i++] = terrain->GetIndice(xEndMinusOne, zEndMinusOne);
                    for (int z = zEndMinusOne - delta; z >= zStart; z -= delta){
                        ret[i++] = terrain->GetIndice(xEndMinusOne - delta, z);
                        ret[i++] = terrain->GetIndice(xEndMinusOne, z);
                    }
                    return ret;
                }
            default: // HIGHER
                {
                    indices = 2 * PATCH_EDGE_SQUARES / delta + 1;
                    unsigned int* ret = new unsigned int[indices];
                    
                    for (int z = zEndMinusOne - 2 * delta; z >= zStart; z -= 2 * delta){
                        ret[i++] = terrain->GetIndice(xEndMinusOne, z + 2 * delta);
                        ret[i++] = terrain->GetIndice(xEndMinusOne - delta, z + delta);
                        ret[i++] = terrain->GetIndice(xEndMinusOne, z + 2 * delta);
                        ret[i++] = terrain->GetIndice(xEndMinusOne - delta, z);
                    }
                    ret[i++] = terrain->GetIndice(xEnd - 1, zStart);

                    return ret;
                }
            }
        }

        void HeightMapPatch::SetupBoundingBox(){
            min = Vector<3, float>(terrain->GetVertex(xStart, zStart));
            max = Vector<3, float>(terrain->GetVertex(xEnd-1, zEnd-1));

            for (int x = xStart; x < xEnd; ++x){
                for (int z = zStart; z < zEnd; ++z){
                    float y = terrain->GetVertex(x, z)[1];
                    min[1] = y < min[1] ? y : min[1];
                    max[1] = y > max[1] ? y : max[1];
                }
            }

            UpdateBoundingBox();
        }

        void HeightMapPatch::UpdateBoundingBox(){
            patchCenter = (min + max) / 2;
            boundingBox = Box(patchCenter, max - patchCenter);
            
            patchCenter[1] = 0;
        }

    }
}

// Height field patch node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Scene/HeightFieldPatchNode.h>
#include <Scene/HeightFieldNode.h>
#include <Meta/OpenGL.h>
#include <Display/IViewingVolume.h>
#include <Logging/Logger.h>

#include <math.h>

using namespace OpenEngine::Display;

namespace OpenEngine {
    namespace Scene {
        
        HeightFieldPatchNode::HeightFieldPatchNode(int xStart, int zStart, HeightFieldNode* t)
            : LOD(MAX_LODS-1), xStart(xStart), zStart(zStart), terrain(t){

            xEnd = xStart + PATCH_EDGE_VERTICES;
            zEnd = zStart + PATCH_EDGE_VERTICES;
            xEndMinusOne = xEnd - 1;
            zEndMinusOne = zEnd - 1;

            ComputeIndices();
            
            rightNeighbour = upperNeighbour = NULL;

            SetupBoundingBox();
        }

        void HeightFieldPatchNode::CalcLOD(IViewingVolume* view){
            if (!view->IsVisible(boundingBox)){
                visible = false;
                return;
            }
            visible = true;

            Vector<3, float> viewPos = view->GetPosition();
            
            float distance = (viewPos - patchCenter).GetLength();

            float baseDistance = terrain->GetLODBaseDistance();
            float incDistance = terrain->GetLODIncDistance();

            distance -= baseDistance;
            
            geomorphingScale = distance / incDistance;

            geomorphingScale = geomorphingScale < 1 ? 1 : geomorphingScale;
            geomorphingScale = geomorphingScale > MAX_LODS ? MAX_LODS : geomorphingScale;

            LOD = floor(geomorphingScale);
        }

        void HeightFieldPatchNode::Render(){
            if (visible){
                int rightLODdiff = rightNeighbour != NULL ? rightNeighbour->GetLOD() - LOD + 1 : 1;
                int upperLODdiff = upperNeighbour != NULL ? upperNeighbour->GetLOD() - LOD + 1 : 1;
                
                unsigned int numberOfIndices = LODs[LOD-1][rightLODdiff][upperLODdiff].numberOfIndices;
                void* offset = LODs[LOD-1][rightLODdiff][upperLODdiff].indiceBufferOffset;
                glDrawElements(GL_TRIANGLE_STRIP, numberOfIndices, GL_UNSIGNED_INT, offset);
            }
        }

        void HeightFieldPatchNode::RenderBoundingGeometry(){
            glBegin(GL_LINES);
            Vector<3, float> center = boundingBox.GetCenter();
            glColor3f(center[0], center[1], center[2]);

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

        void HeightFieldPatchNode::ComputeIndices(){

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
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

            //logger.info << "indice addr in patch: " << (unsigned int) LODs[1][1][1].indices << logger.end;
        }
        
        unsigned int* HeightFieldPatchNode::ComputeBodyIndices(int& indices, int LOD){
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

            if (i != indices)
                logger.info << "woa" << logger.end;

            return ret;
        }

        unsigned int* HeightFieldPatchNode::ComputeRightStichingIndices(int& indices, int LOD, LODrelation rightLOD){
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

        unsigned int* HeightFieldPatchNode::ComputeUpperStichingIndices(int& indices, int LOD, LODrelation upperLOD){
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

        void HeightFieldPatchNode::SetupBoundingBox(){
            Vector<3, float> min = Vector<3, float>(terrain->GetVertex(xStart, zStart));
            Vector<3, float> max = Vector<3, float>(terrain->GetVertex(xEnd-1, zEnd-1));

            for (int x = xStart; x < xEnd; ++x){
                for (int z = zStart; z < zEnd; ++z){
                    float y = terrain->GetVertex(x, z)[1];
                    min[1] = y < min[1] ? y : min[1];
                    max[1] = y > max[1] ? y : max[1];
                }
            }

            patchCenter = (min + max) / 2;
            boundingBox = Box(patchCenter, max - patchCenter);

            patchCenter[1] = 0;
        }

    }
}

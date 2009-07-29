// Landscape patch node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include "LandscapePatchNode.h"
#include <Meta/OpenGL.h>
#include <Logging/Logger.h>

namespace OpenEngine {
    namespace Scene {

        LandscapePatchNode::LandscapePatchNode(int xOffset, int zOffset, LandscapeNode* land) 
            : xStart(xOffset), zStart(zOffset), landscape(land), landscapeWidth(land->GetWidth())
        {
            xEnd = xStart + PATCH_EDGE_VERTICES;
            zEnd = zStart + PATCH_EDGE_VERTICES;

            ComputeIndices();

            right = upper = NULL;

            baseDistance = 100;
            incrementalDistance = 100;
            CalcLODSwitchDistances();

            float xmin, ymin, zmin, xmax, ymax, zmax;
            land->GetCoords(zStart + xStart * landscapeWidth, xmin, ymin, zmin);
            land->GetCoords(zEnd-1 + (xEnd-1) * landscapeWidth, xmax, ymax, zmax);
            for (int x = xStart; x < xEnd; ++x){
                for (int z = zStart; z < zEnd; ++z){
                    float y;
                    land->GetYCoord(z + x * landscapeWidth, y);
                    ymin = y < ymin ? y : ymin;
                    ymax = y > ymax ? y : ymax;
                }
            }

            patchCenter = Vector<3, float>((xmax + xmin) / 2, (ymax + ymin) / 2, (zmax + zmin) / 2);
            boundingBox = new Box(patchCenter, Vector<3, float>(xmax - patchCenter[0], ymax - patchCenter[1], zmax - patchCenter[2]));
        }

        LandscapePatchNode::~LandscapePatchNode(){
            /*
            if (indices) delete[] indices;
            if (LODs){ 
                for (int i = 0; i < MAX_LODS; ++i){
                    delete[] LODs[i].rightStitchingHigherIndices;
                    delete[] LODs[i].rightStitchingEqualIndices;
                    delete[] LODs[i].rightStitchingLowerIndices;
                    delete[] LODs[i].upperStitchingHigherIndices;
                    delete[] LODs[i].upperStitchingEqualIndices;
                    delete[] LODs[i].upperStitchingLowerIndices;
                }
                delete[] LODs;
                }
            if (lodDistanceSquared) delete[] lodDistanceSquared;
            */
        }

        /**
         * Set the distance at which the LOD should switch.
         *
         * @ base The base distance to the camera where the LOD is the highest.
         * @ dec The distance between each decrement in LOD.
         */
        void LandscapePatchNode::SetLODSwitchDistance(float base, float dec){
            baseDistance = base;
            incrementalDistance = dec;
            CalcLODSwitchDistances();
        }

        /**
         * Calculate the level of detail of the node based on the
         * distance to the camera.
         */
        void LandscapePatchNode::CalcLOD(IViewingVolume* view){
            if (!view->IsVisible(*boundingBox)){
                currentLOD = NULL;
                LOD = 0;
                return;
            }

            Vector<3, float> viewPos = view->GetPosition();
            float distanceSquared = (viewPos - patchCenter).GetLengthSquared();

            currentLOD = &LODs[MAX_LODS-1];
            for (int i = 0; i < MAX_LODS-1; ++i){
                if (distanceSquared < lodDistanceSquared[i]){
                    currentLOD = &LODs[i];
                    break;
                }
            }

            LOD = currentLOD->LOD;
        }

        /**
         * Render the patch.
         *
         * Assumes the vertex arrays, color arrays and soforth already
         * has been specified by the landscape node. Thus here we only
         * need to send the indices to the graphics card.
         *
         * @TODO perhaps send along shader info / reload shader
         */
        void LandscapePatchNode::Render(){
            if (LOD == 0) return;

            glDrawElements(GL_TRIANGLE_STRIP, currentLOD->numberOfBodyIndices, GL_UNSIGNED_INT, currentLOD->bodyIndices);

            if (right == NULL || right->GetLOD() == 0 || LOD == right->GetLOD()){
                glDrawElements(GL_TRIANGLE_STRIP, currentLOD->numberOfStichingEqualIndices, GL_UNSIGNED_INT, currentLOD->rightStitchingEqualIndices);
            }else if (LOD < right->GetLOD()){
                glDrawElements(GL_TRIANGLE_STRIP, currentLOD->numberOfStichingHigherIndices, GL_UNSIGNED_INT, currentLOD->rightStitchingHigherIndices);
            }else //if (LOD > right->GetLOD())
                glDrawElements(GL_TRIANGLE_STRIP, currentLOD->numberOfStichingLowerIndices, GL_UNSIGNED_INT, currentLOD->rightStitchingLowerIndices);

            if (upper == NULL || upper->GetLOD() == 0 || LOD == upper->GetLOD()){
                glDrawElements(GL_TRIANGLE_STRIP, currentLOD->numberOfStichingEqualIndices, GL_UNSIGNED_INT, currentLOD->upperStitchingEqualIndices);
            }else if (LOD < upper->GetLOD()){
                glDrawElements(GL_TRIANGLE_STRIP, currentLOD->numberOfStichingHigherIndices, GL_UNSIGNED_INT, currentLOD->upperStitchingHigherIndices);
            }else //if (LOD > upper->GetLOD())
                glDrawElements(GL_TRIANGLE_STRIP, currentLOD->numberOfStichingLowerIndices, GL_UNSIGNED_INT, currentLOD->upperStitchingLowerIndices);

            // Render bounding box
            /*
            glBegin(GL_LINES);
            
            Vector<3, float> center = boundingBox->GetCenter();
            glColor3f(center[0], center[1], center[2]);

            for (int i = 0; i < 8; ++i){
                Vector<3, float> ic = boundingBox->GetCorner(i);
                for (int j = i+1; j < 8; ++j){
                    Vector<3, float> jc = boundingBox->GetCorner(j);
                    glVertex3f(ic[0], ic[1], ic[2]);
                    glVertex3f(jc[0], jc[1], jc[2]);
                }
            }

            glEnd();
            */
        }

        void LandscapePatchNode::RenderNormals(){
            
        }

        void LandscapePatchNode::ComputeIndices(){
            LODs = new LODstruct[MAX_LODS];

            int LOD = 1;
            for (int i = 0; i < MAX_LODS; ++i){
                LODs[i].LOD = LOD;
                
                ComputeBodyIndices(&LODs[i]);

                ComputeStitchingIndices(&LODs[i]);

                LOD *= 2;
            }
        }

        void LandscapePatchNode::ComputeBodyIndices(LODstruct* lods){
            int LOD = lods->LOD;
            int xs = (PATCH_EDGE_VERTICES - 1)/LOD - 1;
            int zs = (PATCH_EDGE_VERTICES - 1)/LOD;
            lods->numberOfBodyIndices = 2 * xs * zs + 2 * xs;
            lods->bodyIndices = new GLuint[lods->numberOfBodyIndices];

            int i = 0;
            for (int x = xStart; x < xEnd - 2 * LOD; x += LOD){
                for (int z = zEnd - 1 - LOD; z >= zStart; z -= LOD){
                    lods->bodyIndices[i++] = z + x * landscapeWidth;
                    lods->bodyIndices[i++] = z + (x + LOD) * landscapeWidth;
                }
                lods->bodyIndices[i++] = lods->bodyIndices[i-1];
                lods->bodyIndices[i++] = zEnd - 1 - LOD + (x + LOD) * landscapeWidth;
            }
        }
        
        void LandscapePatchNode::ComputeStitchingIndices(LODstruct* lods){
            int LOD = lods->LOD;
            lods->numberOfStichingLowerIndices = 4 * (PATCH_EDGE_VERTICES - 1)/LOD + 1;
            lods->numberOfStichingEqualIndices = 2 * (PATCH_EDGE_VERTICES - 1)/LOD + 1;
            lods->numberOfStichingHigherIndices = 2 * (PATCH_EDGE_VERTICES - 1)/LOD;

            lods->rightStitchingHigherIndices = new GLuint[lods->numberOfStichingHigherIndices];
            ComputeRightStichingIndices(lods->rightStitchingHigherIndices, lods->LOD, lods->LOD * 2);

            lods->rightStitchingEqualIndices = new GLuint[lods->numberOfStichingEqualIndices];
            ComputeRightStichingIndices(lods->rightStitchingEqualIndices, lods->LOD, lods->LOD);

            if (lods->LOD > 1){
                lods->rightStitchingLowerIndices = new GLuint[lods->numberOfStichingLowerIndices];
                ComputeRightStichingIndices(lods->rightStitchingLowerIndices, lods->LOD, lods->LOD / 2);
            }
            
            lods->upperStitchingHigherIndices = new GLuint[lods->numberOfStichingHigherIndices];
            ComputeUpperStichingIndices(lods->upperStitchingHigherIndices, lods->LOD, lods->LOD * 2);

            lods->upperStitchingEqualIndices = new GLuint[lods->numberOfStichingEqualIndices];
            ComputeUpperStichingIndices(lods->upperStitchingEqualIndices, lods->LOD, lods->LOD);

            if (lods->LOD > 1){
                lods->upperStitchingLowerIndices = new GLuint[lods->numberOfStichingLowerIndices];
                ComputeUpperStichingIndices(lods->upperStitchingLowerIndices, lods->LOD, lods->LOD / 2);
            }
        }

        void LandscapePatchNode::ComputeRightStichingIndices(GLuint* indices, int LOD, int rightLOD){
            int i = 0;
            if (LOD == rightLOD){
                // Draw the stitching with the patchs LOD
                for (int x = xStart; x < xEnd - LOD; x += LOD){
                    indices[i++] = zEnd - 1 - LOD + x * landscapeWidth;
                    indices[i++] = zEnd - 1 + x * landscapeWidth;
                }
                indices[i++] = zEnd - 1 + (xEnd - 1) * landscapeWidth;
            }else if (LOD < rightLOD){
                // Right patch is twice as detailed
                for (int x = xStart; x < xEnd - LOD; x += LOD){
                    if ((x/LOD) % 2 == 0){
                        indices[i++] = zEnd - 1 - LOD + x * landscapeWidth;
                        indices[i++] = zEnd - 1 + x * landscapeWidth;
                    }else{
                        indices[i++] = zEnd - 1 - LOD + x * landscapeWidth;
                        indices[i++] = zEnd - 1 + (x + LOD) * landscapeWidth;
                    }
                }
            }else if (rightLOD < LOD){
                // Right patch is half as detailed
                for (int x = xEnd - 1 - LOD; x >= xStart; x -= LOD){
                    indices[i++] = zEnd - 1 + (x + LOD) * landscapeWidth;
                    indices[i++] = zEnd - 1 - LOD + x * landscapeWidth;
                    indices[i++] = zEnd - 1 + (x + rightLOD) * landscapeWidth;
                    indices[i++] = zEnd - 1 - LOD + x * landscapeWidth;
                }
                indices[i++] = zEnd - 1 + xStart * landscapeWidth;
            }
        }

        void LandscapePatchNode::ComputeUpperStichingIndices(GLuint* indices, int LOD, int upperLOD){
            int i = 0;
            if (LOD == upperLOD){
                // Draw the stitching with the patchs LOD
                indices[i++] = zEnd - 1 + (xEnd - 1) * landscapeWidth;
                for (int z = zEnd - 1 - LOD; z >= zStart; z -= LOD){
                    indices[i++] = z + (xEnd - 1) * landscapeWidth;
                    indices[i++] = z + (xEnd - 1 - LOD) * landscapeWidth;
                }
            }else if (LOD < upperLOD){
                // The upper patch is twice as detailed
                for (int z = zEnd - 1 - LOD; z >= zStart; z -= LOD){
                    if ((z/LOD) % 2 == 0){
                        indices[i++] = z + (xEnd - 1 - LOD) * landscapeWidth;
                        indices[i++] = z + (xEnd - 1) * landscapeWidth;
                    }else{
                        indices[i++] = z + (xEnd - 1 - LOD) * landscapeWidth;
                        indices[i++] = z + LOD + (xEnd - 1) * landscapeWidth;
                    }
                }
            }else if (upperLOD < LOD){
                indices[i++] = zStart + (xEnd - 1) * landscapeWidth;
                // The upper patch is half as detailed
                for (int z = zStart; z < zEnd - LOD; z += LOD){
                    indices[i++] = z + (xEnd - 1 - LOD) * landscapeWidth;
                    indices[i++] = z + upperLOD + (xEnd - 1) * landscapeWidth;
                    indices[i++] = z + (xEnd - 1 - LOD) * landscapeWidth;
                    indices[i++] = z + LOD + (xEnd - 1) * landscapeWidth;
                }
            }
        }

        void LandscapePatchNode::CalcLODSwitchDistances(){
            lodDistanceSquared = new float[MAX_LODS];
            
            for (int i = 0; i < MAX_LODS; ++i){
                lodDistanceSquared[i] = (baseDistance + i * incrementalDistance) * (baseDistance + i * incrementalDistance);
            }
        }

    }
}

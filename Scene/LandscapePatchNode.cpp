// Landscape patch node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Scene/LandscapePatchNode.h>
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

            SetupBoundingBox();
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

        void LandscapePatchNode::RecalcBoundingBox(){
            SetupBoundingBox(); 
        }

        /**
         * Calculate the level of detail of the node based on the
         * distance to the camera.
         */
        void LandscapePatchNode::CalcLOD(IViewingVolume* view){
            if (!view->IsVisible(boundingBox)){
                currentLOD = NULL;
                LOD = 0;
                return;
            }

            Vector<3, float> viewPos = view->GetPosition();
            float distanceSquared = (viewPos - patchCenter).GetLengthSquared();

            float* lodSwitch = landscape->GetLODSwitchArray();
            
            currentLOD = &LODs[MAX_LODS-1];
            for (int i = 1; i < MAX_LODS; ++i){
                if (distanceSquared < lodSwitch[i]){
                    currentLOD = &LODs[i-1];

                    // calculate geoMorphingScale
                    float l = distanceSquared - lodSwitch[i-1];
                    float h = lodSwitch[i] - lodSwitch[i-1];
                    geoMorphingScale = l / h;

                    break;
                }
            }

            LOD = currentLOD->LOD;

            GeoMorph();
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

        }

        void LandscapePatchNode::RenderNormals(){
            
        }

        void LandscapePatchNode::RenderBoundingBoxes(){
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
            lods->numberOfStichingHigherIndices = 2 * (PATCH_EDGE_VERTICES - 1)/LOD + 1;

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
                indices[i++] = zEnd - 1 + (xEnd - 1) * landscapeWidth;
                for (int x = xEnd - 1 - LOD; x >= xStart; x -= LOD){
                    indices[i++] = zEnd - 1 - LOD + x * landscapeWidth;
                    indices[i++] = zEnd - 1 + x * landscapeWidth;
                }
            }else if (LOD < rightLOD){
                // Right patch is twice as detailed
                for (int x = xEnd - 1 - 2*LOD; x >= xStart; x -= 2*LOD){
                    indices[i++] = zEnd - 1 + (x+2*LOD) * landscapeWidth;
                    indices[i++] = zEnd - 1 - LOD + (x+LOD) * landscapeWidth;
                    indices[i++] = zEnd - 1 + (x + 2*LOD) * landscapeWidth;
                    indices[i++] = zEnd - 1 - LOD + x * landscapeWidth;
                }
                indices[i++] = zEnd - 1 + xStart * landscapeWidth;
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
                for (int z = zStart; z < zEnd - LOD; z += LOD){
                    indices[i++] = z + (xEnd - 1) * landscapeWidth;
                    indices[i++] = z + (xEnd - 1 - LOD) * landscapeWidth;
                }
                indices[i++] = zEnd - 1 + (xEnd - 1) * landscapeWidth;
            }else if (LOD < upperLOD){
                // The upper patch is twice as detailed
                indices[i++] = zStart + (xEnd - 1) * landscapeWidth;
                // The upper patch is half as detailed
                for (int z = zStart; z < zEnd - LOD; z += 2*LOD){
                    indices[i++] = z + (xEnd-1-LOD) * landscapeWidth;
                    indices[i++] = z + 2 * LOD + (xEnd - 1) * landscapeWidth;
                    indices[i++] = z + LOD + (xEnd - 1 - LOD) * landscapeWidth;
                    indices[i++] = z + 2 * LOD + (xEnd - 1) * landscapeWidth;
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

        void LandscapePatchNode::SetupBoundingBox(){
            float xmin, ymin, zmin, xmax, ymax, zmax;
            landscape->GetCoords(zStart + xStart * landscapeWidth, xmin, ymin, zmin);
            landscape->GetCoords(zEnd-1 + (xEnd-1) * landscapeWidth, xmax, ymax, zmax);
            for (int x = xStart; x < xEnd; ++x){
                for (int z = zStart; z < zEnd; ++z){
                    float y = landscape->GetYCoord(x, z);;
                    ymin = y < ymin ? y : ymin;
                    ymax = y > ymax ? y : ymax;
                }
            }

            patchCenter = Vector<3, float>((xmax + xmin) / 2, (ymax + ymin) / 2, (zmax + zmin) / 2);
            boundingBox = Box(patchCenter, Vector<3, float>(xmax - patchCenter[0], ymax - patchCenter[1], zmax - patchCenter[2]));
        }

        void LandscapePatchNode::GeoMorph(){
            for (int x = xStart; x < xEnd-1; x += LOD){
                for (int z = zStart; z < zEnd-1; z += LOD){
                    landscape->GeoMorphCoord(x, z, LOD, geoMorphingScale);
                }
            }
        }

    }
}

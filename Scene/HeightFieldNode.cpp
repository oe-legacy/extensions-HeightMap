// Landscape node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Scene/HeightFieldNode.h>
#include <Scene/HeightFieldPatchNode.h>
#include <Math/Math.h>
#include <Meta/OpenGL.h>
#include <Logging/Logger.h>
#include <Utils/TerrainUtils.h>
#include <Renderers/OpenGL/TextureLoader.h>

#include <algorithm>

#define USE_PATCHES true

using namespace OpenEngine::Renderers::OpenGL;

namespace OpenEngine {
    namespace Scene {
        
        HeightFieldNode::HeightFieldNode(ITextureResourcePtr tex)
            : tex(tex) {
            tex->Load();
            heightScale = 1;
            widthScale = 1;
            waterlevel = 10;
            
            texDetail = 1;
            baseDistance = 1;
            incrementalDistance = 1;

            texCoords = NULL;
        }

        void HeightFieldNode::Load() {
            InitArrays();
            if (USE_PATCHES)
                SetupPatches();
            else
                ComputeIndices();
        }

        void HeightFieldNode::CalcLOD(Display::IViewingVolume* view){
            if (USE_PATCHES)
                for (int i = 0; i < numberOfPatches; ++i)
                    patchNodes[i]->CalcLOD(view);
        }

        void HeightFieldNode::Render(){
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indiceId);
            if (USE_PATCHES){
                for (int i = 0; i < numberOfPatches; ++i)
                    patchNodes[i]->Render();
            }else{
                glDrawElements(GL_TRIANGLE_STRIP, numberOfIndices, GL_UNSIGNED_INT, 0);
            }
        }

        void HeightFieldNode::RenderBoundingGeometry(){
            if (USE_PATCHES)
                for (int i = 0; i < numberOfPatches; ++i)
                    patchNodes[i]->RenderBoundingGeometry();
        }

        void HeightFieldNode::VisitSubNodes(ISceneNodeVisitor& visitor){
            list<ISceneNode*>::iterator itr;
            for (itr = subNodes.begin(); itr != subNodes.end(); ++itr){
                (*itr)->Accept(visitor);
            }
        }

        void HeightFieldNode::Handle(RenderingEventArg arg){
            Load();

            if (landscapeShader != NULL) {
                landscapeShader->Load();
                TextureList texs = landscapeShader->GetTextures();
                for (unsigned int i = 0; i < texs.size(); ++i)
                    TextureLoader::LoadTextureResource(texs[i]);

                TextureLoader::LoadTextureResource(normalmap, true, false);

                landscapeShader->ApplyShader();

                landscapeShader->SetUniform("snowStartHeight", (float)50);
                landscapeShader->SetUniform("snowBlend", (float)20);
                landscapeShader->SetUniform("grassStartHeight", (float)5);
                landscapeShader->SetUniform("grassBlend", (float)5);
                landscapeShader->SetUniform("sandStartHeight", (float)-10);
                landscapeShader->SetUniform("sandBlend", (float)10);

                landscapeShader->SetTexture("normalMap", normalmap);

                landscapeShader->ReleaseShader();
            }
            
            SetLODSwitchDistance(0, 100);

            // Create vbos

            // Vertice buffer object
            glGenBuffers(1, &verticeBufferId);
            glBindBuffer(GL_ARRAY_BUFFER, verticeBufferId);
            glBufferData(GL_ARRAY_BUFFER, 
                         sizeof(GLfloat) * numberOfVertices * DIMENSIONS,
                         vertices, GL_STATIC_DRAW);
            
            if (landscapeShader != NULL){
                // Geomorph values buffer object
                glGenBuffers(1, &geomorphBufferId);
                glBindBuffer(GL_ARRAY_BUFFER, geomorphBufferId);
                glBufferData(GL_ARRAY_BUFFER, 
                             sizeof(GLfloat) * numberOfVertices * 3,
                             geomorphValues, GL_STATIC_DRAW);
            }

            // Tex Coord buffer object
            glGenBuffers(1, &texCoordBufferId);
            glBindBuffer(GL_ARRAY_BUFFER, texCoordBufferId);
            glBufferData(GL_ARRAY_BUFFER, 
                         sizeof(GLfloat) * numberOfVertices * TEXCOORDS,
                         texCoords, GL_STATIC_DRAW);

            // Tex Coord buffer object
            glGenBuffers(1, &normalMapCoordBufferId);
            glBindBuffer(GL_ARRAY_BUFFER, normalMapCoordBufferId);
            glBufferData(GL_ARRAY_BUFFER, 
                         sizeof(GLfloat) * numberOfVertices * TEXCOORDS,
                         normalMapCoords, GL_STATIC_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // Create indice buffer
            glGenBuffers(1, &indiceId);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indiceId);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         sizeof(GLuint) * numberOfIndices,
                         indices, GL_STATIC_DRAW);
            
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }
        
        // **** Get/Set methods ****

        int HeightFieldNode::GetIndice(int x, int z){
            return CoordToIndex(x, z);
        }

        float* HeightFieldNode::GetVertex(int x, int z){
            if (x < 0)
                x = 0;
            else if (x >= depth)
                x = depth - 1;

            if (z < 0)
                z = 0;
            else if (z >= width)
                z = width - 1;
            return GetVertice(x, z);
        }

        /**
         * Set the distance at which the LOD should switch.
         *
         * @ base The base distance to the camera where the LOD is the highest.
         * @ dec The distance between each decrement in LOD.
         */
        void HeightFieldNode::SetLODSwitchDistance(float base, float dec){
            baseDistance = base;
            
            float edgeLength = HeightFieldPatchNode::PATCH_EDGE_SQUARES * widthScale;
            if (dec * dec < edgeLength * edgeLength * 2){
                incrementalDistance = sqrt(edgeLength * edgeLength * 2);
                logger.error << "Incremental LOD distance is too low, setting it to lowest value: " << dec << logger.end;
            }else
                incrementalDistance = dec;

            // Update uniforms
            if (landscapeShader != NULL) {
                landscapeShader->ApplyShader();

                landscapeShader->SetUniform("baseDistance", baseDistance);
                landscapeShader->SetUniform("invIncDistance", 1.0f / incrementalDistance);

                landscapeShader->ReleaseShader();
            }
        }

        void HeightFieldNode::SetTextureDetail(const float detail){
            texDetail = detail;
            if (texCoords)
                SetupTerrainTexture();
        }

        // **** inline functions ****

        void HeightFieldNode::InitArrays(){
            int texWidth = tex->GetWidth();
            int texDepth = tex->GetHeight();

            // if texwidth/depth isn't expressible as n * patchwidth + 1 fix it.
            int patchWidth = HeightFieldPatchNode::PATCH_EDGE_SQUARES;
            int widthRest = (texWidth - 1) % patchWidth;
            width = widthRest ? texWidth + patchWidth - widthRest : texWidth;

            int depthRest = (texDepth - 1) % patchWidth;
            depth = depthRest ? texDepth + patchWidth - depthRest : texDepth;

            numberOfVertices = width * depth;

            vertices = new float[numberOfVertices * DIMENSIONS];
            texCoords = new float[numberOfVertices * TEXCOORDS];
            normalMapCoords = new float[numberOfVertices * TEXCOORDS];
            geomorphValues = new float[numberOfVertices * 3];

            int numberOfCharsPrColor = tex->GetDepth() / 8;
            unsigned char* data = tex->GetData();

            // Fill the vertex array
            int d = numberOfCharsPrColor - 1;
            for (int x = 0; x < depth; ++x){
                for (int z = 0; z < width; ++z){
                    float* vertice = GetVertice(x, z);
                     
                    vertice[0] = widthScale * x;
                    vertice[2] = widthScale * z;
                    vertice[3] = 1;
       
                    if (x < texDepth && z < texWidth){
                        // inside the heightmap
                        float height = (float)data[d];
                        d += numberOfCharsPrColor;
                        vertice[1] = height * heightScale - waterlevel - heightScale / 2;
                    }else{
                        // outside the heightmap, set height to 0
                        vertice[1] = -waterlevel;
                    }
                }
            }
            
            SetupNormalMap();
            SetupTerrainTexture();

            if (landscapeShader != NULL)
                CalcVerticeLOD();
                for (int x = 0; x < depth; ++x)
                    for (int z = 0; z < width; ++z)
                        CalcGeomorphHeight(x, z);
        }

        void HeightFieldNode::SetupNormalMap(){
            normalmap = Utils::CreateNormalMap(tex, heightScale, widthScale);
            for (int x = 0; x < depth; ++x)
                for (int z = 0; z < width; ++z){
                    float* coord = GetNormalMapCoord(x, z);
                    coord[1] = x / (float) depth-1;
                    coord[0] = z / (float) width-1;
                }
        }

        void HeightFieldNode::SetupTerrainTexture(){
            for (int x = 0; x < depth; ++x){
                for (int z = 0; z < width; ++z){
                    CalcTexCoords(x, z);
                }
            }
        }

        void HeightFieldNode::CalcTexCoords(int x, int z){
            float* texCoord = GetTexCoord(x, z);
            texCoord[1] = x * texDetail;
            texCoord[0] = z * texDetail;
        }

        void HeightFieldNode::CalcVerticeLOD(){
            for (int LOD = 1; LOD <= HeightFieldPatchNode::MAX_LODS; ++LOD){
                int delta = pow(2, LOD-1);
                for (int x = 0; x < depth; x += delta){
                    for (int z = 0; z < width; z += delta){
                        GetVerticeLOD(x, z) = LOD;
                    }
                }
            }
        }

        void HeightFieldNode::CalcGeomorphHeight(int x, int z){
            int LOD = (int)GetVerticeLOD(x, z);

            int delta = pow(2, LOD-1);

            int dx, dz;
            if (LOD < HeightFieldPatchNode::MAX_LODS){
                dx = x % (delta * 2);
                dz = z % (delta * 2);
            }else{
                dx = 0;
                dz = 0;
            }

            float* vertice = GetVertice(x, z);
            float* verticeNeighbour1 = GetVertice(x + dx, z + dz);
            float* verticeNeighbour2 = GetVertice(x - dx, z - dz);

            // Store the morphing value in the w-coord to use in the
            // shader.
            vertice[3] = (verticeNeighbour1[1] + verticeNeighbour2[1]) / 2 - vertice[1];
        }

        void HeightFieldNode::ComputeIndices(){
            int LOD = 4;
            int xs = (depth-1) / LOD + 1;
            int zs = (width-1) / LOD + 1;
            numberOfIndices = 2 * ((xs - 1) * zs + xs - 2);
            indices = new unsigned int[numberOfIndices];

            unsigned int i = 0;
            for (int x = 0; x < depth - 1; x += LOD){
                for (int z = width - 1; z >= 0; z -= LOD){
                    indices[i++] = CoordToIndex(x, z);
                    indices[i++] = CoordToIndex(x+LOD, z);
                }
                if (x < depth - 1 - LOD){
                    indices[i++] = CoordToIndex(x+LOD, 0);
                    indices[i++] = CoordToIndex(x+LOD, width - 1);
                }
            }

            if (i < numberOfIndices){
                logger.info << "Allocated to much memory for the indices, lets get lower" << logger.end;
                numberOfIndices = i;
            }else if (i > numberOfIndices){
                logger.info << "You're about to crash monsiour. Good luck. Allocated " << numberOfIndices << " but used " << i << logger.end;
                numberOfIndices = i;
            }
        }

        void HeightFieldNode::SetupPatches(){
            // Create the patches
            int squares = HeightFieldPatchNode::PATCH_EDGE_SQUARES;
            patchGridWidth = (width-1) / squares;
            patchGridDepth = (depth-1) / squares;
            numberOfPatches = patchGridWidth * patchGridDepth;
            patchNodes = new HeightFieldPatchNode*[numberOfPatches];
            int entry = 0;
            for (int x = 0; x < depth - squares; x +=squares ){
                for (int z = 0; z < width - squares; z += squares){
                    patchNodes[entry++] = new HeightFieldPatchNode(x, z, this);
                }
            }

            // Link the patches
            for (int x = 0; x < patchGridDepth; ++x){
                for (int z = 0; z < patchGridWidth; ++z){
                    int entry = z + x * patchGridWidth;
                    if (x + 1 < patchGridDepth)
                        patchNodes[entry]->SetUpperNeighbor(patchNodes[entry + patchGridWidth]);
                    if (z + 1 < patchGridWidth) 
                        patchNodes[entry]->SetRightNeighbor(patchNodes[entry + 1]);
                }
            }

            // Setup indice buffer
            numberOfIndices = 0;
            for (int p = 0; p < numberOfPatches; ++p){
                for (int l = 0; l < HeightFieldPatchNode::MAX_LODS; ++l){
                    for (int rl = 0; rl < 3; ++rl){
                        for (int ul = 0; ul < 3; ++ul){
                            LODstruct& lod = patchNodes[p]->GetLod(l,rl,ul);
                            lod.indiceBufferOffset = (void*)(numberOfIndices * sizeof(GLuint));
                            numberOfIndices += lod.numberOfIndices;
                        }
                    }
                }
            }

            indices = new unsigned int[numberOfIndices];

            unsigned int i = 0;
            for (int p = 0; p < numberOfPatches; ++p){
                for (int l = 0; l < HeightFieldPatchNode::MAX_LODS; ++l){
                    for (int rl = 0; rl < 3; ++rl){
                        for (int ul = 0; ul < 3; ++ul){
                            LODstruct& lod = patchNodes[p]->GetLod(l,rl,ul);
                            memcpy(indices + i, lod.indices, sizeof(unsigned int) * lod.numberOfIndices);
                            i += lod.numberOfIndices;
                        }
                    }
                }
            }
                        
            // Setup shader uniforms used in geomorphing
            for (int x = 0; x < depth - 1; ++x){
                for (int z = 0; z < width - 1; ++z){
                    int patchX = x / HeightFieldPatchNode::PATCH_EDGE_SQUARES;
                    int patchZ = z / HeightFieldPatchNode::PATCH_EDGE_SQUARES;
                    float centerOffset = HeightFieldPatchNode::PATCH_EDGE_SQUARES / 2 * widthScale;
                    float* geomorph = GetGeomorphValues(x, z);
                    geomorph[0] = patchX * HeightFieldPatchNode::PATCH_EDGE_SQUARES * widthScale + centerOffset;
                    geomorph[1] = patchZ * HeightFieldPatchNode::PATCH_EDGE_SQUARES * widthScale + centerOffset;
                }
            }
        }
        
        int HeightFieldNode::CoordToIndex(int x, int z) const{
            return z + x * width;
        }
        
        float* HeightFieldNode::GetVertice(int x, int z) const{
            int index = CoordToIndex(x, z);
            return vertices + index * DIMENSIONS;
        }
        
        float* HeightFieldNode::GetTexCoord(int x, int z) const{
            int index = CoordToIndex(x, z);
            return texCoords + index * TEXCOORDS;
        }

        float* HeightFieldNode::GetNormalMapCoord(int x, int z) const{
            int index = CoordToIndex(x, z);
            return normalMapCoords + index * TEXCOORDS;
        }

        float* HeightFieldNode::GetGeomorphValues(int x, int z) const{
            int index = CoordToIndex(x, z);
            return geomorphValues + index * 3;
        }

        float& HeightFieldNode::GetVerticeLOD(int x, int z) const{
            int index = CoordToIndex(x, z);
            return (geomorphValues + index * 3)[2];
        }
    }
}

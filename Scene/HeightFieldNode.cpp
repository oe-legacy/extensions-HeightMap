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
            ComputeIndices();
        }

        void HeightFieldNode::Render(){
            glDrawElements(GL_TRIANGLE_STRIP, numberOfIndices, GL_UNSIGNED_INT, indices);
        }

        void HeightFieldNode::VisitSubNodes(ISceneNodeVisitor& visitor){
            // @TODO Visit patches here aswell
            list<ISceneNode*>::iterator itr;
            for (itr = subNodes.begin(); itr != subNodes.end(); ++itr){
                (*itr)->Accept(visitor);
            }
        }

        void HeightFieldNode::Handle(RenderingEventArg arg){
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
            
            // Create vbos

            // Vertice buffer object
            glGenBuffers(1, &verticeBufferId);
            glBindBuffer(GL_ARRAY_BUFFER, verticeBufferId);
            glBufferData(GL_ARRAY_BUFFER, 
                         sizeof(GLfloat) * numberOfVertices * DIMENSIONS,
                         vertices, GL_STATIC_DRAW);
            
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
                         sizeof(GLfloat) * numberOfIndices,
                         indices, GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        }
        
        // **** Get/Set methods ****

        void HeightFieldNode::SetTextureDetail(float detail){
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
            width = widthRest ? texWidth + 32 - widthRest : texWidth;

            int depthRest = (texDepth - 1) % patchWidth;
            depth = depthRest ? texDepth + patchWidth - depthRest : texDepth;

            numberOfVertices = width * depth;

            vertices = new float[numberOfVertices * DIMENSIONS];
            texCoords = new float[numberOfVertices * TEXCOORDS];
            normalMapCoords = new float[numberOfVertices * TEXCOORDS];
            verticeLOD = new float[numberOfVertices];

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
            CalcVerticeLOD();

            if (landscapeShader != NULL)
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
            for (int LOD = 1; LOD < pow(2, HeightFieldPatchNode::MAX_LODS-1); LOD *= 2){
                for (int x = 0; x < depth; x += LOD){
                    for (int z = 0; z < width; z += LOD){
                        int entry = CoordToIndex(x, z);
                        verticeLOD[entry] = LOD;
                    }
                }
            }
        }

        void HeightFieldNode::CalcGeomorphHeight(int x, int z){
            int LOD = (int)GetVerticeLOD(x, z)[0];

            int dx = x % (LOD * 2);
            int dz = z % (LOD * 2);

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

        float* HeightFieldNode::GetVerticeLOD(int x, int z) const{
            int index = CoordToIndex(x, z);
            return verticeLOD + index;
        }
    }
}

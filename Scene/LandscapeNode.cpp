// Landscape node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Scene/LandscapeNode.h>
#include <Math/Math.h>
#include <Renderers/OpenGL/TextureLoader.h>
#include <Logging/Logger.h>

#include <algorithm>

using namespace OpenEngine::Renderers::OpenGL;

namespace OpenEngine {
    namespace Scene {

        /**
         * Creates the landscape node containing the terrain.
         *
         * @tex The texture used to create the heightmap. Must have a
         * width and depth of size (n * 32) + 1 or it will be padded.
         * @ heightScale Scales the height of the heightmap.
         * @ widthScale Scales the width and depth of the heightmap.
         */        
        LandscapeNode::LandscapeNode(ITextureResourcePtr tex, IShaderResourcePtr shader, float heightScale, float widthScale) 
            : widthScale(widthScale), landscapeShader(shader) {

            tex->Load();
            int texWidth = tex->GetWidth();
            int widthRest = (texWidth - 1) % 32;
            width = widthRest ? texWidth + 32 - widthRest : texWidth;

            int texDepth = tex->GetHeight();
            int depthRest = (texDepth - 1) % 32;
            depth = depthRest ? texDepth + 32 - depthRest : texDepth;

            numberOfVertices = width * depth;
            entries = numberOfVertices * DIMENSIONS;

            vertices = new GLfloat[entries];
            colors = new GLubyte[entries];
            normals = new GLfloat[entries];
            texCoords = new GLfloat[numberOfVertices * TEXCOORDS];
            verticeLOD = new int[numberOfVertices];
            originalValues = new GLfloat[numberOfVertices * 4];
            morphedValues = new GLfloat[numberOfVertices * 4];

            int texColorDepth = tex->GetDepth();
            int numberOfCharsPrColor = texColorDepth / 8;
            unsigned char* data = tex->GetData();

            // Fill the vertex and color arrays
            int i = 0, v = 0, c = 0, e = 0;
            for (int x = 0; x < depth; ++x){
                if (x < texDepth){
                    for (int z = 0; z < width; ++z){
                        if (z < texWidth){
                            // Fill the color array from the texture
                            float height = 0;
                            switch(numberOfCharsPrColor){
                            case 1:
                                colors[c++] = 0;
                                colors[c++] = 0;
                                colors[c++] = 0;
                                height = (float)data[i++];
                                break;
                            case 3:
                                {
                                    unsigned int r = colors[c++] = data[i++];
                                    unsigned int g = colors[c++] = data[i++];
                                    unsigned int b = colors[c++] = data[i++];
                                    height = (r + g + b) / 3;
                                    break;
                                }
                            case 4:
                                colors[c++] = data[i++];
                                colors[c++] = data[i++];
                                colors[c++] = data[i++];
                                height = (float)data[i++];
                                break;
                            }
                            vertices[v++] = widthScale * x;
                            originalValues[e++ * 4 + 3] = vertices[v++] = heightScale * height - WATERLEVEL - heightScale/2;
                            vertices[v++] = widthScale * z;
                        }else{
                            // Fill the color array with black
                            colors[c++] = 0;
                            colors[c++] = 0;
                            colors[c++] = 0;

                            // Place the vertex at the bottom.
                            vertices[v++] = widthScale * x;
                            originalValues[e++ * 4 + 3] = vertices[v++] = -WATERLEVEL - heightScale/2;
                            vertices[v++] = widthScale * z;
                        }
                    }
                }else{
                    for (int z = 0; z < width; ++z){
                        // Fill the color array with black
                        colors[c++] = 0;
                        colors[c++] = 0;
                        colors[c++] = 0;
                        
                        // Place the vertex at the bottom.
                        vertices[v++] = widthScale * x;
                        originalValues[e++ * 4 + 3] = vertices[v++] = -WATERLEVEL - heightScale/2;
                        vertices[v++] = widthScale * z;
                    }
                }
            }

            tex->Unload();

            // Calculate the normals
            for (int x = 0; x < depth; ++x){
                for (int z = 0; z < width; ++z){
                    CalcNormal(x, z);
                    int entry = CoordToEntry(x, z);
                    normals[entry * DIMENSIONS] = originalValues[entry * 4];
                    normals[entry * DIMENSIONS + 1] = originalValues[entry * 4 + 1];
                    normals[entry * DIMENSIONS + 2] = originalValues[entry * 4 + 2];
                }
            }

            // Setup the texture
            texDetail = 1;
            SetupTerrainTexture();

            // Create patches
            int squares = LandscapePatchNode::PATCH_EDGE_SQUARES;
            patchGridWidth = (width-1) / squares;
            patchGridDepth = (depth-1) / squares;
            numberOfPatches = patchGridWidth * patchGridDepth;
            patchNodes = new LandscapePatchNode*[numberOfPatches];
            int entry = 0;
            for (int x = 0; x < depth - squares; x +=squares ){
                for (int z = 0; z < width - squares; z += squares){
                    patchNodes[entry++] = new LandscapePatchNode(x, z, width, this);
                }
            }

            // Link patches
            for (int x = 0; x < patchGridDepth; ++x){
                for (int z = 0; z < patchGridWidth; ++z){
                    int entry = z + x * patchGridWidth;
                    if (0 < x)
                        patchNodes[entry]->SetLowerNeighbor(patchNodes[entry - patchGridWidth]);
                    if (x + 1 < patchGridDepth) 
                        patchNodes[entry]->SetUpperNeighbor(patchNodes[entry + patchGridWidth]);
                    if (z + 1 < patchGridWidth) 
                        patchNodes[entry]->SetRightNeighbor(patchNodes[entry + 1]);
                    if (0 < z) 
                        patchNodes[entry]->SetLeftNeighbor(patchNodes[entry - 1]);
                }
            }

            SetLODSwitchDistance(100, widthScale * 100);
            SetupGeoMorphing();
        }

        LandscapeNode::~LandscapeNode(){
            if (vertices) delete[] vertices;
            if (colors) delete[] colors;
            if (normals) delete[] normals;
            if (patchNodes) delete[] patchNodes;
        }

        void LandscapeNode::CloseBorder(float margin){
            for (int x = 0; x < depth; ++x){
                for (int z = 0; z < margin && z < width; ++z){
                    float scale = std::min(x / margin, z / margin);
                    scale = std::min(scale, (depth - x) / margin);
                    float y = YCoord(x, z) + WATERLEVEL;
                    SetYCoord(x, z, y * scale - WATERLEVEL);

                    int newZ = width - z - 1;
                    y = YCoord(x, newZ) + WATERLEVEL;
                    SetYCoord(x, newZ, y * scale - WATERLEVEL);
                }
            }

            for (int z = margin; z < width-margin; ++z){
                for (int x = 0; x < margin && x < depth; ++x){
                    float scale = std::min(x / margin, z / margin);
                    scale = std::min(scale, (width - z) / margin);
                    float y = YCoord(x, z) + WATERLEVEL;
                    SetYCoord(x, z, y * scale - WATERLEVEL);

                    int newX = depth - x - 1;
                    y = YCoord(newX, z) + WATERLEVEL;
                    SetYCoord(newX, z, y * scale - WATERLEVEL);
                }
            }
        }

        void LandscapeNode::SetCenter(Vector<3, float> center){
            // Move vertices
            float deltaX = center[0] - (depth * widthScale) / 2;
            float deltaZ = center[2] - (width * widthScale) / 2;
            for (int x = 0; x < depth; ++x){
                for (int z = 0; z < width; ++z){
                    int coord = CoordToEntry(x, z);
                    vertices[coord * 3] = x * widthScale + deltaX;
                    vertices[coord * 3 + 2] = z * widthScale + deltaZ;
                }
            }

            // Adjust boxes for culling
            for (int x = 0; x < patchGridDepth; ++x){
                for (int z = 0; z < patchGridWidth; ++z){
                    int entry = z + x * patchGridWidth;
                    patchNodes[entry]->RecalcBoundingBox();
                }
            }
        }

        void LandscapeNode::CalcLOD(IViewingVolume* view){
            for (int i = 0; i < numberOfPatches; ++i)
                patchNodes[i]->CalcLOD(view);
        }

        void LandscapeNode::Render(){
            glDrawElements(GL_TRIANGLE_STRIP, numberOfIndices, GL_UNSIGNED_INT, indices);
        }

        void LandscapeNode::RenderPatches(){
            for (int i = 0; i < numberOfPatches; ++i)
                patchNodes[i]->Render();
        }

        void LandscapeNode::RenderNormals(){
            glBegin(GL_LINES);
            glColor3f(1, 0, 0);
            for (int i = 0; i < numberOfVertices; ++i){
                glVertex3f(vertices[i*3], originalValues[i * 4 + 3], vertices[i*3+2]);
                glVertex3f(vertices[i*3]+originalValues[i*4], originalValues[i * 4 + 3]+originalValues[i*4+1], vertices[i*3+2]+originalValues[i*4+2]);
            }
            glEnd();
        }

        void LandscapeNode::VisitSubNodes(ISceneNodeVisitor& visitor){
            for (int i = 0; i < numberOfPatches; ++i)
                patchNodes[i]->Accept(visitor);
            list<ISceneNode*>::iterator itr;
            for (itr = subNodes.begin(); itr != subNodes.end(); ++itr){
                (*itr)->Accept(visitor);
            }
        }
        
        void LandscapeNode::Handle(RenderingEventArg arg){
            if (landscapeShader != NULL) {
                landscapeShader->Load();
                TextureList texs = landscapeShader->GetTextures();
                for (unsigned int i = 0; i < texs.size(); ++i)
                    TextureLoader::LoadTextureResource(texs[i]);

                landscapeShader->ApplyShader();

                landscapeShader->SetUniform("snowStartHeight", (float)50);
                landscapeShader->SetUniform("snowBlend", (float)20);
                landscapeShader->SetUniform("grassStartHeight", (float)5);
                landscapeShader->SetUniform("grassBlend", (float)5);
                landscapeShader->SetUniform("sandStartHeight", (float)-10);
                landscapeShader->SetUniform("sandBlend", (float)10);

                landscapeShader->ReleaseShader();
            }
        }

        void LandscapeNode::GetCoords(int index, float &x, float &y, float &z) const{
            int i = index * DIMENSIONS;
            x = vertices[i];
            y = GetYCoord(index);
            z = vertices[i+2];
        }

        float LandscapeNode::GetYCoord(const int index) const{
            return originalValues[index * 4 + 3];
        }

        float LandscapeNode::GetXCoord(const int x, const int z) const{
            return XCoord(x, z);
        }
        
        float LandscapeNode::GetYCoord(const int x, const int z) const{
            return YCoord(x, z);
        }

        float LandscapeNode::GetZCoord(const int x, const int z) const{
            return ZCoord(x, z);
        }

        void LandscapeNode::SetYCoord(const int x, const int z, float value){
            originalValues[CoordToEntry(x, z) * 4 + 3] = value;

            // Calculate the new normals
            CalcNormal(x, z);

            if (x > 0)
                CalcNormal(x - 1, z);
            if (x + 1 < depth)
                CalcNormal(x + 1, z);
            if (z > 0)
                CalcNormal(x, z - 1);
            if (z + 1 < width)
                CalcNormal(x, z + 1);

            // Setup geomorphing for the surrounding affected vertices
            CalcGeoMorphing(x, z);
            CalcSurroundingGeoMorphing(x, z);

            if (x > 0)
                CalcSurroundingGeoMorphing(x - 1, z);
            if (x + 1 < depth)
                CalcSurroundingGeoMorphing(x + 1, z);
            if (z > 0)
                CalcSurroundingGeoMorphing(x, z - 1);
            if (z + 1 < width)
                CalcSurroundingGeoMorphing(x, z + 1);
        }

        void LandscapeNode::GeoMorphCoord(int x, int z, int LOD, float scale){
            int entry = CoordToEntry(x, z);
            float vertexLOD = verticeLOD[entry];
            if (LOD >= vertexLOD){
                normals[entry * DIMENSIONS] = morphedValues[entry * 4] * scale + originalValues[entry * 4];
                normals[entry * DIMENSIONS + 1] = morphedValues[entry * 4 + 1] * scale + originalValues[entry * 4 + 1];
                normals[entry * DIMENSIONS + 2] = morphedValues[entry * 4 + 2] * scale + originalValues[entry * 4 + 2];
                vertices[entry * DIMENSIONS + 1] = morphedValues[entry * 4 + 3] * scale + originalValues[entry * 4 + 3];
            }else{
                normals[entry * DIMENSIONS] = originalValues[entry * 4];
                normals[entry * DIMENSIONS + 1] = originalValues[entry * 4 + 1];
                normals[entry * DIMENSIONS + 2] = originalValues[entry * 4 + 2];
                vertices[entry * DIMENSIONS + 1] = originalValues[entry * 4 + 3];
            }
        }

        void LandscapeNode::SetTextureDetail(float detail){
            texDetail = detail;
            SetupTerrainTexture();
        }

        /**
         * Set the distance at which the LOD should switch.
         *
         * @ base The base distance to the camera where the LOD is the highest.
         * @ dec The distance between each decrement in LOD.
         */
        void LandscapeNode::SetLODSwitchDistance(float base, float dec){
            baseDistance = base;
            
            float edgeLength = LandscapePatchNode::PATCH_EDGE_SQUARES * widthScale;
            if (dec * dec < edgeLength * edgeLength * 2){
                incrementalDistance = sqrt(edgeLength * edgeLength * 2);
                logger.info << "Incremental LOD distance is too low, setting it to lowest value: " << dec << logger.end;
            }else
                incrementalDistance = dec;
            

            CalcLODSwitchDistances();
        }

        // **** inline functions ****

        void LandscapeNode::ComputeIndices(){
            numberOfIndices = 2 * depth * width + 2 * depth - 2;
            indices = new unsigned int[numberOfIndices];

            int i = 0;
            for (int x = 0; x < depth - 2; ++x){
                for (int z = width - 1; z >= 0; --z){
                    indices[i++] = CoordToEntry(x, z);
                    indices[i++] = CoordToEntry(x+1, z);
                }
                if (x < depth - 2){
                    indices[i++] = indices[i-1];
                    indices[i++] = CoordToEntry(x+1, width - 1);
                }
            }

            if (i < numberOfIndices){
                logger.info << "Allocated to much memory, lets get lower" << logger.end;
                numberOfIndices = i;
            }else if (i > numberOfIndices){
                logger.info << "You're about to crash monsiour" << logger.end;
            }
        }

        void LandscapeNode::CalcNormal(int x, int z){
            int ns = 0;
            float pheight = YCoord(x, z);
            GLfloat normal[] = {0, 0, 0};

            // Calc "normal" to point above
            if (x + 1 < depth){
                GLfloat qheight = YCoord(x+1, z);
                normal[0] += pheight - qheight;
                ++ns;
            }

            // Calc "normal" to point below
            if (x > 1){
                GLfloat qheight = YCoord(x-1, z);
                normal[0] += qheight - pheight;
                ++ns;
            }

            // point to the right
            if (z + 1 < width){
                GLfloat qheight = YCoord(x, z+1);
                normal[2] += pheight - qheight;
                ++ns;
            }

            // point to the left
            if (z > 1){
                GLfloat qheight = YCoord(x, z+1);
                normal[2] += qheight - pheight;
                ++ns;
            }

            normal[1] = widthScale * ns;

            float norm = sqrt(normal[0] * normal[0]
                              + normal[1] * normal[1]
                              + normal[2] * normal[2]);

            int vertice = CoordToEntry(x, z) * 4;
            originalValues[vertice++] = normal[0] / norm;
            originalValues[vertice++] = normal[1] / norm;
            originalValues[vertice] = normal[2] / norm;
        }

        void LandscapeNode::SetupTerrainTexture(){
            for (int x = 0; x < depth; ++x){
                for (int z = 0; z < width; ++z){
                    CalcTexCoords(x, z);
                }
            }
        }

        void LandscapeNode::CalcTexCoords(int x, int z){
            int entry = CoordToEntry(x, z) * TEXCOORDS;

            texCoords[entry+1] = x * texDetail;
            texCoords[entry] = z * texDetail;
            /*
            texCoords[entry+1] = (x * texDetail) / (float)depth;
            texCoords[entry] = (z * texDetail) / (float)width;
            */
        }

        void LandscapeNode::SetupGeoMorphing(){
            for (int LOD = 1; LOD < pow(2, LandscapePatchNode::MAX_LODS-1); LOD *= 2){
                for (int x = 0; x < depth; x += LOD){
                    for (int z = 0; z < width; z += LOD){
                        int entry = CoordToEntry(x, z);
                        verticeLOD[entry] = LOD;
                    }
                }
            }

            for (int x = 0; x < depth; ++x){
                for (int z = 0; z < width; ++z){
                    CalcGeoMorphing(x, z);
                }
            }
        }

        void LandscapeNode::CalcGeoMorphing(int x, int z){
            int entry = CoordToEntry(x, z);
            int LOD = verticeLOD[entry];
            
            int placementX = x % (LOD * 2);
            int placementZ = z % (LOD * 2);

            if (placementX == LOD && placementZ == 0){
                // vertical line
                int entryAbove = CoordToEntry(x + LOD, z);
                int entryBelow = CoordToEntry(x - LOD, z);
                
                morphedValues[entry * 4] = (originalValues[entryAbove * 4] + originalValues[entryBelow * 4]) / 2 - originalValues[entry * 4];
                morphedValues[entry * 4 + 1] = (originalValues[entryAbove * 4 + 1] + originalValues[entryBelow * 4 + 1]) / 2 - originalValues[entry * 4 + 1];
                morphedValues[entry * 4 + 2] = (originalValues[entryAbove * 4 + 2] + originalValues[entryBelow * 4 + 2]) / 2 - originalValues[entry * 4 + 2];
                morphedValues[entry * 4 + 3] = (originalValues[entryAbove * 4 + 3] + originalValues[entryBelow * 4 + 3]) / 2 - originalValues[entry * 4 + 3];
            }else if(placementX == 0 && placementZ == LOD){
                // horizontal line
                int leftEntry = CoordToEntry(x, z - LOD);
                int rightEntry = CoordToEntry(x, z + LOD);
                
                morphedValues[entry * 4] = (originalValues[leftEntry * 4] + originalValues[rightEntry * 4]) / 2 - originalValues[entry * 4];
                morphedValues[entry * 4 + 1] = (originalValues[leftEntry * 4 + 1] + originalValues[rightEntry * 4 + 1]) / 2 - originalValues[entry * 4 + 1];
                morphedValues[entry * 4 + 2] = (originalValues[leftEntry * 4 + 2] + originalValues[rightEntry * 4 + 2]) / 2 - originalValues[entry * 4 + 2];
                morphedValues[entry * 4 + 3] = (originalValues[leftEntry * 4 + 3] + originalValues[rightEntry * 4 + 3]) / 2 - originalValues[entry * 4 + 3];
            }else if(placementX == LOD && placementZ == LOD){
                // diagonal line
                int entryAbove = CoordToEntry(x + LOD, z + LOD);
                int entryBelow = CoordToEntry(x - LOD, z - LOD);

                morphedValues[entry * 4] = (originalValues[entryAbove * 4] + originalValues[entryBelow * 4]) / 2 - originalValues[entry * 4];                
                morphedValues[entry * 4 + 1] = (originalValues[entryAbove * 4 + 1] + originalValues[entryBelow * 4 + 1]) / 2 - originalValues[entry * 4 + 1];                
                morphedValues[entry * 4 + 2] = (originalValues[entryAbove * 4 + 2] + originalValues[entryBelow * 4 + 2]) / 2 - originalValues[entry * 4 + 2];                
                morphedValues[entry * 4 + 3] = (originalValues[entryAbove * 4 + 3] + originalValues[entryBelow * 4 + 3]) / 2 - originalValues[entry * 4 + 3];                
            }else{
                // Highest LOD so no morphing
                //logger.info << "LOD " << LOD << " with coords (" << x << ", " << z << ")" << logger.end;
            }
        }

        void LandscapeNode::CalcLODSwitchDistances(){
            int maxLods = LandscapePatchNode::MAX_LODS;
            lodDistanceSquared = new float[maxLods + 1];
            
            lodDistanceSquared[0] = 0;
            for (int i = 1; i < maxLods + 1; ++i){
                float distance = baseDistance + (i-1) * incrementalDistance;
                lodDistanceSquared[i] = distance * distance;
            }
        }

        void LandscapeNode::EntryToCoord(int entry, int &x, int &z) const{
            x = entry / width;
            z = entry % width;
        }

        int LandscapeNode::CoordToEntry(int x, int z) const{
            return z + x * width;
        }

        GLfloat LandscapeNode::XCoord(int x, int z) const{
            return vertices[CoordToEntry(x, z) * DIMENSIONS];
        }

        GLfloat LandscapeNode::YCoord(int x, int z) const{
            return originalValues[CoordToEntry(x, z) * 4 + 3];
        }

        GLfloat LandscapeNode::ZCoord(int x, int z) const{
            return vertices[CoordToEntry(x, z) * DIMENSIONS + 2];
        }

        int LandscapeNode::LODLevel(int x, int z) const {
            int entry = CoordToEntry(x, z);
            return verticeLOD[entry];
        }

        void LandscapeNode::CalcSurroundingGeoMorphing(const int x, const int z){
            for (int affectOffset = LODLevel(x, z); affectOffset >= 1; affectOffset /= 2){
                if (x - affectOffset >= 0)
                    CalcGeoMorphing(x - affectOffset, z);
                if (x + affectOffset < depth)
                    CalcGeoMorphing(x + affectOffset, z);
                if (z - affectOffset >= 0)
                    CalcGeoMorphing(x, z - affectOffset);
                if (z + affectOffset < width)
                    CalcGeoMorphing(x, z + affectOffset);
                if (x - affectOffset >= 0 && z - affectOffset >= 0)
                    CalcGeoMorphing(x - affectOffset, z - affectOffset);
                if (x + affectOffset < depth && z + affectOffset < width)
                    CalcGeoMorphing(x + affectOffset, z + affectOffset);
            }
            
        }
    }
}

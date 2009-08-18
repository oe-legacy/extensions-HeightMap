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
            initialized = false;
            
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
                            // Fill the color array form the texture
                            unsigned int r = colors[c++] = data[i++];
                            unsigned int g = colors[c++] = data[i++];
                            unsigned int b = colors[c++] = data[i++];
                            
                            // Fill the vertex array
                            float height;
                            if (numberOfCharsPrColor == 4)
                                height = (float)data[i++];
                            else
                                height = (r + g + b) / 3;                            
                            vertices[v++] = widthScale * x;
                            vertices[v++] = heightScale * height;
                            vertices[v++] = widthScale * z;
                        }else{
                            // Fill the color array with black
                            colors[c++] = 0;
                            colors[c++] = 0;
                            colors[c++] = 0;

                            // Place the vertex at the bottom.
                            vertices[v++] = widthScale * x;
                            vertices[v++] = 0;
                            vertices[v++] = widthScale * z;
                        }
                        e++; //increment the entry
                    }
                }else{
                    for (int z = 0; z < width; ++z){
                        // Fill the color array with black
                        colors[c++] = 0;
                        colors[c++] = 0;
                        colors[c++] = 0;
                        
                        // Place the vertex at the bottom.
                        vertices[v++] = widthScale * x;
                        vertices[v++] = 0;
                        vertices[v++] = widthScale * z;
                    }
                }
            }

            // Calculate the normals
            for (int x = 0; x < depth; ++x){
                for (int z = 0; z < width; ++z){
                    CalcNormal(x, z);
                }
            }            

            // Setup the terrain
            texDetail = 1;
            SetupTerrainTexture();

            // Create patches
            int squares = LandscapePatchNode::PATCH_EDGE_SQUARES;
            patchGridWidth = (width-1) / squares;
            patchGridDepth = (depth-1) / squares;
            numberOfPatches = patchGridWidth * patchGridDepth;
            patchNodes = new LandscapePatchNode[numberOfPatches];
            e = 0;
            for (int x = 0; x < depth - squares; x +=squares ){
                for (int z = 0; z < width - squares; z += squares){
                    patchNodes[e++] = LandscapePatchNode(x, z, this);
                }
            }

            // Link patches
            for (int x = 0; x < patchGridDepth; ++x){
                for (int z = 0; z < patchGridWidth; ++z){
                    int entry = z + x * patchGridWidth;
                    if (x + 1 < patchGridDepth) 
                        patchNodes[entry].SetUpperNeighbor(&patchNodes[entry + patchGridWidth]);
                    if (z + 1 < patchGridWidth) 
                        patchNodes[entry].SetRightNeighbor(&patchNodes[entry + 1]);
                }
            }

            SetLODSwitchDistance(100, 100);

            SetupGeoMorphing();
        }

        LandscapeNode::~LandscapeNode(){
            if (vertices) delete[] vertices;
            if (colors) delete[] colors;
            if (normals) delete[] normals;
            if (patchNodes) delete[] patchNodes;
        }

        void LandscapeNode::Initialize(){
            if (landscapeShader) landscapeShader->Load();
            for (ShaderTextureMap::iterator itr = landscapeShader->textures.begin(); 
                 itr != landscapeShader->textures.end(); itr++)
                TextureLoader::LoadTextureResource( (*itr).second );
            initialized = true;
        }

        void LandscapeNode::CalcLOD(IViewingVolume* view){
            for (int i = 0; i < numberOfPatches; ++i)
                patchNodes[i].CalcLOD(view);
        }

        void LandscapeNode::RenderPatches(){
            for (int i = 0; i < numberOfPatches; ++i)
                patchNodes[i].Render();
        }

        void LandscapeNode::RenderNormals(){
            for (int i = 0; i < numberOfPatches; ++i)
                patchNodes[i].RenderNormals();
        }

        void LandscapeNode::GetCoords(int index, float &x, float &y, float &z) const{
            int i = index * DIMENSIONS;
            x = vertices[i++];
            y = vertices[i++];
            z = vertices[i];
        }

        void LandscapeNode::GetYCoord(int index, float &y) const{
            int i = index * DIMENSIONS;
            y = vertices[i+1];
        }

        void LandscapeNode::GeoMorphCoord(int x, int z, int LOD, float scale){
            int entry = CoordToEntry(x, z);
            float vertexLOD = verticeLOD[entry];
            if (LOD >= vertexLOD){
                normals[entry * DIMENSIONS] = morphedValues[entry * 4] * scale + originalValues[entry * 4];
                normals[entry * DIMENSIONS + 1] = morphedValues[entry * 4 + 1] * scale + originalValues[entry * 4 + 1];
                normals[entry * DIMENSIONS + 2] = morphedValues[entry * 4 + 2] * scale + originalValues[entry * 4 + 2];
                vertices[entry * DIMENSIONS + 1] = morphedValues[entry * 4 + 3] * scale + originalValues[entry * 4 + 3];
            }
        }

        void LandscapeNode::SetTextureDetail(int pixelsPrEdge){
            texDetail = pixelsPrEdge;
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
            incrementalDistance = dec;
            CalcLODSwitchDistances();
        }

        // **** inline functions ****

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

            int vertice = CoordToEntry(x, z) * DIMENSIONS;
            normals[vertice++] = normal[0] / norm;
            normals[vertice++] = normal[1] / norm;
            normals[vertice] = normal[2] / norm;
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

            texCoords[entry+1] = (x * texDetail) / (float)depth;
            texCoords[entry] = (z * texDetail) / (float)width;
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
                    int entry = CoordToEntry(x, z);
                    originalValues[entry * 4] = normals[entry * DIMENSIONS];
                    originalValues[entry * 4 + 1] = normals[entry * DIMENSIONS + 1];
                    originalValues[entry * 4 + 2] = normals[entry * DIMENSIONS + 2];
                    originalValues[entry * 4 + 3] = YCoord(x, z);
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
            return vertices[CoordToEntry(x, z) * DIMENSIONS + 1];
        }

        GLfloat LandscapeNode::ZCoord(int x, int z) const{
            return vertices[CoordToEntry(x, z) * DIMENSIONS + 2];
        }

        int LandscapeNode::LODLevel(int x, int z) const {
            int entry = CoordToEntry(x, z);
            return verticeLOD[entry];
        }

        void LandscapeNode::SetYCoord(const int x, const int z, float value){
            vertices[CoordToEntry(x, z) * DIMENSIONS + 1] = value;
        }
    }
}

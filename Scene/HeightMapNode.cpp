// Landscape node.
// -------------------------------------------------------------------
// Copyright (C) 2009 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Scene/HeightMapNode.h>
#include <Scene/HeightMapPatch.h>
#include <Resources/IShaderResource.h>
#include <Math/Math.h>
#include <Meta/OpenGL.h>
#include <Utils/TerrainUtils.h>
#include <Display/IViewingVolume.h>
#include <Display/Viewport.h>
#include <Geometry/GeometrySet.h>

#include <Logging/Logger.h>

#include <algorithm>
#include <cstring>

#define USE_PATCHES true

using namespace OpenEngine::Display;

namespace OpenEngine {
    namespace Scene {

        HeightMapNode::HeightMapNode(FloatTexture2DPtr tex)
            : tex(tex) {
            tex->Load();
            heightScale = 1;
            widthScale = 1;
            waterlevel = 10;
            offset = Vector<3, float>(0, 0, 0);
            
            texDetail = 1;
            baseDistance = 1;
            invIncDistance = 1.0f / 100.0f;

            isLoaded = false;

            texCoordBuffer.reset();

            landscapeShader.reset();
        }

        HeightMapNode::~HeightMapNode(){
            delete [] normals;
            delete [] deltaValues;

            delete [] patchNodes;
        }
        
        void HeightMapNode::Load() {
            if (isLoaded)
                return;
            InitArrays();
            if (USE_PATCHES)
                SetupPatches();
            else
                ComputeIndices();
            isLoaded = true;
        }

        void HeightMapNode::CalcLOD(IViewingVolume* view){
            if (USE_PATCHES)
                for (int i = 0; i < numberOfPatches; ++i)
                    patchNodes[i]->CalcLOD(view);
        }

        void HeightMapNode::Render(Viewport view){
            PreRender(view);

            if (USE_PATCHES){
                // Draw patches front to back.
                Vector<3, float> dir = view.GetViewingVolume()->GetDirection().RotateVector(Vector<3, float>(0,0,1));

                int xStart, xEnd, xStep, zStart, zEnd, zStep;
                if (dir[0] < 0){
                    // If we're looking along the x-axis.
                    xStart = 0;
                    xEnd = patchGridWidth;
                    xStep = 1;
                }else{
                    // else iterate form the other side.
                    xStart = patchGridWidth-1;
                    xEnd = -1;
                    xStep = -1;
                }
                if (dir[2] < 0){
                    // If we're looking along the z-axis.
                    zStart = 0;
                    zEnd = patchGridDepth;
                    zStep = 1;
                }else{
                    // else iterate form the other side.
                    zStart = patchGridDepth-1;
                    zEnd = -1;
                    zStep = -1;
                }
                
                for (int x = xStart; x != xEnd ; x += xStep){
                    for (int z = zStart; z != zEnd; z += zStep){
                        patchNodes[z + x * patchGridDepth]->Render();
                    }
                }
            }else{
                if (indexBuffer->GetID() != 0)
                    glDrawElements(GL_TRIANGLE_STRIP, indexBuffer->GetSize(), GL_UNSIGNED_INT, 0);
                else
                    glDrawElements(GL_TRIANGLE_STRIP, indexBuffer->GetSize(), GL_UNSIGNED_INT, indexBuffer->GetVoidDataPtr());
            }

            PostRender(view);

            /*
            landscapeShader->ReleaseShader();
            glBegin(GL_LINES);
            for (int x = 0; x < width; ++x){
                for (int z = 0; z < depth; ++z){
                    float* v = GetVertice(x, z);
                    float* n = GetNormals(x, z);
                    glColor3f(1, 0, 0);
                    glVertex3fv(v);
                    glVertex3f(v[0] + n[0], v[1] + n[1], v[2] + n[2]);
                    glColor3f(0, 1, 0);
                    glVertex3fv(v);
                    glVertex3f(v[0] - n[1] / 2, v[1] + n[0], v[2]);
                    glColor3f(0, 0, 1);
                    glVertex3fv(v);
                    glVertex3f(v[0], v[1] + n[2], v[2] - n[1] / 2);
                }
            }
            glEnd();
            */
        }

        void HeightMapNode::RenderBoundingGeometry(){
            if (USE_PATCHES)
                for (int i = 0; i < numberOfPatches; ++i)
                    patchNodes[i]->RenderBoundingGeometry();
        }

        void HeightMapNode::VisitSubNodes(ISceneNodeVisitor& visitor){
            list<ISceneNode*>::iterator itr;
            for (itr = subNodes.begin(); itr != subNodes.end(); ++itr){
                (*itr)->Accept(visitor);
            }
        }

        void HeightMapNode::Handle(RenderingEventArg arg){
            Initialize(arg);

            Load();

            // Create vbos
            arg.renderer.BindDataBlock(vertexBuffer.get());
            arg.renderer.BindDataBlock(texCoordBuffer.get());
            arg.renderer.BindDataBlock(indexBuffer.get());

            if (landscapeShader != NULL) {
                // Init shader used buffer objects

                // Create the image to hold the normal map
                normalmap = FloatTexture2DPtr(new Texture2D<float>(width, depth, 3, normals));
                normalmap->SetColorFormat(RGB32F);
                normalmap->SetMipmapping(false);
                normalmap->SetCompression(false);
                landscapeShader->SetTexture("normalMap", (ITexture2DPtr)normalmap);

                // Geomorph values buffer object
                arg.renderer.BindDataBlock(geomorphBuffer.get());

                // normal map Coord buffer object
                arg.renderer.BindDataBlock(normalMapCoordBuffer.get());

                IDataBlockList texCoords;
                texCoords.push_back(texCoordBuffer);
                texCoords.push_back(normalMapCoordBuffer);
                geom = GeometrySetPtr(new GeometrySet(vertexBuffer, geomorphBuffer, texCoords));

                landscapeShader->Load();
                TextureList texs = landscapeShader->GetTextures();
                for (unsigned int i = 0; i < texs.size(); ++i)
                    arg.renderer.LoadTexture(texs[i].get());
            }else{
                // Create a non shader geometry set
                IDataBlockList texCoords;
                texCoords.push_back(texCoordBuffer);
                normalBuffer = Float3DataBlockPtr(new DataBlock<3, float>(numberOfVertices, normals));
                normalBuffer->SetUnloadPolicy(UNLOAD_EXPLICIT);
                arg.renderer.BindDataBlock(normalBuffer.get());
                geom = GeometrySetPtr(new GeometrySet(vertexBuffer, normalBuffer, texCoords));
            }

            SetLODSwitchDistance(baseDistance, 1 / invIncDistance);
        }

        void HeightMapNode::Handle(ProcessEventArg arg){
            Process(arg);
        }
        
        // **** Get/Set methods ****

        float HeightMapNode::GetHeight(Vector<3, float> point) const{
            return GetHeight(point[0], point[2]);
        }
        float HeightMapNode::GetHeight(float x, float z) const{
            /**
             * http://en.wikipedia.org/wiki/Bilinear_interpolation
             */
            
            // x and z normalized with respect to the scaling and
            // translation from offset.
            x = (x - offset.Get(0)) / widthScale;
            z = (z - offset.Get(2)) / widthScale;

            // The indices into the array
            int X = floor(x);
            int Z = floor(z);
            
            float dX = x - X;
            float dZ = z - Z;

            // Bilinear interpolation of the heights.
            float height = GetVertice(X, Z)[1] * (1-dX) * (1-dZ) +
                           GetVertice(X+1, Z)[1] * dX * (1-dZ) +
                           GetVertice(X, Z+1)[1] * (1-dX) * dZ +
                           GetVertice(X+1, Z+1)[1] * dX * dZ;
            
            return height;
        }

        Vector<3, float> HeightMapNode::GetNormal(Vector<3, float> point) const{
            return GetNormal(point[0], point[2]);
        }
        Vector<3, float> HeightMapNode::GetNormal(float x, float z) const{
            /**
             * http://en.wikipedia.org/wiki/Bilinear_interpolation
             */
            
            // x and z normalized with respect to the scaling and
            // translation from offset.
            x = (x - offset.Get(0)) / widthScale;
            z = (z - offset.Get(2)) / widthScale;

            // The indices into the array
            int X = floor(x);
            int Z = floor(z);
            
            float dX = x - X;
            float dZ = z - Z;

            // Bilinear interpolation of the heights.
            Vector<3, float> normal = Vector<3, float>(GetNormals(X, Z)) * (1-dX) * (1-dZ) +
                Vector<3, float>(GetNormals(X+1, Z)) * dX * (1-dZ) +
                Vector<3, float>(GetNormals(X, Z+1)) * (1-dX) * dZ +
                Vector<3, float>(GetNormals(X+1, Z+1)) * dX * dZ;
            
            return normal.GetNormalize();
        }

        Vector<3, float> HeightMapNode::GetReflectedDirection(Vector<3, float> point, Vector<3, float> dir) const{
            return GetReflectedDirection(point[0], point[2], dir);
        }
        Vector<3, float> HeightMapNode::GetReflectedDirection(float x, float z, Vector<3, float> dir) const{
            /**
             * http://www.lighthouse3d.com/opengl/glsl/index.php?ogldir2
             */

            Vector<3, float> normal = GetNormal(x, z);
            Vector<3, float> direction = dir.GetNormalize();

            Vector<3, float> reflect = -2.0f * normal * (normal * direction) + direction;
            
            return reflect;
        }

        int HeightMapNode::GetIndice(int x, int z){
            return CoordToIndex(x, z);
        }

        float* HeightMapNode::GetVertex(int x, int z){
            if (x < 0)
                x = 0;
            else if (x >= width)
                x = width - 1;

            if (z < 0)
                z = 0;
            else if (z >= depth)
                z = depth - 1;
            return GetVertice(x, z);
        }

        void HeightMapNode::SetVertex(int x, int z, float value){
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer->GetID());
            float* vbo = (float*) glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

            // Update height for the moved vertice affected.
            int index = CoordToIndex(x, z);
            vbo[index * DIMENSIONS + 1] = GetVertice(index)[1] = value;
            vbo[index * DIMENSIONS + 3] = GetVertice(index)[3] = CalcGeomorphHeight(x, z);

            // Update morphing height for all surrounding affected
            // vertices.
            for (int delta = GetVerticeDelta(index) / 2; delta >= 1; delta /= 2){
                if (0 <= x-delta){
                    index = CoordToIndex(x-delta, z);
                    vbo[index * DIMENSIONS + 3] = GetVertice(index)[3] = CalcGeomorphHeight(x-delta, z);
                }
                
                if (x+delta < width){
                    index = CoordToIndex(x+delta, z);
                    vbo[index * DIMENSIONS + 3] = GetVertice(index)[3] = CalcGeomorphHeight(x+delta, z);
                }
                
                if (0 <= z-delta){
                    index = CoordToIndex(x, z-delta);
                    vbo[index * DIMENSIONS + 3] = GetVertice(index)[3] = CalcGeomorphHeight(x, z-delta);
                }

                if (z+delta < depth){
                    index = CoordToIndex(x, z+delta);
                    vbo[index * DIMENSIONS + 3] = GetVertice(index)[3] = CalcGeomorphHeight(x, z+delta);
                }

                if (0 <= x-delta && 0 <= z-delta){
                    index = CoordToIndex(x-delta, z-delta);
                    vbo[index * DIMENSIONS + 3] = GetVertice(index)[3] = CalcGeomorphHeight(x-delta, z-delta);
                }

                if (x+delta < width && z+delta < depth){
                    index = CoordToIndex(x+delta, z+delta);
                    vbo[index * DIMENSIONS + 3] = GetVertice(index)[3] = CalcGeomorphHeight(x+delta, z+delta);
                }
            }

            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // Update shadows
            /*
            int shadowLeft = x < 2 ? 0 : x - 1;
            int shadowRight = x + 3 > width ? width : x + 2;
            int shadowBelow = z < 2 ? 0 : z - 1;
            int shadowAbove = z + 3 > depth ? depth : z + 2;

            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, normalsBufferId);
            float* pbo = (float*) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
            int pboIndex = 0;
            for (int xi = shadowLeft; xi < shadowRight; ++xi)
                for (int zi = shadowBelow; zi < shadowAbove; ++zi){
                    Vector<3, float> normal = GetNormal(xi, zi);
                    normal.ToArray(GetNormals(xi, zi));
                    normal.ToArray(pbo + pboIndex);
                    pboIndex += 3;
                }

            glBindTexture(GL_TEXTURE_2D, normalmap->GetID());

            glTexSubImage2D(GL_TEXTURE_2D, 0, shadowLeft, shadowBelow, 
                            shadowRight - shadowLeft, shadowAbove - shadowBelow, 
                            GL_RGB, GL_FLOAT, 0);

            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);            
            */

            // Update bounding box
            HeightMapPatch* mainNode = GetPatch(x, z);
            mainNode->UpdateBoundingGeometry(value);
            HeightMapPatch* upperNode = GetPatch(x+1, z);
            if (upperNode != mainNode) upperNode->UpdateBoundingGeometry(value);
            HeightMapPatch* rightNode = GetPatch(x, z+1);
            if (rightNode != mainNode) rightNode->UpdateBoundingGeometry(value);
            HeightMapPatch* upperRightNode = GetPatch(x+1, z+1);
            if (upperRightNode != mainNode) upperRightNode->UpdateBoundingGeometry(value);

        }

        void HeightMapNode::SetVertices(int x, int z, int w, int d, float* values){

            //          Above
            //        z
            //        A
            //        |
            // Left   |           Right
            //        |
            //        |
            //       -+-------> x 
            // 
            //          Below

            // if the area is outside the heightmap
            if (x >= width || z >= depth || x + w <= 0 || z + d <= 0) return;

            // Update the height for the moved vertices
            int xStart = x < 0 ? 0 : x;
            int zStart = z < 0 ? 0 : z;
            int xEnd = (x + w >= width) ? width : x + w;
            int zEnd = (z + d >= depth) ? depth : z + d;
            
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer->GetID());
            float* vbo = (float*) glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
            for (int xi = xStart; xi < xEnd; ++xi)
                for (int zi = zStart; zi < zEnd; ++zi){
                    int index = CoordToIndex(xi, zi);
                    vbo[index * DIMENSIONS + 1] = GetVertice(index)[1] = values[(zi - z) + (xi - x) * d];
                }

            // Update the morphing height for all affected vertices
            int morphLeft = xStart - HeightMapPatch::MAX_DELTA < 0 ? 0 : xStart - HeightMapPatch::MAX_DELTA;
            int morphRight = xEnd + HeightMapPatch::MAX_DELTA > width ? width : xEnd + HeightMapPatch::MAX_DELTA;
            int morphBelow = zStart - HeightMapPatch::MAX_DELTA < 0 ? 0 : zStart - HeightMapPatch::MAX_DELTA;;
            int morphAbove = zEnd + HeightMapPatch::MAX_DELTA > depth ? depth : zEnd + HeightMapPatch::MAX_DELTA;

            for (int xi = morphLeft; xi < morphRight; ++xi)
                for (int zi = morphBelow; zi < morphAbove; ++zi){
                    int index = CoordToIndex(xi, zi);
                    vbo[index * DIMENSIONS + 3] = GetVertice(index)[3] = CalcGeomorphHeight(xi, zi);
                }

            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // Update the shadows
            /*
            int shadowLeft = xStart < 2 ? 0 : xStart - 1;
            int shadowRight = xEnd + 3 > width ? width : xEnd + 2;
            int shadowBelow = zStart < 2 ? 0 : zStart - 1;
            int shadowAbove = zEnd + 3 > depth ? depth : zEnd + 2;

            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, normalsBufferId);
            float* pbo = (float*) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
            int pboIndex = 0;
            for (int xi = shadowLeft; xi < shadowRight; ++xi)
                for (int zi = shadowBelow; zi < shadowAbove; ++zi){
                    Vector<3, float> normal = GetNormal(xi, zi);
                    normal.ToArray(GetNormals(xi, zi));
                    normal.ToArray(pbo + pboIndex);
                    pboIndex += 3;
                }

            glBindTexture(GL_TEXTURE_2D, normalmap->GetID());

            glTexSubImage2D(GL_TEXTURE_2D, 0, shadowLeft, shadowBelow, 
                            shadowRight - shadowLeft, shadowAbove - shadowBelow, 
                            GL_RGB, GL_FLOAT, 0);

            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
            */

            // Update the bounding geometry
            int patchSize = HeightMapPatch::PATCH_EDGE_SQUARES;
            int xBoundingStart = (xStart / patchSize) * patchSize;
            int zBoundingStart = (zStart / patchSize) * patchSize;
            for (int xi = xBoundingStart; xi < xEnd; xi += patchSize)
                for (int zi = zBoundingStart; zi < zEnd; zi += patchSize){
                    GetPatch(xi, zi)->UpdateBoundingGeometry();
                }
        }

        Vector<3, float> HeightMapNode::GetNormal(int x, int z){
            
            Vector<3, float> normal = Vector<3, float>(0.0f);
            float vHeight = GetVertice(x, z)[1];

            // Right vertex
            if (x + 1 < width){
                float wHeight = GetVertice(x + 1, z)[1];
                normal[0] += vHeight - wHeight;
                normal[1] += widthScale;
            }
            
            // Left vertex
            if (0 < x){
                float wHeight = GetVertice(x - 1, z)[1];
                normal[0] += wHeight - vHeight;
                normal[1] += widthScale;
            }

            // upper vertex
            if (z + 1 < depth){
                float wHeight = GetVertice(x, z + 1)[1];
                normal[2] += vHeight - wHeight;
                normal[1] += widthScale;
            }
            
            // Lower vertex
            if (0 < z){
                float wHeight = GetVertice(x, z - 1)[1];
                normal[2] += wHeight - vHeight;
                normal[1] += widthScale;
            }

            normal.Normalize();

            return normal;
            
        }
        
        /**
         * Set the distance at which the LOD should switch.
         *
         * @ base The base distance to the camera where the LOD is the highest.
         * @ dec The distance between each decrement in LOD.
         */
        void HeightMapNode::SetLODSwitchDistance(float base, float dec){
            baseDistance = base;
            
            float edgeLength = HeightMapPatch::PATCH_EDGE_SQUARES * widthScale;
            if (dec * dec < edgeLength * edgeLength * 2){
                invIncDistance = 1.0f / sqrt(edgeLength * edgeLength * 2);
                logger.error << "Incremental LOD distance is too low, setting it to lowest value: " << 1.0f / invIncDistance << logger.end;
            }else
                invIncDistance = 1.0f / dec;

            // Update uniforms
            if (landscapeShader != NULL){
                landscapeShader->SetUniform("baseDistance", baseDistance);
                landscapeShader->SetUniform("invIncDistance", invIncDistance);
            }
        }

        void HeightMapNode::SetTextureDetail(const float detail){
            texDetail = detail;
            if (texCoordBuffer != NULL)
                SetupTerrainTexture();
        }

        // **** inline functions ****

        void HeightMapNode::InitArrays(){
            int texWidth = tex->GetHeight();
            int texDepth = tex->GetWidth();

            // if texwidth/depth isn't expressible as n * patchwidth + 1 fix it.
            int patchWidth = HeightMapPatch::PATCH_EDGE_SQUARES;
            int widthRest = (texWidth - 1) % patchWidth;
            width = widthRest ? texWidth + patchWidth - widthRest : texWidth;

            int depthRest = (texDepth - 1) % patchWidth;
            depth = depthRest ? texDepth + patchWidth - depthRest : texDepth;

            numberOfVertices = width * depth;

            vertexBuffer = Float4DataBlockPtr(new DataBlock<4, float>(numberOfVertices));
            vertexBuffer->SetUnloadPolicy(UNLOAD_EXPLICIT);
            normals = new float[numberOfVertices * 3];
            texCoordBuffer = Float2DataBlockPtr(new DataBlock<2, float>(numberOfVertices));
            normalMapCoordBuffer = Float2DataBlockPtr(new DataBlock<2, float>(numberOfVertices));
            geomorphBuffer = Float3DataBlockPtr(new DataBlock<3, float>(numberOfVertices));
            deltaValues = new char[numberOfVertices];

            // Fill the vertex array
            for (int x = 0; x < width; ++x){
                //for (int x = width-1; x >= 0; --x){
                for (int z = 0; z < depth; ++z){
                    //for (int z = depth-1; z >= 0; --z){
                    float* vertice = GetVertice(x, z);
                     
                    vertice[0] = widthScale * x + offset[0];
                    vertice[2] = widthScale * z + offset[2];
                    vertice[3] = 1;
       
                    if (x < texWidth && z < texDepth){
                        // inside the heightmap
                        float height = tex->GetPixel(x, z)[0];
                        vertice[1] = height * heightScale - waterlevel - heightScale / 2 + offset[1];
                    }else{
                        // outside the heightmap, set height to waterlevel
                        vertice[1] = -waterlevel - heightScale / 2 + offset[1];
                    }
                }
            }

            // Release the heightmap.
            tex.reset();
            
            SetupNormalMap();
            SetupTerrainTexture();

            if (landscapeShader != NULL){
                CalcVerticeLOD();
                for (int x = 0; x < width; ++x)
                    for (int z = 0; z < depth; ++z){
                        // Store the morphing value in the w-coord to
                        // use in the shader.
                        float* vertice = GetVertice(x, z);
                        vertice[3] = CalcGeomorphHeight(x, z);
                    }
            }
        }

        void HeightMapNode::SetupNormalMap(){
            for (int x = 0; x < width; ++x)
                for (int z = 0; z < depth; ++z){
                    float* coord = GetNormalMapCoord(x, z);
                    coord[1] = (x + 0.5f) / (float) width;
                    coord[0] = (z + 0.5f) / (float) depth;
                    Vector<3, float> normal = GetNormal(x, z);
                    normal.ToArray(GetNormals(x, z));
                }
        }

        void HeightMapNode::SetupTerrainTexture(){
            for (int x = 0; x < width; ++x){
                for (int z = 0; z < depth; ++z){
                    CalcTexCoords(x, z);
                }
            }
        }

        void HeightMapNode::CalcTexCoords(int x, int z){
            float* texCoord = GetTexCoord(x, z);
            texCoord[1] = x * texDetail;
            texCoord[0] = z * texDetail;
        }

        void HeightMapNode::CalcVerticeLOD(){
            for (int LOD = 1; LOD <= HeightMapPatch::MAX_LODS; ++LOD){
                int delta = pow(2, LOD-1);
                for (int x = 0; x < width; x += delta){
                    for (int z = 0; z < depth; z += delta){
                        GetVerticeLOD(x, z) = LOD;
                        GetVerticeDelta(x, z) = pow(2, LOD-1);
                    }
                }
            }
        }

        float HeightMapNode::CalcGeomorphHeight(int x, int z){
            if (landscapeShader == NULL)
                return 1.0f;
            else{
                short delta = GetVerticeDelta(x, z);
                
                int dx, dz;
                if (delta < HeightMapPatch::MAX_DELTA){
                    dx = x % (delta * 2);
                    dz = z % (delta * 2);
                }else{
                    dx = 0;
                    dz = 0;
                }
                
                float* vertice = GetVertice(x, z);
                float* verticeNeighbour1 = GetVertice(x + dx, z + dz);
                float* verticeNeighbour2 = GetVertice(x - dx, z - dz);
                
                return (verticeNeighbour1[1] + verticeNeighbour2[1]) / 2 - vertice[1];
            }
        }

        void HeightMapNode::ComputeIndices(){
            int LOD = 4;
            int xs = (width-1) / LOD + 1;
            int zs = (depth-1) / LOD + 1;
            unsigned int numberOfIndices = 2 * ((xs - 1) * zs + xs - 2);

            indexBuffer = IndicesPtr(new Indices(numberOfIndices));
            unsigned int* indices = indexBuffer->GetData();

            unsigned int i = 0;
            for (int x = 0; x < width - 1; x += LOD){
                for (int z = depth - 1; z >= 0; z -= LOD){
                    indices[i++] = CoordToIndex(x, z);
                    indices[i++] = CoordToIndex(x+LOD, z);
                }
                if (x < depth - 1 - LOD){
                    indices[i++] = CoordToIndex(x+LOD, 0);
                    indices[i++] = CoordToIndex(x+LOD, width - 1);
                }
            }
        }

        void HeightMapNode::SetupPatches(){
            // Create the patches
            int squares = HeightMapPatch::PATCH_EDGE_SQUARES;
            patchGridWidth = (width-1) / squares;
            patchGridDepth = (depth-1) / squares;
            numberOfPatches = patchGridWidth * patchGridDepth;
            patchNodes = new HeightMapPatch*[numberOfPatches];
            int entry = 0;
            for (int x = 0; x < width - squares; x +=squares ){
                for (int z = 0; z < depth - squares; z += squares){
                    patchNodes[entry++] = new HeightMapPatch(x, z, this);
                }
            }

            // Setup indice buffer
            unsigned int numberOfIndices = 0;
            for (int p = 0; p < numberOfPatches; ++p){
                for (int l = 0; l < HeightMapPatch::MAX_LODS; ++l){
                    for (int rl = 0; rl < 3; ++rl){
                        for (int ul = 0; ul < 3; ++ul){
                            LODstruct& lod = patchNodes[p]->GetLodStruct(l,rl,ul);
                            lod.indiceBufferOffset = numberOfIndices;
                            numberOfIndices += lod.numberOfIndices;
                        }
                    }
                }
            }

            indexBuffer = IndicesPtr(new Indices(numberOfIndices));
            unsigned int* indices = indexBuffer->GetData();

            unsigned int i = 0;
            for (int p = 0; p < numberOfPatches; ++p){
                patchNodes[p]->SetDataIndices(indexBuffer);
                for (int l = 0; l < HeightMapPatch::MAX_LODS; ++l){
                    for (int rl = 0; rl < 3; ++rl){
                        for (int ul = 0; ul < 3; ++ul){
                            LODstruct& lod = patchNodes[p]->GetLodStruct(l,rl,ul);
                            memcpy(indices + i, lod.indices, sizeof(unsigned int) * lod.numberOfIndices);
                            i += lod.numberOfIndices;
                        }
                    }
                }
            }

            // Setup shader uniforms used in geomorphing
            if (landscapeShader != NULL){
                for (int x = 0; x < width - 1; ++x){
                    for (int z = 0; z < depth - 1; ++z){
                        HeightMapPatch* patch = GetPatch(x, z);
                        float* geomorph = GetGeomorphValues(x, z);
                        geomorph[0] = patch->GetCenter()[0];
                        geomorph[1] = patch->GetCenter()[2];
                    }
                }
            }
        }
        
        int HeightMapNode::CoordToIndex(const int x, const int z) const{
            return z + x * depth;
        }
        
        float* HeightMapNode::GetVertice(const int x, const int z) const{
            int index = CoordToIndex(x, z);
            return GetVertice(index);
        }

        float* HeightMapNode::GetVertice(const int index) const{
            return vertexBuffer->GetData() + index * vertexBuffer->GetDimension();
        }
        
        float* HeightMapNode::GetNormals(const int x, const int z) const{
            int index = CoordToIndex(x, z);
            return GetNormals(index);
        }

        float* HeightMapNode::GetNormals(const int index) const{
            return normals + index * 3;
        }

        float* HeightMapNode::GetTexCoord(const int x, const int z) const{
            int index = CoordToIndex(x, z);
            return texCoordBuffer->GetData() + index * texCoordBuffer->GetDimension();
        }

        float* HeightMapNode::GetNormalMapCoord(const int x, const int z) const{
            int index = CoordToIndex(x, z);
            return normalMapCoordBuffer->GetData() + index * normalMapCoordBuffer->GetDimension();
        }

        float* HeightMapNode::GetGeomorphValues(const int x, const int z) const{
            int index = CoordToIndex(x, z);
            return geomorphBuffer->GetData() + index * 3;
        }

        float& HeightMapNode::GetVerticeLOD(const int x, const int z) const{
            int index = CoordToIndex(x, z);
            return GetVerticeLOD(index);
        }

        float& HeightMapNode::GetVerticeLOD(const int index) const{
            return (geomorphBuffer->GetData() + index * 3)[2];
        }

        char& HeightMapNode::GetVerticeDelta(const int x, const int z) const{
            int index = CoordToIndex(x, z);
            return GetVerticeDelta(index);
        }

        char& HeightMapNode::GetVerticeDelta(const int index) const{
            return deltaValues[index];
        }

        int HeightMapNode::GetPatchIndex(const int x, const int z) const{
            int patchX = (x-1) / HeightMapPatch::PATCH_EDGE_SQUARES;
            int patchZ = (z-1) / HeightMapPatch::PATCH_EDGE_SQUARES;
            return patchZ + patchX * patchGridDepth;
        }

        HeightMapPatch* HeightMapNode::GetPatch(const int x, const int z) const{
            int index = GetPatchIndex(x, z);
            return patchNodes[index];
        }

    }
}

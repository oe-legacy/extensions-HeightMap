// Grass node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Scene/GrassNode.h>

#include <Geometry/Mesh.h>
#include <Geometry/GeometrySet.h>
#include <Resources/IShaderResource.h>
#include <Resources/DataBlock.h>
#include <Resources/Texture2D.h>
#include <Math/RandomGenerator.h>

#include <Scene/HeightMapNode.h>

#include <list>
using std::list;

#include <Logging/Logger.h>

using namespace OpenEngine::Geometry;
using namespace OpenEngine::Resources;

namespace OpenEngine {
    namespace Scene {

        GrassNode::GrassNode() {
            quadsPrObject = 3;
            grassShader.reset();
            heightmap = NULL;
            gridDim = straws = 0;
            elapsedTime = 0;
            grassGeom = CreateGrassObject();
        }

        GrassNode::GrassNode(HeightMapNode* heightmap, IShaderResourcePtr shader) 
            : heightmap(heightmap) {
            quadsPrObject = 3;
            grassShader = shader;
            gridDim = 128;
            straws = 12000;
            elapsedTime = 0;

            grassGeom = CreateGrassObject();
        }
        
        void GrassNode::Handle(RenderingEventArg arg){
            if (grassShader){
                ITexture2DPtr tex;
                grassShader->GetTexture("heightmap", tex);
                arg.renderer.LoadTexture(tex);

                Vector<2, float> heightmapDims(tex->GetWidth(),
                                               tex->GetHeight());
                grassShader->SetUniform("hmapDims", heightmapDims);

                grassShader->SetTexture("normalmap", heightmap->GetNormalMap());
                arg.renderer.LoadTexture(tex);

                Vector<2, float> normalmapDims(heightmap->GetNormalMap()->GetWidth(),
                                               heightmap->GetNormalMap()->GetHeight());
                grassShader->SetUniform("nmapDims", normalmapDims);

                grassShader->SetUniform("hmapOffset", heightmap->GetOffset());
                
                grassShader->SetUniform("gridDim", float(gridDim));
                grassShader->SetUniform("invGridDim", 1.0f / float(gridDim));

                grassShader->Load();

                grassShader->GetTexture("grassTex", tex);
                arg.renderer.LoadTexture(tex);
            }
        }

        void GrassNode::Handle(Core::ProcessEventArg arg){
            elapsedTime += arg.approx;
        }

        GeometrySetPtr GrassNode::CreateGrassObject() {
            RandomGenerator rand;
            rand.SeedWithTime();

            float radsPrQuad = PI / quadsPrObject;

            const float WIDTH = 3;
            const float HEIGHT = 3;

            const int texsPrQuad = 1;

            unsigned int blockSize = 4 * quadsPrObject * straws;

            Float3DataBlockPtr vertices = Float3DataBlockPtr(new DataBlock<3, float>(blockSize));
            Float3DataBlockPtr center = Float3DataBlockPtr(new DataBlock<3, float>(blockSize));
            Float2DataBlockPtr texCoords = Float2DataBlockPtr(new DataBlock<2, float>(blockSize));
            Float2DataBlockPtr noise = Float2DataBlockPtr(new DataBlock<2, float>(blockSize));

            int index = 0;
            for (int j = 0; j < straws; ++j){
                // Create geometry for the quads
                Vector<3, float> position = Vector<3, float>(rand.UniformFloat(0, gridDim), 0, 
                                                             rand.UniformFloat(0, gridDim));
                float rotationOffset = rand.UniformFloat(0, 2 * PI);
            
                for (int i = 0; i < quadsPrObject; ++i){
                    Vector<3, float> direction = Vector<3, float>(cos(i * radsPrQuad + rotationOffset),
                                                                  0, sin(i * radsPrQuad + rotationOffset));
                    
                    // lower left
                    vertices->SetElement(index, position + direction * WIDTH / 2 * texsPrQuad);
                    center->SetElement(index, position);
                    texCoords->SetElement(index, Vector<2, float>(0, 0));
                    ++index;
                        
                    // upper left
                    vertices->SetElement(index, position + direction * WIDTH / 2 * texsPrQuad + Vector<3, float>(0, HEIGHT, 0));
                    center->SetElement(index, position);
                    texCoords->SetElement(index, Vector<2, float>(0, 1));
                    ++index;
                        
                    // upper right
                    vertices->SetElement(index, position + direction * WIDTH / -2 * texsPrQuad + Vector<3, float>(0, HEIGHT, 0));
                    center->SetElement(index, position);
                    texCoords->SetElement(index, Vector<2, float>(1 * texsPrQuad, 1));
                    ++index;
                        
                    // lower right
                    vertices->SetElement(index, position + direction * WIDTH / -2 * texsPrQuad);
                    center->SetElement(index, position);
                    texCoords->SetElement(index, Vector<2, float>(1 * texsPrQuad, 0));
                    ++index;
                }
            }

            IDataBlockList tcs;
            tcs.push_back(texCoords);

            GeometrySet* geom = new GeometrySet(vertices, center, tcs);

            return GeometrySetPtr(geom);
        }

    }
}

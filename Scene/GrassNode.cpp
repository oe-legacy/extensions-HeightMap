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
            grassGeom = CreateGrassObject();
            grassShader.reset();
            heightmap = NULL;
            gridDim = 0;
            elapsedTime = 0;
        }

        GrassNode::GrassNode(HeightMapNode* heightmap, IShaderResourcePtr shader) 
            : heightmap(heightmap) {
            quadsPrObject = 3;
            grassShader = shader;
            gridDim = 64;
            elapsedTime = 0;

            grassGeom = CreateGrassObject();
        }
        
        void GrassNode::Handle(RenderingEventArg arg){
            grassGeom = CreateGrassObject();
            
            if (grassShader){
                ITexture2DPtr tex;
                grassShader->GetTexture("heightmap", tex);
                arg.renderer.LoadTexture(tex);
                grassShader->SetTexture("normalmap", heightmap->GetNormalMap());
                arg.renderer.LoadTexture(tex);

                grassShader->Load();

                grassShader->GetTexture("grassTex", tex);
                arg.renderer.LoadTexture(tex);
            }
        }

        void GrassNode::Handle(ProcessEventArg arg){
            elapsedTime +=  arg.approx;
        }

        GeometrySetPtr GrassNode::CreateGrassObject() {
            RandomGenerator rand;
            rand.SeedWithTime();

            float radsPrQuad = 2 * PI / quadsPrObject;
            float rotationOffset = PI / (quadsPrObject * 4);
            int halfDim = gridDim / 2;

            const float WIDTH = 3;
            const float HEIGHT = 3;

            unsigned int blockSize = 4 * quadsPrObject * gridDim * gridDim;

            Float3DataBlockPtr vertices = Float3DataBlockPtr(new DataBlock<3, float>(blockSize));
            Float3DataBlockPtr center = Float3DataBlockPtr(new DataBlock<3, float>(blockSize));
            Float2DataBlockPtr texCoords = Float2DataBlockPtr(new DataBlock<2, float>(blockSize));
            Float2DataBlockPtr noise = Float2DataBlockPtr(new DataBlock<2, float>(blockSize));

            int index = 0;
            for (int x = 0; x < gridDim; ++x)
                for (int z = 0; z < gridDim; ++z)
                    // Create geometry for the quads
                    for (int i = 0; i < quadsPrObject; ++i){
                        Vector<3, float> position = Vector<3, float>(x - halfDim, 0, z - halfDim);

                        Vector<3, float> direction = Vector<3, float>(cos(i * radsPrQuad + rotationOffset),
                                                                      0, sin(i * radsPrQuad + rotationOffset));
                        
                        // lower left
                        vertices->SetElement(index, position + direction * WIDTH / 2);
                        center->SetElement(index, position);
                        texCoords->SetElement(index, Vector<2, float>(0, 0));
                        noise->SetElement(index, Vector<2, float>(rand.UniformFloat(-0.5, 0.5),
                                                                  rand.UniformFloat(-0.5, 0.5)));
                        ++index;
                        
                        // upper left
                        vertices->SetElement(index, position + direction * WIDTH / 2 + Vector<3, float>(0, HEIGHT, 0));
                        center->SetElement(index, position);
                        texCoords->SetElement(index, Vector<2, float>(0, 1));
                        noise->SetElement(index, Vector<2, float>(rand.UniformFloat(-0.5, 0.5),
                                                                  rand.UniformFloat(-0.5, 0.5)));
                        ++index;
                        
                        // upper right
                        vertices->SetElement(index, position + direction * WIDTH / -2 + Vector<3, float>(0, HEIGHT, 0));
                        center->SetElement(index, position);
                        texCoords->SetElement(index, Vector<2, float>(1, 1));
                        noise->SetElement(index, Vector<2, float>(rand.UniformFloat(-0.5, 0.5),
                                                                  rand.UniformFloat(-0.5, 0.5)));
                        ++index;
                        
                        // lower right
                        vertices->SetElement(index, position + direction * WIDTH / -2);
                        center->SetElement(index, position);
                        texCoords->SetElement(index, Vector<2, float>(1, 0));
                        noise->SetElement(index, Vector<2, float>(rand.UniformFloat(-0.5, 0.5),
                                                                  rand.UniformFloat(-0.5, 0.5)));
                        ++index;
                    }

            IDataBlockList tcs;
            tcs.push_back(texCoords);
            //tcs.push_back(noise);
            GeometrySet* geom = new GeometrySet(vertices, center, tcs);

            return GeometrySetPtr(geom);
        }

    }
}

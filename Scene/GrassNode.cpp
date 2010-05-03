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
        }

        GrassNode::GrassNode(IShaderResourcePtr shader) {
            quadsPrObject = 3;
            grassGeom = CreateGrassObject();
            grassShader = shader;
        }
        
        void GrassNode::Handle(RenderingEventArg arg){
            if (grassShader){
                ITexture2DPtr map;
                grassShader->GetTexture("heightmap", map);
                arg.renderer.LoadTexture(map);
                
                grassShader->Load();
            }
        }

        GeometrySetPtr GrassNode::CreateGrassObject() {
            float radsPrQuad = 2 * PI / quadsPrObject;

            const float WIDTH = 40;
            const float HEIGHT = 40;

            Float3DataBlockPtr vertices = Float3DataBlockPtr(new DataBlock<3, float>(12));
            Float2DataBlockPtr texCoords = Float2DataBlockPtr(new DataBlock<2, float>(12));

            // Create geometry for all the quads
            int index = 0;
            for (int i = 0; i < quadsPrObject; ++i){
                Vector<3, float> direction = Vector<3, float>(cos(i * radsPrQuad),
                                                              0, sin(i * radsPrQuad));
                
                // lower left
                vertices->SetElement(index, direction * WIDTH / 2);
                texCoords->SetElement(index, Vector<2, float>(0, 0));
                ++index;

                // upper left
                vertices->SetElement(index, direction * WIDTH / 2 + Vector<3, float>(0, HEIGHT, 0));
                texCoords->SetElement(index, Vector<2, float>(0, 1));
                ++index;

                // upper right
                vertices->SetElement(index, direction * WIDTH / -2 + Vector<3, float>(0, HEIGHT, 0));
                texCoords->SetElement(index, Vector<2, float>(1, 1));
                ++index;

                // lower right
                vertices->SetElement(index, direction * WIDTH / -2);
                texCoords->SetElement(index, Vector<2, float>(1, 0));
                ++index;
            }

            IDataBlockList tcs;
            tcs.push_back(texCoords);
            GeometrySet* geom = new GeometrySet(vertices, IDataBlockPtr(), tcs);

            return GeometrySetPtr(geom);
        }

    }
}

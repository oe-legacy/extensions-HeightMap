// Sky sphere node.
// -------------------------------------------------------------------
// Copyright (C) 2010 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Scene/SkySphereNode.h>

#include <Geometry/Mesh.h>
#include <Resources/IShaderResource.h>
#include <Utils/MeshCreator.h>

using namespace OpenEngine::Geometry;
using namespace OpenEngine::Resources;
using namespace OpenEngine::Renderers;
using namespace OpenEngine::Utils::MeshCreator;

namespace OpenEngine {
    namespace Scene {

        SkySphereNode::SkySphereNode(Resources::IShaderResourcePtr atmos, float radius, unsigned int detail) 
            : skySphere(CreateSphere(radius, detail, Vector<3, float>(0,0,0), true)), atmosphere(atmos) {
            
            skySphere->GetMaterial()->shad = atmosphere;

        }

        void SkySphereNode::Handle(RenderingEventArg arg){
            if (atmosphere != NULL){
                atmosphere->Load();
            }
        }

    }
}

// Sky sphere node.
// -------------------------------------------------------------------
// Copyright (C) 2010 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _SKY_SPHERE_NODE_H_
#define _SKY_SPHERE_NODE_H_

#include <Scene/ISceneNode.h>
#include <Core/IListener.h>
#include <Renderers/IRenderer.h>

#include <boost/shared_ptr.hpp>

namespace OpenEngine {
    namespace Geometry {
        class Mesh;
        typedef boost::shared_ptr<Mesh> MeshPtr;
    }
    namespace Resources {
        class IShaderResource;
        typedef boost::shared_ptr<IShaderResource> IShaderResourcePtr;
    }
    namespace Scene {
        
        class SkySphereNode : public ISceneNode, 
                              public Core::IListener<Renderers::RenderingEventArg> {
            
            OE_SCENE_NODE(SkySphereNode, ISceneNode)
        protected:
            Geometry::MeshPtr skySphere;
            Resources::IShaderResourcePtr atmosphere;

        public:
            SkySphereNode() {}
            SkySphereNode(Resources::IShaderResourcePtr atmos, float radius, unsigned int detail);
            
            void Handle(Renderers::RenderingEventArg arg);

            inline Geometry::MeshPtr GetMesh() const { return skySphere; }
            inline Resources::IShaderResourcePtr GetAtmostphereShader() const { return atmosphere; }
            
        };

    }
}

#endif

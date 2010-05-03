// Grass node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _GRASS_NODE_H_
#define _GRASS_NODE_H_

#include <Scene/ISceneNode.h>

#include <Core/IListener.h>
#include <Math/Vector.h>
#include <Renderers/IRenderer.h>

using namespace OpenEngine::Core;
using namespace OpenEngine::Math;
using namespace OpenEngine::Renderers;

namespace OpenEngine {
    namespace Geometry {
        class Mesh;
        typedef boost::shared_ptr<Mesh> MeshPtr;
        class GeometrySet;
        typedef boost::shared_ptr<GeometrySet> GeometrySetPtr;
    }
    namespace Resources {
        class IShaderResource;
        typedef boost::shared_ptr<IShaderResource> IShaderResourcePtr;
    }
    namespace Scene {

        class GrassNode : public ISceneNode,
                          public IListener<RenderingEventArg> {
            OE_SCENE_NODE(GrassNode, ISceneNode);
        private:
            int quadsPrObject;

            Geometry::GeometrySetPtr grassGeom;
            Resources::IShaderResourcePtr grassShader;

        public:
            GrassNode();
            GrassNode(const Resources::IShaderResourcePtr shader);

            void Handle(RenderingEventArg arg);

            inline Resources::IShaderResourcePtr GetGrassShader() const { return grassShader; }
            inline Geometry::GeometrySetPtr GetGrassGeometry() const { return grassGeom; }
            
        private:
            /**
             * Creates the grass star object.
             */
            inline Geometry::GeometrySetPtr CreateGrassObject();

        };

    }
}

#endif

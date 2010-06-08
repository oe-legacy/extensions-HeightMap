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

#include <Core/EngineEvents.h>
#include <Core/IListener.h>
#include <Math/Vector.h>
#include <Renderers/IRenderer.h>

using namespace OpenEngine;
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
        class HeightMapNode;


        class GrassNode : public ISceneNode,
            public IListener<RenderingEventArg>, 
            public IListener<Core::ProcessEventArg> {
            OE_SCENE_NODE(GrassNode, ISceneNode);
        private:
            Geometry::GeometrySetPtr grassGeom;
            Resources::IShaderResourcePtr grassShader;

            // Heightmap to place the grass one
            HeightMapNode* heightmap;

            // Grass grid dimensions
            int gridDim;
            int straws;
            int quadsPrObject;

            unsigned int elapsedTime;

        public:
            GrassNode();
            GrassNode(HeightMapNode* heightmap, const Resources::IShaderResourcePtr shader, 
                      int straws = 4000, int gridDimension = 64, int quadsPrObject = 3);

            void Handle(RenderingEventArg arg);
            void Handle(Core::ProcessEventArg arg);

            inline int GetGridDimension() const { return gridDim; }
            inline Resources::IShaderResourcePtr GetGrassShader() const { return grassShader; }
            inline Geometry::GeometrySetPtr GetGrassGeometry() const { return grassGeom; }
            inline unsigned int GetElapsedTime() const { return elapsedTime; }
            
        private:
            /**
             * Creates the grass star object.
             */
            inline Geometry::GeometrySetPtr CreateGrassObject();

        };

    }
}

#endif

// Water node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _WATER_NODE_H_
#define _WATER_NODE_H_

#include <Scene/ISceneNode.h>
#include <Core/IListener.h>
#include <Renderers/IRenderer.h>
#include <Scene/LandscapeNode.h>
#include <Resources/ITextureResource.h>

using namespace OpenEngine::Resources;
using namespace OpenEngine::Core;
using namespace OpenEngine::Renderers;

namespace OpenEngine {
    namespace Scene {

        class WaterNode : public ISceneNode, public IListener<RenderingEventArg> {
            OE_SCENE_NODE(WaterNode, ISceneNode)

        private:
            static const int SLICES = 24;
            static const int DIMENSIONS = 3;

            int entries;
            float* waterVertices;
            float* waterColors;
            float* bottomVertices;
            float* bottomColors;
            float* texCoords;

            int texDetail;
            ITextureResourcePtr surface;

            Vector<3, float> center;
            float diameter;
            ISceneNode* reflection;

        public:
            WaterNode() {}
            WaterNode(Vector<3, float> c, float d);
            ~WaterNode();

            void VisitSubNode(ISceneNodeVisitor& visitor) {}

            void Handle(RenderingEventArg arg);
            
            float* GetWaterVerticeArray() { return waterVertices; }
            float* GetWaterColorArray() { return waterColors; }
            float* GetBottomVerticeArray() { return bottomVertices; }
            float* GetBottomColorArray() { return bottomColors; }
            float* GetTextureCoordArray() const { return texCoords; }
            
            void SetReflectionsScene(ISceneNode* r) { reflection = r; }
            void SetSurfaceTexture(ITextureResourcePtr tex, int pixelsPrEdge);
            ITextureResourcePtr GetSurfaceTexture() { return surface; }
            Vector<3, float> GetCenter() const { return center; }
            float GetDiameter() const { return diameter; }
            ISceneNode* GetReflectionScene() const { return reflection; }

        private:
            inline void SetupArrays();
            inline void SetupTexCoords();
        };

    }
}

#endif

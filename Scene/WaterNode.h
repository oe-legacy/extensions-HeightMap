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
#include <Resources/IShaderResource.h>
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

            int FBOheight;
            int FBOwidth;
            GLuint reflectionFboID;
            GLuint refractionFboID;
            ITextureResourcePtr depthbufferTex;
            ITextureResourcePtr reflectionTex;
            ITextureResourcePtr refractionTex;

            IShaderResourcePtr waterShader;

        public:
            WaterNode() {}
            WaterNode(Vector<3, float> c, float d);
            ~WaterNode();

            void VisitSubNode(ISceneNodeVisitor& visitor) {}

            void Handle(RenderingEventArg arg);

            Vector<3, float> GetCenter() const { return center; }
            float GetDiameter() const { return diameter; }
            
            float* GetWaterVerticeArray() { return waterVertices; }
            float* GetWaterColorArray() { return waterColors; }
            float* GetBottomVerticeArray() { return bottomVertices; }
            float* GetBottomColorArray() { return bottomColors; }
            float* GetTextureCoordArray() const { return texCoords; }

            int GetFBOHeight() const { return FBOheight; }
            int GetFBOWidth() const { return FBOwidth; }
            GLuint GetReflectionFboID() const { return reflectionFboID; }
            ITextureResourcePtr GetReflectionTex() const { return reflectionTex; }
            GLuint GetRefractionFboID() const { return refractionFboID; }
            GLuint GetRefractionTexID() const { return refractionTexID; }

            void SetReflectionScene(ISceneNode* r) { reflection = r; }
            ISceneNode* GetReflectionScene() const { return reflection; }
            void SetSurfaceTexture(ITextureResourcePtr tex, int pixelsPrEdge);
            ITextureResourcePtr GetSurfaceTexture() { return surface; }
            void SetWaterShader(IShaderResourcePtr water) { waterShader = water; }
            IShaderResourcePtr GetWaterShader() { return waterShader; }

        private:
            inline void SetupArrays();
            inline void SetupTexCoords();
            inline void SetupReflectionFBO();
            inline void SetupRefractionFBO();
        };

    }
}

#endif

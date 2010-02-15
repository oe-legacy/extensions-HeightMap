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
#include <Resources/IShaderResource.h>
#include <Resources/Texture2D.h>
#include <Meta/OpenGL.h>

using namespace OpenEngine::Resources;
using namespace OpenEngine::Core;
using namespace OpenEngine::Renderers;

namespace OpenEngine {
    namespace Scene {

        class SunNode;

        class WaterNode : public ISceneNode, public IListener<RenderingEventArg>, public IListener<ProcessEventArg> {
            OE_SCENE_NODE(WaterNode, ISceneNode)

        private:
            static const int SLICES = 24;
            static const int DIMENSIONS = 3;

            int entries;
            float* waterVertices;
            float* waterNormals;
            float* waterColor;
            float* bottomVertices;
            float* floorColor;
            float* texCoords;

            float texDetail;
            ITextureResourcePtr surface;

            Vector<3, float> center;
            float diameter;
            float planetDiameter;

            ISceneNode* reflection;

            int FBOheight;
            int FBOwidth;
            GLuint reflectionFboID;
            GLuint refractionFboID;
            UCharTexture2DPtr depthbufferTex;
            UCharTexture2DPtr reflectionTex;
            UCharTexture2DPtr refractionTex;

            IShaderResourcePtr waterShader;
            unsigned int elapsedTime;
            SunNode* sun;

        public:
            WaterNode() {}
            WaterNode(Vector<3, float> c, float d);
            ~WaterNode();

            void VisitSubNode(ISceneNodeVisitor& visitor) {}

            void Handle(RenderingEventArg arg);
            void Handle(ProcessEventArg arg);

            Vector<3, float> GetCenter() const { return center; }
            float GetDiameter() const { return diameter; }
            
            SunNode* GetSun() const { return sun; }
            void SetSun(SunNode* s) { sun = s; }

            int GetNumberOfVertices() { return SLICES+2; }
            float* GetWaterVerticeArray() { return waterVertices; }
            float* GetWaterNormalArray() { return waterNormals; }
            float* GetWaterColor() { return waterColor; }
            float* GetBottomVerticeArray() { return bottomVertices; }
            float* GetFloorColor() { return floorColor; }
            float* GetTextureCoordArray() const { return texCoords; }

            int GetFBOHeight() const { return FBOheight; }
            int GetFBOWidth() const { return FBOwidth; }
            GLuint GetReflectionFboID() const { return reflectionFboID; }
            UCharTexture2DPtr GetReflectionTex() const { return reflectionTex; }
            GLuint GetRefractionFboID() const { return refractionFboID; }
            UCharTexture2DPtr GetRefractionTex() const { return refractionTex; }
            UCharTexture2DPtr GetRefractionDepthMap() const { return depthbufferTex; }
            unsigned int GetElapsedTime() { return elapsedTime; }

            void SetReflectionScene(ISceneNode* r) { reflection = r; }
            ISceneNode* GetReflectionScene() const { return reflection; }
            void SetSurfaceTexture(ITextureResourcePtr tex, float pixelsPrEdge);
            ITextureResourcePtr GetSurfaceTexture() { return surface; }
            void SetWaterShader(IShaderResourcePtr water, float pixelsPrEdge);
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

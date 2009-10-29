// Water node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include "WaterNode.h"
#include <Renderers/OpenGL/TextureLoader.h>
#include <Resources/OpenGLTextureResource.h>
#include <Logging/Logger.h>
#include <string.h>

using namespace std;
using namespace OpenEngine::Renderers::OpenGL;

namespace OpenEngine {
    namespace Scene {

        WaterNode::WaterNode(Vector<3, float> c, float d)
            : center(c), diameter(d), planetDiameter(1000), reflection(NULL), 
              waterShader(IShaderResourcePtr()), elapsedTime(0) {
            SetupArrays();
        }

        WaterNode::~WaterNode(){
            
        }

        void WaterNode::Handle(RenderingEventArg arg){
            if (waterShader != NULL){
                waterShader->Load();
                TextureList texs = waterShader->GetTextures();
                for (unsigned int i = 0; i < texs.size(); ++i)
                    TextureLoader::LoadTextureResource(texs[i]);
                
                // Check if framebuffering is supported
                const std::string fboExt = "GL_EXT_framebuffer_object";
                if (glewGetExtension(fboExt.c_str()) != GL_TRUE )
                    throw Exception(fboExt + " not supported");
                
                if (reflection){
                    FBOwidth = 400;
                    FBOheight = 300;
                    
                    SetupReflectionFBO();                
                    //SetupRefractionFBO();

                    waterShader->SetTexture("reflection", reflectionTex);
                }
            }else if (surface != NULL)
                TextureLoader::LoadTextureResource(surface);
        }

        void WaterNode::Handle(ProcessEventArg arg){
            elapsedTime += arg.approx;
        }

        void WaterNode::SetSurfaceTexture(ITextureResourcePtr tex, float pixelsPrUnit){
            surface = tex; 
            texDetail = pixelsPrUnit;
            SetupTexCoords();
        }

        void WaterNode::SetWaterShader(IShaderResourcePtr water, float pixelsPrUnit){
            waterShader = water;
            texDetail = pixelsPrUnit;
            SetupTexCoords();
        }
        
        void WaterNode::SetupArrays(){
            entries = SLICES + 2;
            
            float radsPrSlice = 2 * 3.14 / SLICES;

            int e = 0;
            waterVertices = new float[entries * DIMENSIONS];
            // Set center
            waterVertices[e++] = center[0];
            waterVertices[e++] = center[1];
            waterVertices[e++] = center[2];
            for (int i = 0; i < SLICES; ++i){
                waterVertices[e++] = center[0] + diameter * cos(i * radsPrSlice);
                waterVertices[e++] = center[1];
                waterVertices[e++] = center[2] + diameter * sin(i * radsPrSlice);
            }
            // Set end vertex
            waterVertices[e++] = center[0] + diameter;
            waterVertices[e++] = center[1];
            waterVertices[e++] = center[2];

            waterNormals = new float[entries * DIMENSIONS];
            
            Vector<3, float> planetCenter = Vector<3, float>(center[0], center[1] - planetDiameter, center[2]);

            e = 0;
            for (int i = 0; i < SLICES+2; ++i){
                // Find the vector from the center of the planet to the vertex
                Vector<3, float> normal = Vector<3, float>(waterVertices + e) -planetCenter;
                normal.Normalize();
                waterNormals[e++] = normal[0];
                waterNormals[e++] = normal[1];
                waterNormals[e++] = normal[2];
            }

            waterColor = new float[4];
            waterColor[0] = 0;
            waterColor[1] = 0.1;
            waterColor[2] = 0.7;
            waterColor[3] = .7;

            bottomVertices = new float[entries * DIMENSIONS];
            memcpy(bottomVertices, waterVertices, entries * DIMENSIONS * sizeof(float));
            bottomVertices[1] = center[1] - 100;

            floorColor = new float[4];
            floorColor[0] = 0;
            floorColor[1] = 0.5;
            floorColor[2] = 0;
            floorColor[3] = 1;

            texCoords = new float[entries * 2];
        }

        void WaterNode::SetupTexCoords(){
            for (int i = 0; i < entries; ++i){
                texCoords[i * 2] = waterVertices[i * DIMENSIONS] / texDetail;
                texCoords[i * 2 + 1] = waterVertices[i * DIMENSIONS + 2] / texDetail;
            }
        }

        void WaterNode::SetupReflectionFBO(){
            // setup frame buffer object for reflection
            glGenFramebuffersEXT(1, &reflectionFboID);
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, reflectionFboID);
            
            // attach the depth buffer to the frame buffer
            GLuint depth;
            glGenRenderbuffersEXT(1, &depth);
            glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth);
            glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT32, FBOwidth, FBOheight);
            
            glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                         GL_RENDERBUFFER_EXT, depth);
                        
            // Setup texture to render reflection to
            GLuint reflectionTexID;
            glGenTextures(1, &reflectionTexID);
            glBindTexture(GL_TEXTURE_2D, reflectionTexID);
            
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FBOwidth, FBOheight, 0, GL_RGBA, GL_UNSIGNED_INT, NULL);

            reflectionTex = ITextureResourcePtr(new OpenGLTextureResource(reflectionTexID, FBOwidth, FBOheight, 32));
            
            // attach the texture to FBO color attachment point
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, 
                                      GL_COLOR_ATTACHMENT0_EXT,
                                      GL_TEXTURE_2D, reflectionTexID, 0);
            
            glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        void WaterNode::SetupRefractionFBO(){
            // setup frame buffer object for refraction
            glGenFramebuffersEXT(1, &refractionFboID);
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, refractionFboID);                        
                        
            // Setup texture to render refraction to
            GLuint refractionTexID;
            glGenTextures(1, &refractionTexID);
            glBindTexture(GL_TEXTURE_2D, refractionTexID);
            
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FBOwidth, FBOheight, 0, GL_RGBA, GL_UNSIGNED_INT, NULL);

            // attach the texture to FBO color attachment point
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, 
                                      GL_COLOR_ATTACHMENT0_EXT,
                                      GL_TEXTURE_2D, refractionTexID, 0);

            // attach the depth buffer to the frame buffer
            GLuint depthBuffer;
            glGenTextures(1, &depthBuffer);
            glBindTexture(GL_TEXTURE_2D, depthBuffer);

            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, FBOwidth, FBOheight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, 
                                      GL_DEPTH_ATTACHMENT_EXT,
                                      GL_TEXTURE_2D, depthBuffer, 0);

            refractionTex = ITextureResourcePtr(new OpenGLTextureResource(refractionTexID, FBOwidth, FBOheight, 32));
            depthbufferTex = ITextureResourcePtr(new OpenGLTextureResource(depthBuffer, FBOwidth, FBOheight, 8));

            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        
    }
}

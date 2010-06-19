// Water node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Scene/WaterNode.h>

#include <Resources/IShaderResource.h>
#include <Resources/FrameBuffer.h>
#include <Utils/TerrainTexUtils.h>
#include <Logging/Logger.h>
#include <string.h>

using namespace std;

namespace OpenEngine {
    namespace Scene {

        WaterNode::WaterNode(Vector<3, float> c, float d)
            : center(c), diameter(d), planetDiameter(1000),
              waterShader(IShaderResourcePtr()), elapsedTime(0) {
            SetupArrays();
        }

        WaterNode::~WaterNode(){
            
        }

        void WaterNode::Handle(RenderingEventArg arg){
            if (waterShader != NULL){
                Vector<2, int> dim(200,150);
                reflectionFbo = new FrameBuffer(dim, 1, false);
                arg.renderer.BindFrameBuffer(reflectionFbo);
                
                waterShader->SetTexture("reflection", reflectionFbo->GetTexAttachment(0));

                if (normaldudvmap != NULL)
                    waterShader->SetTexture("normaldudvmap", (ITexture2DPtr)normaldudvmap);

                waterShader->Load();
                TextureList texs = waterShader->GetTextures();
                for (unsigned int i = 0; i < texs.size(); ++i)
                    arg.renderer.LoadTexture(texs[i].get());
                
            }else if (surface != NULL)
                arg.renderer.LoadTexture(surface);
        }

        void WaterNode::Handle(Core::ProcessEventArg arg){
            elapsedTime += arg.approx;
        }

        void WaterNode::SetSurfaceTexture(ITextureResourcePtr tex, float pixelsPrUnit){
            surface = tex; 
            texDetail = pixelsPrUnit;
            SetupTexCoords();
        }

        void WaterNode::SetNormalDudvMap(UCharTexture2DPtr normal, UCharTexture2DPtr dudv){
            if (normal != NULL && dudv != NULL){
                normal->Load();
                dudv->Load();
                
                // Combine the textures
                unsigned int width = normal->GetWidth();
                unsigned int height = normal->GetHeight();
                
                normaldudvmap = UCharTexture2DPtr(new UCharTexture2D(width, height, 4));
                normaldudvmap->SetColorFormat(RGBA);
                normaldudvmap->SetCompression(false);
                normaldudvmap->Load();
                
                for (unsigned int x = 0; x < width; ++x)
                    for (unsigned int z = 0; z < height; ++z){
                        if (normal->GetColorFormat() == BGR){
                            normaldudvmap->GetPixel(x,z)[0] = normal->GetPixel(x,z)[2];
                            normaldudvmap->GetPixel(x,z)[1] = normal->GetPixel(x,z)[1];
                            normaldudvmap->GetPixel(x,z)[2] = dudv->GetPixel(x,z)[2];
                            normaldudvmap->GetPixel(x,z)[3] = dudv->GetPixel(x,z)[1];
                        }else{
                            normaldudvmap->GetPixel(x,z)[0] = normal->GetPixel(x,z)[0];
                            normaldudvmap->GetPixel(x,z)[1] = normal->GetPixel(x,z)[1];
                            normaldudvmap->GetPixel(x,z)[2] = dudv->GetPixel(x,z)[0];
                            normaldudvmap->GetPixel(x,z)[3] = dudv->GetPixel(x,z)[1];
                        }
                    }
            }
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
            floorColor[0] = 0.18f;
            floorColor[1] = 0.24;
            floorColor[2] = 0.45;
            floorColor[3] = 1;

            texCoords = new float[entries * 2];
        }

        void WaterNode::SetupTexCoords(){
            for (int i = 0; i < entries; ++i){
                texCoords[i * 2] = waterVertices[i * DIMENSIONS] / texDetail;
                texCoords[i * 2 + 1] = waterVertices[i * DIMENSIONS + 2] / texDetail;
            }
        }
        
    }
}

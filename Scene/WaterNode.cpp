// Water node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include "WaterNode.h"
#include <Renderers/OpenGL/TerrainTextureLoader.h>
#include <Logging/Logger.h>
#include <string.h>

using namespace std;
using namespace OpenEngine::Renderers::OpenGL;

namespace OpenEngine {
    namespace Scene {

        WaterNode::WaterNode(Vector<3, float> c, float d)
            : center(c), diameter(d), reflection(NULL){
            waterShader = IShaderResourcePtr();
            SetupArrays();
        }

        WaterNode::~WaterNode(){
            
        }

        void WaterNode::Handle(RenderingEventArg arg){
            if (waterShader != NULL){
                waterShader->Load();
                for (ShaderTextureMap::iterator itr = waterShader->textures.begin(); 
                     itr != waterShader->textures.end(); itr++)
                    TerrainTextureLoader::LoadTextureWithMipmapping( (*itr).second );
            }else if (surface != NULL)
                TerrainTextureLoader::LoadTextureWithMipmapping(surface);
        }

        void WaterNode::SetSurfaceTexture(ITextureResourcePtr tex, int pixelsPrEdge){
            surface = tex; 
            texDetail = pixelsPrEdge;
            SetupTexCoords();
        }

        void WaterNode::SetupArrays(){
            entries = SLICES + 2;
            waterVertices = new float[entries * DIMENSIONS];

            waterVertices[0] = center[0];
            waterVertices[1] = center[1];
            waterVertices[2] = center[2];

            float radsPrSlice = 2 * 3.14 / SLICES;

            int e = 3;
            for (int i = 0; i < SLICES; ++i){
                waterVertices[e++] = center[0] + diameter * cos(i * radsPrSlice);
                waterVertices[e++] = center[1];
                waterVertices[e++] = center[2] + diameter * sin(i * radsPrSlice);
            }

            waterVertices[e++] = center[0] + diameter;
            waterVertices[e++] = center[1];
            waterVertices[e++] = center[2];

            waterColors = new float[entries * 4];
            e = 0;
            for (int i = 0; i < entries; ++i){
                waterColors[e++] = 0;
                waterColors[e++] = 0.1;
                waterColors[e++] = 0.7;
                waterColors[e++] = .7;
            }

            bottomVertices = new float[entries * DIMENSIONS];
            memcpy(bottomVertices, waterVertices, entries * DIMENSIONS * sizeof(float));
            bottomVertices[1] = center[1] - 20;

            bottomColors = new float[entries * 4];
            e = 0;
            for (int i = 0; i < entries; ++i){
                bottomColors[e++] = 0;
                bottomColors[e++] = 0.5;
                bottomColors[e++] = 0;
                bottomColors[e++] = 1;
            }

            texCoords = new float[entries * 2];
        }

        void WaterNode::SetupTexCoords(){
            if (surface != NULL){
                surface->Load();
                for (int i = 0; i < entries; ++i){
                    texCoords[i * 2] = waterVertices[i * DIMENSIONS] * texDetail / surface->GetWidth();
                    texCoords[i * 2 + 1] = waterVertices[i * DIMENSIONS + 2] * texDetail / surface->GetHeight();
                }
                surface->Unload();
            }
        }
        
    }
}

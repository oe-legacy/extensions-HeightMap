// HeightMap via OpenGL rendering view.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include "TerrainRenderingView.h"
#include <Scene/LandscapeNode.h>
#include <Scene/SunNode.h>
#include <Math/Vector.h>
#include <Logging/Logger.h>

namespace OpenEngine {
    namespace Renderers {
        namespace OpenGL {
            
            using namespace OpenEngine::Scene;
            
            TerrainRenderingView::TerrainRenderingView(Viewport& viewport) : 
                IRenderingView(viewport), 
                RenderingView(viewport) {
            }
            
            void TerrainRenderingView::VisitLandscapeNode(LandscapeNode* node) {
                if (node->IsInitialized() == false)
                    node->Initialize();

                // First calculate LOD
                node->CalcLOD(viewport.GetViewingVolume());

                // Then do opengl stuff

                glEnable(GL_CULL_FACE);
                
                IShaderResourcePtr shader = node->GetLandscapeShader();
                float* sunPos = node->GetSunPos();
                glLightfv(GL_LIGHT0, GL_POSITION, sunPos);
                // @TODO send a light normal along to the shader, instead of calculating it for each vertex?
                shader->ApplyShader();

                glEnableClientState(GL_VERTEX_ARRAY);
                glVertexPointer(3, GL_FLOAT, 0, node->GetVerticeArray());

                glEnableClientState(GL_NORMAL_ARRAY);
                glNormalPointer(GL_FLOAT, 0, node->GetNormalArray());

                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                glTexCoordPointer(2, GL_FLOAT, 0, node->GetTextureCoordArray());

                node->RenderPatches();

                glDisableClientState(GL_VERTEX_ARRAY);
                glDisableClientState(GL_NORMAL_ARRAY);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);

                shader->ReleaseShader();
                
                glDisable(GL_CULL_FACE);
            }

            void TerrainRenderingView::VisitSunNode(SunNode* node) {
                float* coords = node->GetPos();

                GLboolean t = glIsEnabled(GL_TEXTURE_2D);
                GLboolean l = glIsEnabled(GL_LIGHTING);
                glDisable(GL_TEXTURE_2D);
                glDisable(GL_LIGHTING);               

                glPushMatrix();
                glTranslatef(coords[0], coords[1], coords[2]);
                glColor3f(1, 0.8, 0);
                GLUquadricObj* qobj = gluNewQuadric();
                glLineWidth(1);
                gluQuadricNormals(qobj, GLU_SMOOTH);
                gluQuadricDrawStyle(qobj, GLU_FILL);
                gluQuadricOrientation(qobj, GLU_INSIDE);
                gluSphere(qobj, 15, 10, 10); 
                gluDeleteQuadric(qobj);
                glPopMatrix();
                
                // reset state
                if (t) glEnable(GL_TEXTURE_2D);
                if (l) glEnable(GL_LIGHTING);
            }
            
        }
    }
}

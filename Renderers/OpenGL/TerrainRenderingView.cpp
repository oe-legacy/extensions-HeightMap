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
#include <Scene/WaterNode.h>
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
                //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

                node->CalcLOD(viewport.GetViewingVolume());

                // Then do opengl stuff
                glEnable(GL_CULL_FACE);

                glEnableClientState(GL_VERTEX_ARRAY);
                glVertexPointer(3, GL_FLOAT, 0, node->GetVerticeArray());
                
                glEnableClientState(GL_NORMAL_ARRAY);
                glNormalPointer(GL_FLOAT, 0, node->GetNormalArray());
                
                IShaderResourcePtr shader = node->GetLandscapeShader();
                if (shader){
                    glLightfv(GL_LIGHT0, GL_POSITION, node->GetSunPos());
                    shader->ApplyShader();
                    
                    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                    glTexCoordPointer(2, GL_FLOAT, 0, node->GetTextureCoordArray());
                }else{
                    glEnableClientState(GL_COLOR_ARRAY);
                    glColorPointer(3, GL_UNSIGNED_BYTE, 0, node->GetColorArray());
                }

                //node->RenderPatches();
                node->VisitSubNodes(*this);

                glDisableClientState(GL_VERTEX_ARRAY);
                glDisableClientState(GL_NORMAL_ARRAY);
                if (shader){
                    shader->ReleaseShader();
                    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                }else{
                    glDisableClientState(GL_COLOR_ARRAY);
                }
                glDisable(GL_CULL_FACE);
            }

            void TerrainRenderingView::VisitLandscapePatchNode(LandscapePatchNode* node) {
                node->Render();
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

            void TerrainRenderingView::VisitWaterNode(WaterNode* node) {
                ISceneNode* reflection = node->GetReflectionScene();
                if (reflection){
                    // Render reflection
                    glCullFace(GL_FRONT);
                    double plane[4] = {0.0, -1.0, 0.0, 0.0}; //water at y=0
                    glEnable(GL_CLIP_PLANE0);
                    glClipPlane(GL_CLIP_PLANE0, plane);
                    
                    glScalef(1, -1, 1);
                    node->GetReflectionScene()->Accept(*this);
                    
                    glDisable(GL_CLIP_PLANE0);
                    
                    glCullFace(GL_BACK);
                }

                glEnableClientState(GL_VERTEX_ARRAY);
                glEnableClientState(GL_COLOR_ARRAY);
                glVertexPointer(3, GL_FLOAT, 0, node->GetBottomVerticeArray());
                glColorPointer(4, GL_FLOAT, 0, node->GetBottomColorArray());

                glDrawArrays(GL_TRIANGLE_FAN, 0, 26);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glVertexPointer(3, GL_FLOAT, 0, node->GetWaterVerticeArray());
                glColorPointer(4, GL_FLOAT, 0, node->GetWaterColorArray());

                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                glTexCoordPointer(2, GL_FLOAT, 0, node->GetTextureCoordArray());
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, node->GetSurfaceTexture()->GetID() );

                glDrawArrays(GL_TRIANGLE_FAN, 0, 26);

                glDisable(GL_BLEND);
                glDisable(GL_TEXTURE_2D);
                glDisableClientState(GL_VERTEX_ARRAY);
                glDisableClientState(GL_COLOR_ARRAY);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            }
        }
    }
}

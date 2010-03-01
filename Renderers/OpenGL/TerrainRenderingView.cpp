// HeightMap via OpenGL rendering view.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include "TerrainRenderingView.h"

#include <Resources/IShaderResource.h>
#include <Scene/HeightMapNode.h>
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

            void TerrainRenderingView::VisitHeightMapNode(HeightMapNode* node) {
                glEnable(GL_CULL_FACE);

                this->ApplyMesh(node->GetMesh().get());

                IShaderResourcePtr shader = node->GetLandscapeShader();
                if (shader){
                    // Setup uniforms
                    SunNode* sun = node->GetSun();
                    if (sun)
                        shader->SetUniform("lightDir", sun->GetPos().GetNormalize());
                    shader->SetUniform("viewPos", viewport.GetViewingVolume()->GetPosition());

                    shader->ApplyShader();
                }

                node->CalcLOD(viewport.GetViewingVolume());

                node->Render(viewport);

                ApplyMesh(NULL);

                if (shader){
                    shader->ReleaseShader();
                }

                glDisable(GL_CULL_FACE);

                if (renderTangent)
                    node->RenderBoundingGeometry();

                node->VisitSubNodes(*this);

            }

            void TerrainRenderingView::VisitSunNode(SunNode* node) {
                float pos[4];
                pos[3] = 1.0;
                node->GetPos().ToArray(pos);
                glLightfv(GL_LIGHT0, GL_POSITION, pos);
                float color[4];
                node->GetAmbient().ToArray(color);
                glLightfv(GL_LIGHT0, GL_AMBIENT, color);
                node->GetDiffuse().ToArray(color);
                glLightfv(GL_LIGHT0, GL_DIFFUSE, color);
                node->GetSpecular().ToArray(color);
                glLightfv(GL_LIGHT0, GL_SPECULAR, color);
                glEnable(GL_LIGHT0);

                if (node->renderGeometry()){
                    Vector<3, float> coords = node->GetPos();
                    
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
                    gluSphere(qobj, 35, 10, 10); 
                    gluDeleteQuadric(qobj);
                    glPopMatrix();
                    
                    // reset state
                    if (t) glEnable(GL_TEXTURE_2D);
                    if (l) glEnable(GL_LIGHTING);
                }

                node->VisitSubNodes(*this);
            }

            void TerrainRenderingView::VisitWaterNode(WaterNode* node) {
                IShaderResourcePtr shader = node->GetWaterShader();
                ISceneNode* reflection = node->GetReflectionScene();
                if (shader != NULL){
                    if (reflection){
                        
                        // setup water clipping plane
                        double plane[4] = {0.0, -1.0, 0.0, 0.0}; //water at y~~0
                        glEnable(GL_CLIP_PLANE0);
                        glClipPlane(GL_CLIP_PLANE0, plane);

                        glViewport(0, 0, node->GetFBOWidth(), node->GetFBOHeight());

                        // Render reflection

                        // Enable frame buffer
                        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, node->GetReflectionFboID());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                        
                        glCullFace(GL_FRONT);

                        glPushMatrix();

                        glScalef(1, -1, 1);
                        glTranslatef(0, -2, 0);
                        
                        // Render scene
                        reflection->Accept(*this);
                        
                        glPopMatrix();
                        glCullFace(GL_BACK);

                        // Disable frame buffer
                        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
                        glDisable(GL_CLIP_PLANE0);
                        
                        // Reset viewport
                        Vector<4, int> viewDims = this->renderer->GetViewport().GetDimension();
                        glViewport(viewDims[0], viewDims[1], viewDims[2], viewDims[3]);

                        // Render reflection
                        reflection->Accept(*this);
                    }

                    float pos[3];
                    node->GetSun()->GetPos().ToArray(pos);
                    glLightfv(GL_LIGHT0, GL_POSITION, pos);

                    glEnableClientState(GL_VERTEX_ARRAY);
                    glVertexPointer(3, GL_FLOAT, 0, node->GetBottomVerticeArray());
                    
                    glColor4fv(node->GetFloorColor());
                    glDrawArrays(GL_TRIANGLE_FAN, 0, 26);

                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                    // set shader uniforms
                    Vector<3, float> viewPos = viewport.GetViewingVolume()->GetPosition();
                    shader->SetUniform("viewpos", viewPos);
                    float time = (float)node->GetElapsedTime();
                    shader->SetUniform("time", time / 8000000000.0f);
                    shader->SetUniform("time2", time / 4000000.0f);
                    shader->SetUniform("lightDir", node->GetSun()->GetPos().GetNormalize());
                    shader->ApplyShader();

                    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                    glEnableClientState(GL_NORMAL_ARRAY);
                    glVertexPointer(3, GL_FLOAT, 0, node->GetWaterVerticeArray());
                    glNormalPointer(GL_FLOAT, 0, node->GetWaterNormalArray());
                    glTexCoordPointer(2, GL_FLOAT, 0, node->GetTextureCoordArray());

                    glNormal3f(0, 1, 0);
                    glDrawArrays(GL_TRIANGLE_FAN, 0, 26);

                    glDisableClientState(GL_VERTEX_ARRAY);
                    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                    glDisableClientState(GL_NORMAL_ARRAY);

                    shader->ReleaseShader();

                    glDisable(GL_BLEND);
                    
                }else{
                    if (reflection){
                        // Render refraction
                        reflection->Accept(*this);
                    }
                    
                    glEnableClientState(GL_VERTEX_ARRAY);
                    glVertexPointer(3, GL_FLOAT, 0, node->GetBottomVerticeArray());
                    
                    glColor4fv(node->GetFloorColor());
                    glDrawArrays(GL_TRIANGLE_FAN, 0, 26);
                    
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    
                    glVertexPointer(3, GL_FLOAT, 0, node->GetWaterVerticeArray());
                    
                    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                    glTexCoordPointer(2, GL_FLOAT, 0, node->GetTextureCoordArray());
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, node->GetSurfaceTexture()->GetID());

                    glColor4fv(node->GetWaterColor());
                    glDrawArrays(GL_TRIANGLE_FAN, 0, node->GetNumberOfVertices());
                    
                    glDisable(GL_BLEND);
                    glDisable(GL_TEXTURE_2D);
                    glDisableClientState(GL_VERTEX_ARRAY);
                    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                }

                if (renderSoftNormal){
                    float* vert = node->GetWaterVerticeArray();
                    float* norm = node->GetWaterNormalArray();
                    glColor3f(1, 0, 0);
                    glBegin(GL_LINES);
                    for (int i = 0; i < node->GetNumberOfVertices();++i){
                        int j = i * 3;
                        glVertex3f(vert[j], vert[j+1], vert[j+2]);
                        glVertex3f(vert[j] + norm[j], vert[j+1] + norm[j+1], vert[j+2] + norm[j+2]);
                    }
                    glEnd();
                }
            }
        }
    }
}

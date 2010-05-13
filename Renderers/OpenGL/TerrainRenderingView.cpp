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
#include <Scene/GrassNode.h>
#include <Scene/HeightMapNode.h>
#include <Scene/SunNode.h>
#include <Scene/SkySphereNode.h>
#include <Scene/WaterNode.h>
#include <Math/Vector.h>
#include <Geometry/GeometrySet.h>

#include <Logging/Logger.h>

using namespace OpenEngine::Geometry;

namespace OpenEngine {
    namespace Renderers {
        namespace OpenGL {
            
            using namespace OpenEngine::Scene;
            
            TerrainRenderingView::TerrainRenderingView()
                : RenderingView() {

                lightDir = Vector<3, float>(1,1,1).GetNormalize();
            }

            void TerrainRenderingView::VisitGrassNode(GrassNode* node) {
                if (currentRenderState->IsOptionDisabled(RenderStateNode::BACKFACE))
                    glDisable(GL_CULL_FACE);

                GeometrySetPtr geom = node->GetGrassGeometry();
                this->ApplyGeometrySet(geom);
                
                IShaderResourcePtr shader = node->GetGrassShader();
                if (this->renderShader && shader){
                    shader->SetUniform("lightDir", lightDir);
                    shader->SetUniform("time", node->GetElapsedTime() / 1000.0f);

                    // Move the view position by the grid dimension
                    // relative to the eye direction.
                    Vector<3, float> eyeDir = arg->canvas.GetViewingVolume()->GetDirection().RotateVector(Vector<3, float>(0,0,1));
                    Vector<3, float> viewPos = arg->canvas.GetViewingVolume()->GetPosition();
                    Vector<2, float> eyePos(viewPos.Get(0), viewPos.Get(2));
                    int halfDim = node->GetGridDimension() / 2;
                    eyePos[0] -= eyeDir.Get(0) * halfDim;
                    eyePos[1] -= eyeDir.Get(2) * halfDim;
                    shader->SetUniform("viewPos", eyePos);

                    shader->ApplyShader();
                }else
                    glColor3f(0,1,0);
                
                glDrawArrays(GL_QUADS, 0, geom->GetVertices()->GetSize());

                if (shader){
                    shader->ReleaseShader();
                    this->currentShader.reset();
                }

                if (currentRenderState->IsOptionDisabled(RenderStateNode::BACKFACE))
                    glEnable(GL_CULL_FACE);

                node->VisitSubNodes(*this);
            }
            
            void TerrainRenderingView::VisitHeightMapNode(HeightMapNode* node) {
                bool bufferSupport = arg->renderer.BufferSupport();
                
                GeometrySetPtr geom = node->GetGeometrySet();
                this->ApplyGeometrySet(geom);

                IShaderResourcePtr shader = node->GetLandscapeShader();
                if (this->renderShader && shader){
                    // Setup uniforms
                    shader->SetUniform("lightDir", lightDir);
                    shader->SetUniform("viewPos", arg->canvas.GetViewingVolume()->GetPosition());

                    shader->ApplyShader();
                }

                node->CalcLOD(arg->canvas.GetViewingVolume());
                
                IndicesPtr indices = node->GetIndices();
                if (bufferSupport) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices->GetID());

                // Replace with a patch iterator
                node->Render(*arg);

                ApplyGeometrySet(GeometrySetPtr());

                if (shader){
                    shader->ReleaseShader();
                    this->currentShader.reset();
                }
                
                if (bufferSupport) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

                if (renderTangent)
                    node->RenderBoundingGeometry();

                node->VisitSubNodes(*this);
            }

            void TerrainRenderingView::VisitSunNode(SunNode* node) {
                lightDir = node->GetPos().GetNormalize();

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
                        Vector<4, int> viewDims(0,0, arg->canvas.GetWidth(),
                                                arg->canvas.GetHeight());
                        glViewport(viewDims[0], viewDims[1], viewDims[2], viewDims[3]);

                        // Render reflection
                        reflection->Accept(*this);
                    }

                    ApplyGeometrySet(GeometrySetPtr());

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
                    Vector<3, float> viewPos = arg->canvas.GetViewingVolume()->GetPosition();
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
                    this->currentShader.reset();

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
            }
            
            void TerrainRenderingView::VisitSkySphereNode(SkySphereNode* node){
                IShaderResourcePtr atm = node->GetAtmostphereShader();

                Vector<3, float> viewPos = arg->canvas.GetViewingVolume()->GetPosition();
                atm->SetUniform("v3CameraPos", viewPos);
                atm->SetUniform("fCameraHeight", viewPos.Get(1));
                atm->SetUniform("v3LightPos", Vector<3, float>(-1,0.3,0).GetNormalize()); // should be normalized

                this->ApplyMesh(node->GetMesh().get());
            }

        }
    }
}

// Terrain rendering view.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _TERRAIN_RENDERING_VIEW_H_
#define _TERRAIN_RENDERING_VIEW_H_

#include <Renderers/OpenGL/RenderingView.h>
#include <Display/Viewport.h>

#include <Scene/GrassNode.h>
#include <Scene/WaterNode.h>
#include <Scene/HeightMapNode.h>
#include <Scene/SunNode.h>
#include <Scene/SkySphereNode.h>

namespace OpenEngine {
namespace Renderers {
namespace OpenGL {

using namespace OpenEngine::Renderers;
    
 class TerrainRenderingView : public RenderingView {
 protected:
     Vector<3, float> lightDir;
     
 public:
     TerrainRenderingView(Display::Viewport& viewport);
     
     void VisitGrassNode(GrassNode* node);
     void VisitHeightMapNode(HeightMapNode* node);
     void VisitSunNode(SunNode* node);
     void VisitWaterNode(WaterNode* node);
     void VisitSkySphereNode(SkySphereNode* node);

 };

}
}
}

#endif

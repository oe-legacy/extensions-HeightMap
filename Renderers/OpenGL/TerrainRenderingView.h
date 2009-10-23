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

namespace OpenEngine {
namespace Renderers {
namespace OpenGL {

using namespace OpenEngine::Renderers;
    
 class TerrainRenderingView : public RenderingView {

 public:
     TerrainRenderingView(Viewport& viewport);
     
     void VisitLandscapeNode(LandscapeNode* node);
     void VisitLandscapePatchNode(LandscapePatchNode* node);
     void VisitHeightFieldNode(HeightFieldNode* node);
     void VisitSunNode(SunNode* node);
     void VisitWaterNode(WaterNode* node);

 };

}
}
}

#endif

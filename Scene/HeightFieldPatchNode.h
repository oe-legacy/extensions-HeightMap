// Heightfield patch node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _HEIGHTFIELD_PATCH_NODE_H_
#define _HEIGHTFIELD_PATCH_NODE_H_

#include <Scene/ISceneNode.h>

namespace OpenEngine {
    namespace Scene {

        class HeightFieldPatchNode : public ISceneNode {
            OE_SCENE_NODE(HeightFieldPatchNode, ISceneNode)

        public:
            static const int PATCH_EDGE_SQUARES = 32;
            static const int PATCH_EDGE_VERTICES = PATCH_EDGE_SQUARES + 1;
            static const int MAX_LODS = 4;

        private:
            int LOD; // a LOD of 0 means the patch is outside the frustum

        public:            
            HeightFieldPatchNode() {}
            ~HeightFieldPatchNode() {}
        };
        
    }
}

#endif

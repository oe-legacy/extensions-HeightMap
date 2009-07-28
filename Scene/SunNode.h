// Sun node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _SUN_NODE_H_
#define _SUN_NODE_H_

#include <Scene/ISceneNode.h>
#include <Math/Quaternion.h>

using namespace OpenEngine::Math;

namespace OpenEngine {
    namespace Scene {
        
        class SunNode : public ISceneNode {
            OE_SCENE_NODE(sunNode, ISceneNode)
            
        private:
            float* coords;
            float* origo;
            float* direction;

            unsigned int time;
            float timeModifier;

        public:
            SunNode(){coords[0] = 0; coords[1] = 0; coords[2] = 0; }
            SunNode(float* coords);
            SunNode(float* coords, float* origo);

            float* GetPos() { return coords; }

            void SetTime(unsigned int time) { this->time = time; }
            void SetTimeModifier(float timeMod) { timeModifier = timeMod; }

            void Move(unsigned int dt);

            void VisitSubNodes(ISceneNodeVisitor& visitor) {};

        private:
            void Init(float* coords, float* origo);
        };
        
    }
}

#endif _SUN_NODE_H_

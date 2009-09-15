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
#include <Core/IListener.h>
#include <Renderers/IRenderer.h>

using namespace OpenEngine::Core;
using namespace OpenEngine::Renderers;

namespace OpenEngine {
    namespace Scene {
        
        class SunNode : public ISceneNode, public IListener<ProcessEventArg> {
            OE_SCENE_NODE(sunNode, ISceneNode)
            
        private:
            float* coords;
            float* origo;
            float* direction;

            Vector<4, float> diffuse;
            Vector<4, float> baseDiffuse;
            Vector<4, float> specular;
            Vector<4, float> baseSpecular;

            unsigned int time;
            float timeModifier;

        public:
            SunNode(){coords[0] = 0; coords[1] = 0; coords[2] = 0; }
            SunNode(float* coords);
            SunNode(float* coords, float* origo);

            float* GetPos() { return coords; }
            Vector<4, float> GetDiffuse() { return diffuse; }
            Vector<4, float> GetSpecular() { return specular; }

            void SetTime(unsigned int time) { this->time = time; }
            void SetTimeModifier(float timeMod) { timeModifier = timeMod; }
            void SetDiffuse(Vector<4, float> d) { baseDiffuse = d; }
            void SetSpecular(Vector<4, float> s) { baseSpecular = s; }

            void Move(unsigned int dt);

            void VisitSubNodes(ISceneNodeVisitor& visitor) {};

            void Handle(ProcessEventArg arg);

        private:
            void Init(float* coords, float* origo);
        };
        
    }
}

#endif //_SUN_NODE_H_

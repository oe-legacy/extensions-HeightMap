// Sun node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Scene/SunNode.h>
#include <Math/Math.h>
#include <Logging/Logger.h>

namespace OpenEngine {
    namespace Scene {

        SunNode::SunNode(float* dir){
            origo = new float[3];
            origo[0] = 0; origo[1] = 0; origo[2] = 0;
            Init(dir, origo);
        }

        SunNode::SunNode(float* dir, float* origo){
            Init(dir, origo);
        }

        void SunNode::Init(float* dir, float* o){
            direction = dir;
            origo = o;
            coords = new float[3];
            time = 0;
            timeModifier = 5000000;
            Move(time);
        }

        void SunNode::Move(unsigned int dt){
            time += dt;
            
            coords[0] = origo[0] + direction[0] * cos(time/timeModifier);
            coords[1] = origo[1] + direction[1] * sin(time/timeModifier);
            coords[2] = origo[2] + direction[2] * cos(time/timeModifier);
        }

    }
}

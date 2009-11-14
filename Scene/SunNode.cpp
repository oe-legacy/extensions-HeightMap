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
            baseDiffuse = Vector<4, float>(1.0);
            baseSpecular = Vector<4, float>(1.0, 1.0, 0.7, 1);
            ambient = Vector<4, float>(0.2, 0.2, 0.2, 0.0);
            direction = dir;
            origo = o;
            coords = new float[3];
            time = 0;
            timeModifier = 50000000;
            Move(time);
        }

        void SunNode::Move(unsigned int dt){
            time += dt;

            float cosine = cos(time/timeModifier);
            float sinus = sin(time/timeModifier);

            // move the sun
            coords[0] = origo[0] + direction[0] * cosine;
            coords[1] = origo[1] + direction[1] * sinus;
            coords[2] = origo[2] + direction[2] * cosine;

            // set the diffuse strength
            if (sinus <= -0.1)
                diffuse = Vector<4, float>((float)0);
            else if (sinus < 0.15)
                diffuse = baseDiffuse * (sinus + 0.1) * 4;
            else 
                diffuse = baseDiffuse;

            // set the specular strength
            if (sinus <= -0.05)
                specular = Vector<4, float>(0.0);
            else
                specular = baseSpecular;
        }

        void SunNode::Handle(ProcessEventArg arg){
            Move(arg.approx);
        }
        
    }
}

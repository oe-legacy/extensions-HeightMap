// Sun node.
// -------------------------------------------------------------------
// Copyright (C) 2010 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Scene/SunNode.h>
#include <Math/Math.h>

using namespace std;

namespace OpenEngine {
    namespace Scene {

        SunNode::SunNode(Vector<3, float> dir){
            origo = Vector<3, float>(0.0f);
            Init(dir, origo);
        }

        SunNode::SunNode(Vector<3, float> dir, Vector<3, float> origo){
            Init(dir, origo);
        }

        void SunNode::Init(Vector<3, float> dir, Vector<3, float> o){
            geometry = true;
            baseDiffuse = Vector<4, float>(1.0);
            baseSpecular = Vector<4, float>(1.0, 1.0, 0.7, 1);
            ambient = Vector<4, float>(0.2, 0.2, 0.2, 0.0);
            direction = dir;
            origo = o;
            time = 0;
            dayLength = 50 * 1000000;
            Move(time);
        }

        void SunNode::SetTimeOfDay(float t){
            time = (t-6) / 12.0f;
        }
        
        float SunNode::GetTimeOfDay() {
            return fmod(time * 12.0f + 6.0f, 24.0f);
        }

        float SunNode::GetTimeofDayRatio() {
            return fabs((GetTimeOfDay() - 12.0f) / 12.0f);
        }

        void SunNode::Move(unsigned int dt) {
            if (dayLength != 0){
                time += float(dt) / dayLength;
            }
            float cosine = cos(time * Math::PI);
            float sinus = sin(time * Math::PI);
            
            // move the sun
            coords[0] = origo[0] + direction[0] * cosine;
            coords[1] = origo[1] + direction[1] * sinus;
            coords[2] = origo[2] + direction[2] * cosine;
                
            diffuse = baseDiffuse;

            // set the diffuse strength
            if (sinus <= -0.15)
                diffuse[0] = 0.0f;
            else if (sinus < 0.10)
                diffuse[0] = baseDiffuse[0] * (sinus + 0.15) * 4;
            else
                diffuse[0] = baseDiffuse[0];

            if (sinus <= -0.1)
                diffuse[1] = 0.0f;
            else if (sinus < 0.15)
                diffuse[1] = baseDiffuse[1] * (sinus + 0.1) * 4;
            else
                diffuse[1] = baseDiffuse[1];
                
            if (sinus <= 0.0f)
                diffuse[2] = 0.0f;
            else if (sinus < 0.25)
                diffuse[2] = baseDiffuse[2] * sinus * 4;
            else
                diffuse[2] = baseDiffuse[2];

            // set the specular strength
            if (sinus <= -0.05)
                specular = Vector<4, float>(0.0);
            else
                specular = baseSpecular;
        }

        void SunNode::VisitSubNodes(ISceneNodeVisitor& visitor){
            list<ISceneNode*>::iterator itr;
            for (itr = subNodes.begin(); itr != subNodes.end(); ++itr){
                (*itr)->Accept(visitor);
            }
        }

        void SunNode::Handle(Core::ProcessEventArg arg){
            Move(arg.approx);
        }
        
    }
}

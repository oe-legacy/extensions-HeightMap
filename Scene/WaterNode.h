// Water node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _WATER_NODE_H_
#define _WATER_NODE_H_

#include <Scene/ISceneNode.h>
#include <Scene/LandscapeNode.h>

namespace OpenEngine {
    namespace Scene {

        class WaterNode : public ISceneNode {
            OE_SCENE_NODE(WaterNode, ISceneNode)

        private:
            static const int SLICES = 24;
            static const int DIMENSIONS = 3;

            int entries;
            float* waterVertices;
            float* waterColors;
            float* bottomVertices;
            float* bottomColors;
            float* texCoords;

            Vector<3, float> center;
            float diameter;
            ISceneNode* reflection;

        public:
            WaterNode() {}
            WaterNode(Vector<3, float> c, float d);
            ~WaterNode();

            void VisitSubNode(ISceneNodeVisitor& visitor) {}

            float* GetWaterVerticeArray() { return waterVertices; }
            float* GetWaterColorArray() { return waterColors; }
            float* GetBottomVerticeArray() { return bottomVertices; }
            float* GetBottomColorArray() { return bottomColors; }
            
            void SetReflectionsScene(ISceneNode* r) { reflection = r; }
            Vector<3, float> GetCenter() const { return center; }
            float GetDiameter() const { return diameter; }
            ISceneNode* GetReflectionScene() const { return reflection; }

        private:
            inline void SetupArrays();
        };

    }
}

#endif

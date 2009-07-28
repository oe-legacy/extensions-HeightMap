// Terrain Module.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _TERRAIN_MODULE_H_
#define _TERRAIN_MODULE_H_

#include <Core/IModule.h>
#include <Scene/LandscapeNode.h>
#include <Scene/SunNode.h>

using namespace OpenEngine::Core;

namespace OpenEngine {
    namespace Scene {

        class TerrainModule : public IModule {
            
        private:
            LandscapeNode* map;
            SunNode* sun;

        public:
            TerrainModule(SunNode* s, LandscapeNode* hm);

            void Handle(InitializeEventArg arg);
            void Handle(ProcessEventArg arg);
            void Handle(DeinitializeEventArg arg);
        };
    }
}

#endif _TERRAIN_MODULE_H_

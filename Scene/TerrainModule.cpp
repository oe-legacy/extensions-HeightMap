// Sun node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Scene/TerrainModule.h>
#include <Logging/Logger.h>

namespace OpenEngine {
    namespace Scene{

        TerrainModule::TerrainModule(SunNode* s, LandscapeNode* hm) : map(hm), sun(s) {

        }

        void TerrainModule::Handle(InitializeEventArg arg){
            
        }

        void TerrainModule::Handle(ProcessEventArg arg){
            sun->Move(arg.approx);

            /*
            map->CalcSimpleShadow();
            map->BoxBlurShadow(7,7);
            map->ApplyShadow();
            */
        }

        void TerrainModule::Handle(DeinitializeEventArg arg){

        }

    }
}

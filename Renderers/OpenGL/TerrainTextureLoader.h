// Terrain Texture loader.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _TERRAIN_TEXTURE_LOADER_H_
#define _TERRAIN_TEXTURE_LOADER_H_

#include <Renderers/OpenGL/TextureLoader.h>

namespace OpenEngine {
namespace Renderers {
namespace OpenGL {

    class TerrainTextureLoader : public TextureLoader {
    public:
        TerrainTextureLoader();
        ~TerrainTextureLoader();

        static void LoadTextureWithMipmapping(ITextureResourcePtr& tex);
    };

} // NS OpenGL
} // NS Renderers
} // NS OpenEngine

#endif

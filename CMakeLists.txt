
SET ( EXTENSION_NAME "Extensions_HeightMap")

# Create the extension library
ADD_LIBRARY( ${EXTENSION_NAME}
  Renderers/OpenGL/TerrainRenderingView.h
  Renderers/OpenGL/TerrainRenderingView.cpp
  Scene/GrassNode.h
  Scene/GrassNode.cpp
  Scene/HeightMapNode.h
  Scene/HeightMapNode.cpp
  Scene/HeightMapPatch.h
  Scene/HeightMapPatch.cpp
  Scene/SunNode.h
  Scene/SunNode.cpp
  Scene/WaterNode.h
  Scene/WaterNode.cpp
  Scene/SkySphereNode.h
  Scene/SkySphereNode.cpp
  Utils/TerrainUtils.h
  Utils/TerrainUtils.cpp
  Utils/TerrainTexUtils.h
  Utils/TerrainTexUtils.cpp
)

TARGET_LINK_LIBRARIES( ${EXTENSION_NAME}
  OpenEngine_Display
  OpenEngine_Scene
  OpenEngine_Renderers
  OpenEngine_Resources
  Extensions_OpenGLRenderer
  Extensions_MeshUtils
  ${OPENGL_LIBRARY}
  ${GLEW_LIBRARIES}
  ${SDL_LIBRARY}

)

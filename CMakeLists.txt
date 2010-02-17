
SET ( EXTENSION_NAME "Extensions_HeightMap")

# Create the extension library
ADD_LIBRARY( ${EXTENSION_NAME}
  Renderers/OpenGL/TerrainRenderingView.cpp
  Scene/HeightMapNode.cpp
  Scene/HeightMapPatchNode.cpp
  Scene/SunNode.cpp
  Scene/WaterNode.cpp
  Utils/TerrainUtils.cpp
  Utils/TerrainTexUtils.cpp
)

TARGET_LINK_LIBRARIES( ${EXTENSION_NAME}
  OpenEngine_Scene
  OpenEngine_Renderers
  OpenEngine_Resources
  Extensions_OpenGLRenderer
  ${OPENGL_LIBRARY}
  ${GLEW_LIBRARIES}
  ${SDL_LIBRARY}

)

TARGET_LINK_LIBRARIES(Extensions_HeightMap
  OpenEngine_Scene
  OpenEngine_Display
)

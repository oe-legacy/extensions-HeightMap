
SET ( EXTENSION_NAME "Extensions_HeightMap")

# Create the extension library
ADD_LIBRARY( ${EXTENSION_NAME}
  Renderers/OpenGL/TerrainRenderingView.cpp
  Renderers/OpenGL/TerrainTextureLoader.cpp
  Scene/LandscapeNode.cpp
  Scene/LandscapePatchNode.cpp
  Scene/SunNode.cpp
  Scene/TerrainModule.cpp
  Scene/WaterNode.cpp
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
)
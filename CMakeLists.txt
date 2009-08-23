
# Create the extension library
ADD_LIBRARY(Extensions_HeightMap
  Renderers/OpenGL/TerrainRenderingView.cpp
  Renderers/OpenGL/TerrainTextureLoader.cpp
  Scene/LandscapeNode.cpp
  Scene/LandscapePatchNode.cpp
  Scene/SunNode.cpp
  Scene/TerrainModule.cpp
  Scene/WaterNode.cpp
)

TARGET_LINK_LIBRARIES(Extensions_HeightMap
  OpenEngine_Scene
)
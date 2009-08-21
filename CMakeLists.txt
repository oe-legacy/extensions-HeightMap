
# Create the extension library
ADD_LIBRARY(Extensions_HeightMap
  Renderers/OpenGL/TerrainRenderingView.cpp
  Scene/LandscapeNode.cpp
  Scene/LandscapePatchNode.cpp
  Scene/SunNode.cpp
  Scene/TerrainModule.cpp
  Scene/WaterNode.cpp
)

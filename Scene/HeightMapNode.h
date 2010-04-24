// Heightfield node.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _HEIGHTFIELD_NODE_H_
#define _HEIGHTFIELD_NODE_H_

#include <Scene/ISceneNode.h>
#include <Core/IListener.h>
#include <Renderers/IRenderer.h>
#include <Resources/Texture2D.h>
#include <Display/Viewport.h>
#include <Resources/DataBlock.h>

using namespace OpenEngine::Core;
using namespace OpenEngine::Renderers;
using namespace OpenEngine::Resources;
using namespace OpenEngine::Geometry;

namespace OpenEngine {
    namespace Geometry {
        class GeometrySet;
        typedef boost::shared_ptr<GeometrySet> GeometrySetPtr;
    }
    namespace Resources {
        class IShaderResource;
        typedef boost::shared_ptr<IShaderResource> IShaderResourcePtr;
    }
    namespace Display {
        class IViewingVolume;
    }
    namespace Scene {
        class SunNode;
        class HeightMapPatch;

        /**
         * A class for creating landscapes through heightmaps
         *
         * The landscape starts in (0, 0) with it's lower left cornor
         * and has it's depth along the x-axis and width along the
         * z-axis.
         *
         *   X
         * D A
         * E |
         * P |
         * T |
         * H |
         *  -+----->Z
         *    WIDTH
         */
        class HeightMapNode : public ISceneNode, 
                              public IListener<RenderingEventArg>, 
                              public IListener<ProcessEventArg> {
            OE_SCENE_NODE(HeightMapNode, ISceneNode)

        public:
            static const int DIMENSIONS = 4;
            static const int TEXCOORDS = 2;

        protected:
            int numberOfVertices;

            Float4DataBlockPtr vertexBuffer;
            Float2DataBlockPtr texCoordBuffer;
            Float2DataBlockPtr normalMapCoordBuffer;
            Float3DataBlockPtr geomorphBuffer; // {PatchCenterX, PatchCenterZ, LOD}

            float* normals;
            FloatTexture2DPtr normalmap;
            Float3DataBlockPtr normalBuffer;

            GeometrySetPtr geom;

            char* deltaValues;

            IndicesPtr indexBuffer;

            int width;
            int depth;
            int entries;
            float widthScale;
            float heightScale;
            float waterlevel;
            Vector<3, float> offset;

            SunNode* sun;

            float texDetail;

            // Patch variables
            int patchGridWidth, patchGridDepth, numberOfPatches;
            HeightMapPatch** patchNodes;

            // Distances for changing the LOD
            float baseDistance;
            float invIncDistance;

            FloatTexture2DPtr tex;
            IShaderResourcePtr landscapeShader;

            bool isLoaded;

        public:
            HeightMapNode() {}
            HeightMapNode(FloatTexture2DPtr tex);
            ~HeightMapNode();

            void Load();

            void CalcLOD(Display::IViewingVolume* view);
            void Render(Display::Viewport view);
            void RenderBoundingGeometry();

            void VisitSubNodes(ISceneNodeVisitor& visitor);

            void Handle(RenderingEventArg arg);
            void Handle(ProcessEventArg arg);

            FloatTexture2DPtr GetTex() {return tex;}

            // *** Get/Set methods ***

            /**
             * Takes as argument a 3D vector in worldspace and returns
             * the height of the heightmap at that point.
             *
             * @return The height at the given point.
             */
            float GetHeight(Vector<3, float> point) const;
            /**
             * Takes as argument an x- and z-coord in worldspace and
             * returns the height of the heightmap at that point.
             *
             * @return The height at the given point.
             */
            float GetHeight(float x, float z) const;
            /**
             * Takes as argument a 3D vector in worldspace and returns
             * the normal of the heightmap at that point.
             *
             * @return The normal at the given point.
             */
            Vector<3, float> GetNormal(Vector<3, float> point) const;
            /**
             * Takes as argument an x- and z-coord in worldspace and
             * returns the normal of the heightmap at that point.
             *
             * @return The normal at the given point.
             */
            Vector<3, float> GetNormal(float x, float z) const;
            /**
             * Takes as argument a 3D vector in worldspace, a
             * direction and returns the direction reflected of the
             * heightmap at that point.
             *
             * @return The reflected direction  at the given point.
             */
            Vector<3, float> GetReflectedDirection(Vector<3, float> point, Vector<3, float> direction) const;
            /**
             * Takes as argument an x- and z-coord in worldspace, a
             * direction and returns the direction reflected of the
             * heightmap at that point.
             *
             * @return The reflected direction  at the given point.
             */
            Vector<3, float> GetReflectedDirection(float x, float z, Vector<3, float> direction) const;

            IDataBlockPtr GetVertexBuffer() const { return vertexBuffer; }
            IDataBlockPtr GetGeomorphBuffer() const { return geomorphBuffer; }
            IDataBlockPtr GetTexCoordBuffer() const { return texCoordBuffer; }
            IDataBlockPtr GetNormalMapCoordBuffer() const { return normalMapCoordBuffer; }
            IndicesPtr    GetIndices() const { return indexBuffer; }
            int GetNumberOfVertices() const { return numberOfVertices; }
            GeometrySetPtr GetGeometrySet() const { return geom; }

            int GetIndice(int x, int z);
            float* GetVertex(int x, int z);
            void SetVertex(int x, int z, float value);
            void SetVertices(int x, int z, int width, int depth, float* values);
            Vector<3, float> GetNormal(int x, int z);

            void SetHeightScale(const float scale) { heightScale = scale; }
            void SetWidthScale(const float scale) { widthScale = scale; }
            int GetWidth() const { return width * widthScale; }
            int GetDepth() const { return depth * widthScale; }
            int GetVerticeWidth() const { return width; }
            int GetVerticeDepth() const { return depth; }
            float GetWidthScale() const { return widthScale; }
            void SetOffset(Vector<3, float> o) { offset = o; }
            Vector<3, float> GetOffset() const { return offset; }

            void SetLODSwitchDistance(const float base, const float inc);
            float GetLODBaseDistance() const { return baseDistance; }
            float GetLODIncDistance() const { return 1.0f / invIncDistance; }
            float GetLODInverseIncDistance() const { return invIncDistance; }

            SunNode* GetSun() const { return sun; }
            void SetSun(SunNode* s) { sun = s; }

            void SetTextureDetail(const float detail);
            void SetLandscapeShader(IShaderResourcePtr shader) { landscapeShader = shader; }
            IShaderResourcePtr GetLandscapeShader() const { return landscapeShader; }

        protected:
            // Virtual HeightMap framework methods

            /**
             * Used to initialize specialization variables.
             *
             * Called after the renderer has been initalized, but
             * before the HeightMap or it's shader has been loaded.
             */
            virtual void Initialize(RenderingEventArg arg) {}
            /**
             * Called whenever a process renderer event is fired.
             *
             * Can be used to update specialization variables, fx time.
             */
            virtual void Process(ProcessEventArg arg) {}
            /**
             * PreRender is called just before Render.  
             * At this point the shader is applied and can be updated.
             */
            virtual void PreRender(Display::Viewport view) {}
            /**
             * PostRender is called just after Render.  
             * Should mostly be used for cleaning up after PreRender
             * is necessary.
             */
            virtual void PostRender(Display::Viewport view) {}

            // Setup methods
            inline void InitArrays();
            inline void SetupNormalMap();
            inline void SetupTerrainTexture();
            inline void CalcTexCoords(int x, int z);
            inline void CalcVerticeLOD();
            inline float CalcGeomorphHeight(int x, int z);
            inline void ComputeIndices();
            inline void SetupPatches();

            /**
             * Returns the index into the arrays based on the coords.
             * The index must be multiplied by the 'size' of the entry.
             */
            inline int CoordToIndex(const int x, const int z) const;
            /**
             * Returns a pointer to the vertice from the indices into
             * the 2D array.
             */
            inline float* GetVertice(const int x, const int z) const;
            inline float* GetVertice(const int index) const;
            /**
             * Returns a pointer to the normal from the indices into
             * the 2D array.
             */
            inline float* GetNormals(const int x, const int z) const;
            inline float* GetNormals(const int index) const;
            inline float* GetTexCoord(const int x, const int z) const;
            inline float* GetNormalMapCoord(const int x, const int z) const;
            inline float* GetGeomorphValues(const int x, const int z) const;
            inline float& GetVerticeLOD(const int x, const int z) const;
            inline float& GetVerticeLOD(const int index) const;
            inline char& GetVerticeDelta(const int x, const int z) const;
            inline char& GetVerticeDelta(const int index) const;
            inline int GetPatchIndex(const int x, const int z) const;
            inline HeightMapPatch* GetPatch(const int x, const int z) const;
        };
    }
} 

#endif

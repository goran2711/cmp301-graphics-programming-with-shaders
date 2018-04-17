// A bounding volume hierarchy (BVH) built on top of an Octree
// Allows for frustum culling to be performed much more efficently than a naive implementation
// Hierarchy is only generated once, so it does not support dynamic objects

#pragma once
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <vector>
#include <memory>

#include "MeshInstance.h"

using namespace DirectX;

class InstanceShader;

class BoundingVolume
{
	template <typename T>
	using pointer = std::shared_ptr<T>;

	// Extent is the type that is stored within the leaf nodes of the BVH,
	// representing a single object
	struct Extent
	{
		// Object's world-space bounding box
		BoundingBox extent;

		// Pointer to an object within the BVH
		MeshInstance* object = nullptr;
	};

	class Octree
	{
	public:
		struct Node
		{
			bool isLeaf = true;

			// The child voxels
			pointer<Node> children[8];

			// Bounding volume that contains all of the node's children
			BoundingBox extent;

			// The bounding box + pointers to objects in this node (leaf)
			std::vector<Extent*> contents;
		};

		// Create the octree by first initialising the root node
		Octree(const BoundingBox& sceneExtent, int maxDepth) : mSceneExtent(sceneExtent), mMaxDepth(maxDepth)
		{
			mRoot = std::make_shared<Node>();
			mRoot->extent = mSceneExtent;
		}

		// Insert a new object into the octree
		void Insert(Extent& newObject)
		{
			Insert(mRoot, mSceneExtent, &newObject, 0);
		}

		// Calculate the extents of all the nodes within the tree once all objects have been inserted
		void Build()
		{
			Build(mRoot, mSceneExtent);
		}

		auto GetRoot() const { return mRoot; }

	private:
		void Insert(pointer<Node> node, const BoundingBox& nodeBounds, Extent* newObject, int depth)
		{
			if (node->isLeaf)
			{
				// If it is an empty leaf, then we have found a place for our new object
				// NOTE: max depth stops infinite recursion from happening when two objects in the tree are
				// in the exact same position
				if (node->contents.empty() || depth >= mMaxDepth)
				{
					node->contents.push_back(newObject);
				}
				// Otherwise, we need to make this node an internal node, and relocate its current content
				else
				{
					// Mark the node as internal
					node->isLeaf = false;
					while (!node->contents.empty())
					{
						// Re-insert node's old contents
						Insert(node, nodeBounds, node->contents.back(), depth);
						node->contents.pop_back();
					}
					// Finally, try inserting the new object again
					Insert(node, nodeBounds, newObject, depth);
				}
			}
			else
			{
				// Determine which of this node's children the new object should be (attempted) inserted into
				int childIdx = 0;

				// TERMINOLOGY: Bounding volume == extent == cell
				//				Cells are split into voxels, and a voxel can itself be a cell

				// If true, right half of cell
				if (newObject->extent.Center.x > nodeBounds.Center.x) childIdx += 4;
				// If true, upper half of cell
				if (newObject->extent.Center.y > nodeBounds.Center.y) childIdx += 2;
				// If true, near half of cell
				if (newObject->extent.Center.z > nodeBounds.Center.z) childIdx += 1;

				// Create new node if it does not already exist
				if (!node->children[childIdx])
				{
					node->children[childIdx] = std::make_shared<Node>();
				}

				// Calculate child bounds
				BoundingBox childBounds = CalculateChildBounds(childIdx, nodeBounds);

				// Insert new object into the new node
				Insert(node->children[childIdx], childBounds, newObject, depth + 1);
			}
		}

		// Creates the bounding box of a child node (cell) within the octree
		BoundingBox CalculateChildBounds(int idx, const BoundingBox& parentExtents)
		{
			XMVECTOR boundMin = XMVectorZero();
			XMVECTOR boundMax = boundMin;

			// If child is in right half
			boundMin = XMVectorSetX(boundMin, (idx & 4) ? parentExtents.Center.x : parentExtents.Center.x - parentExtents.Extents.x);
			boundMax = XMVectorSetX(boundMax, (idx & 4) ? parentExtents.Center.x + parentExtents.Extents.x : parentExtents.Center.x);
			// If child is in upper half
			boundMin = XMVectorSetY(boundMin, (idx & 2) ? parentExtents.Center.y : parentExtents.Center.y - parentExtents.Extents.y);
			boundMax = XMVectorSetY(boundMax, (idx & 2) ? parentExtents.Center.y + parentExtents.Extents.y : parentExtents.Center.y);
			// If child is in near half
			boundMin = XMVectorSetZ(boundMin, (idx & 1) ? parentExtents.Center.z : parentExtents.Center.z - parentExtents.Extents.z);
			boundMax = XMVectorSetZ(boundMax, (idx & 1) ? parentExtents.Center.z + parentExtents.Extents.z : parentExtents.Center.z);

			// Calculate AABB
			BoundingBox childBounds;
			XMStoreFloat3(&childBounds.Center, (boundMin + boundMax) * 0.5f);
			XMStoreFloat3(&childBounds.Extents, (boundMax - boundMin) * 0.5f);

			return childBounds;
		}

		// Calculate the tight-fitting extent of all nodes (bottom-up)
		void Build(pointer<Node> node, const BoundingBox& nodeBounds)
		{
			if (node->isLeaf)
			{
				// Calculate a bounding box that encompasses all the OBJECTS within the (leaf) node
				node->extent = node->contents.back()->extent;

				for (const auto& object : node->contents)
					BoundingBox::CreateMerged(node->extent, node->extent, object->extent);
			}
			else
			{
				// Calculate a bounding box that encompasses all the CHILDREN within the (internal) node
				for (int childIdx = 0; childIdx < 8; ++childIdx)
				{
					if (!node->children[childIdx])
						continue;

					BoundingBox childBounds = CalculateChildBounds(childIdx, nodeBounds);

					Build(node->children[childIdx], childBounds);
					BoundingBox::CreateMerged(node->extent, nodeBounds, node->children[childIdx]->extent);
				}
			}
		}

		pointer<Node> mRoot = nullptr;
		const int mMaxDepth;
		BoundingBox mSceneExtent;
	};

public:
	// Max depth stops it from breaking when there are multiple objects with the exact same position
	static constexpr int MAX_DEPTH = 16;

	BoundingVolume(std::vector<MeshInstance*>& meshes);
	BoundingVolume(MeshInstance* const meshes, int count);

	BoundingVolume(const BoundingVolume&) = delete;
	BoundingVolume& operator=(const BoundingVolume&) = delete;

	~BoundingVolume();

	BoundingBox& GetSceneExtent() { return mSceneExtent; }
	const BoundingBox& GetSceneExtent() const { return mSceneExtent; }

	// Populates a vector of MeshInstance pointers that may be used to render the non-culled objects without instancing
	int GetVisibleGeometry(BoundingFrustum frustum, std::vector<MeshInstance*>& visibleInstances);

	// Adds visible geometry to the InstanceShader's internal list do that the objects may be rendered with instancing
	// Only to be used for objects with the same mesh
	int GetVisibleGeometry(BoundingFrustum frustum, InstanceShader& shader);

	// Adds the bounding volumes of the BVH to an InstanceShader's internal list so that they may be visualised
	void GetBoundingVolumes(InstanceShader& shader, int depth) const;

private:
	void Init(MeshInstance* const meshes, int count);

	void GetBoundingVolumes(pointer<Octree::Node> node, InstanceShader& shader, int depth) const;

	BoundingBox mSceneExtent;
	Extent* mExtents = nullptr;
	pointer<Octree> mOctree;
};


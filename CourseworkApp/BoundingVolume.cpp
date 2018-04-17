#include "BoundingVolume.h"
#include <queue>

#include "InstanceShader.h"

BoundingVolume::BoundingVolume(std::vector<MeshInstance*>& meshes)
{
	Init(*meshes.data(), meshes.size());
}

BoundingVolume::BoundingVolume(MeshInstance * const meshes, int count)
{
	Init(meshes, count);
}

BoundingVolume::~BoundingVolume()
{
	delete[] mExtents;
}

int BoundingVolume::GetVisibleGeometry(BoundingFrustum frustum, std::vector<MeshInstance*>& visibleInstances)
{
	int visibleObjects = 0;

	// Frustum does not intersect with the scene at all
	if (frustum.Contains(mSceneExtent) == ContainmentType::DISJOINT)
		return -1;

	// Traverse the BVH and test for visibility
	std::queue< pointer<Octree::Node> > queue;
	queue.push(mOctree->GetRoot());

	while (!queue.empty())
	{
		auto node = queue.front();
		queue.pop();

		if (node->isLeaf)
		{
			// Frustum check geometry
			for (auto& object : node->contents)
			{
				// Transform object bounding volume to world space
				XMMATRIX world = object->object->GetWorldMatrix();

				BoundingBox boundingBox = object->object->GetBoundingBox();
				boundingBox.Transform(boundingBox, world);

				if (frustum.Contains(boundingBox) != ContainmentType::DISJOINT)
				{
					// Object is either inside or intersecting the frustum, so we draw it
					visibleInstances.push_back(object->object);
					++visibleObjects;
				}
			}
		}
		else
		{
			// Frustum check children
			for (const auto& child : node->children)
			{
				if (!child)
					continue;

				// Queue the child if it is not outside the frustum
				if (frustum.Contains(child->extent) != ContainmentType::DISJOINT)
					queue.push(child);
			}
		}
	}

	return visibleObjects;
}

int BoundingVolume::GetVisibleGeometry(BoundingFrustum frustum, InstanceShader& shader)
{
	std::vector<MeshInstance*> visibleInstances;
	int visibleObjects = GetVisibleGeometry(frustum, visibleInstances);

	// Add the visible objects to the InstanceShader so that they may be rendered in one draw call
	// Assumes all the instances use the same mesh
	for (const auto& instance : visibleInstances)
		shader.addInstance(instance->GetWorldMatrix());

	return visibleObjects;
}

void BoundingVolume::GetBoundingVolumes(InstanceShader& shader, int depth) const
{
	GetBoundingVolumes(mOctree->GetRoot(), shader, depth);
}

void BoundingVolume::Init(MeshInstance * const meshes, int count)
{
	// Initialise scene extent
	meshes[0].GetBoundingBox().Transform(mSceneExtent, meshes[0].GetWorldMatrix());

	mExtents = new Extent[count];

	for (int i = 0; i < count; ++i)
	{
		// Calculate scene extent and hierarchy in world space
		BoundingBox boundingBox = meshes[i].GetBoundingBox();
		boundingBox.Transform(boundingBox, meshes[i].GetWorldMatrix());

		BoundingBox::CreateMerged(mSceneExtent, mSceneExtent, boundingBox);

		mExtents[i].extent = boundingBox;
		mExtents[i].object = &meshes[i];
	}

	mOctree = std::make_shared<Octree>(mSceneExtent, MAX_DEPTH);

	// Insert all the objects into the octree
	for (int i = 0; i < count; ++i)
	{
		mOctree->Insert(mExtents[i]);
	}

	// Calculate the node's tight-fitting bounding volumes
	mOctree->Build();
}

void BoundingVolume::GetBoundingVolumes(pointer<Octree::Node> node, InstanceShader & shader, int depth) const
{
	// Keep traversing the hierarchy until the desired depth has been reached
	if (depth > 0)
	{
		for (int i = 0; i < 8; ++i)
		{
			if (!node->children[i])
				continue;

			GetBoundingVolumes(node->children[i], shader, depth - 1);
		}

		return;
	}

	// Create a transform matrix that can be applied to a cube so that it visualises the bounding volume
	static const auto GetExtentTransform = [](FXMVECTOR center, FXMVECTOR extents)
	{
		XMMATRIX extentTransform = XMMatrixIdentity();
		extentTransform *= XMMatrixScalingFromVector(extents);
		extentTransform *= XMMatrixTranslationFromVector(center);

		return extentTransform;
	};

	if (node->isLeaf)
	{
		XMVECTOR center = XMLoadFloat3(&node->extent.Center);
		XMVECTOR extents = XMLoadFloat3(&node->extent.Extents);

		XMMATRIX extentTransform = GetExtentTransform(center, extents);
		shader.addInstance(extentTransform);
	}
	else
	{
		for (int i = 0; i < 8; ++i)
		{
			if (!node->children[i])
				continue;

			XMVECTOR center = XMLoadFloat3(&node->children[i]->extent.Center);
			XMVECTOR extents = XMLoadFloat3(&node->children[i]->extent.Extents);

			XMMATRIX extentTransform = GetExtentTransform(center, extents);
			shader.addInstance(extentTransform);
		}
	}
}

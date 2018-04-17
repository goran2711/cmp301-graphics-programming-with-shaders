// Manager similar to texture manager that stores all mesh objects

#pragma once
#include "../DXFramework/BaseMesh.h"
#include <unordered_map>
#include <memory>

class MeshManager
{
	using KeyType = std::string;
	using ValueType = std::unique_ptr<BaseMesh>;

	using MeshMap = std::unordered_map<KeyType, ValueType>;
public:

	template <typename T, typename ... Args>
	static T* LoadMesh(const KeyType& key, Args&& ... args)
	{
		static_assert(std::is_base_of<BaseMesh, T>::value, "T does not derive from BaseMesh");

		std::unique_ptr<T> newMesh = std::make_unique<T>(std::forward<Args>(args)...);

		mMeshes[key] = std::move(newMesh);
		return static_cast<T*>(mMeshes[key].get());
	}

	template <typename T = BaseMesh>
	static T* GetMesh(const KeyType& key) 
	{
		static_assert(std::is_base_of<BaseMesh, T>::value, "T does not derive from BaseMesh");

		auto mesh = mMeshes.find(key);

		assert(mesh != mMeshes.end());

		return static_cast<T*>(mesh->second.get());
	}

private:
	static MeshMap mMeshes;
};


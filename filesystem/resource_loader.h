#pragma once
#include "core/globals.h"
#include "core/types/resource.h"
#include <unordered_map>

// Resource Types
#include "project/resources/shader.h"
#include "project/resources/image.h"
using namespace resources;

class ResourceLoader
{
	struct ImportedResource {
		string location;
		string hash;
	};

	// A map containing pointers to all loaded resource instances.
	static std::unordered_map<string, Resource*> loadedResources;
	static std::unordered_map<string, ImportedResource> projectResources;
	
	static void _updateCache(string hash, string filePath, Resource* res);
	static Resource* _load(string filePath);

	public:
	enum class ImportResult {
		IMPORTED,
		IMPORT_FAIL,
		NOT_RECOGNISED
	};
	static void Init();
	static bool IsResourceImported(string filePath);
	static bool HasImportCacheChanged(string filePath);
	static ImportResult ImportResource(string filePath);
	static void Cleanup() {};


	template <typename T>
	static T* Load(const string filePath) {
		Resource* res = _load(filePath);
		return dynamic_cast<T*>(res);
	};
	static Resource* Load(const string filePath) { return _load(filePath); }
};


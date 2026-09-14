#include "resource_loader.h"
#include "engine_io.h"
#include <stdio.h>

// Resource Types
#include "project/resources/shader.h"
#include "project/resources/image.h"

using namespace EngineIO;
std::unordered_map<string, Resource*> ResourceLoader::loadedResources;
std::unordered_map<string, ResourceLoader::ImportedResource> ResourceLoader::projectResources;

void ResourceLoader::Init() {
    File resourceCache = FileSystem::OpenOrCreateFile(".gusengine/resources", std::ios::in);
    fstream* cacheStream = resourceCache.GetFileStream();
    string line;

    while (std::getline(*cacheStream, line)) {
        ImportedResource ir;
        std::stringstream ss(line);
        std::getline(ss, ir.location, ',');
        std::getline(ss, ir.hash);

        projectResources[ir.location] = ir;
    }
}

void ResourceLoader::_updateCache(string hash, string filePath, Resource* res) {
    Log.Debug("ResourceLoader", "Updating cache for " + filePath);
    ImportedResource newCache;
    newCache.hash = hash;
    newCache.location = filePath;

    if (!projectResources.contains(filePath)) {
        File resourceCache = FileSystem::OpenOrCreateFile(".gusengine/resources", std::ios::out);
        fstream* cacheStream = resourceCache.GetFileStream();

        projectResources[filePath] = newCache;
        cacheStream->seekp(0, std::ios::end);
        cacheStream->write(newCache.location.data(), newCache.location.size());
        cacheStream->put(',');
        cacheStream->write(newCache.hash.data(), newCache.hash.size());
        cacheStream->put('\n');

    }
    else {
        std::remove(projectResources[filePath].hash.data());
        projectResources[filePath] = newCache;
        
        File resourceCache = FileSystem::OpenOrCreateFile(".gusengine/resources", std::ios::out | std::ios::trunc);
        fstream* cacheStream = resourceCache.GetFileStream();

        for (const auto& pair: projectResources)
        {
            cacheStream->write(pair.second.location.data(), pair.second.location.size());
            cacheStream->put(',');
            cacheStream->write(pair.second.hash.data(), pair.second.hash.size());
            cacheStream->put('\n');
        }
    }


    EngineIO::ObjectSaver::SerialiseResourceBinary(res, ".gusengine/" + newCache.hash);
}

bool ResourceLoader::IsResourceImported(string filePath) {
    return projectResources.contains(filePath);
}

bool ResourceLoader::HasImportCacheChanged(string filePath) {
    if (!projectResources.contains(filePath)) return true;
    File extResource = FileSystem::OpenFile(filePath, std::ios::binary | std::ios::in);
    return projectResources[filePath].hash != extResource.GetHash();

}

ResourceLoader::ImportResult ResourceLoader::ImportResource(string extResourcePath)
{
    Log.Debug("ResourceLoader", "Importing resource: " + extResourcePath);
    File extResource = FileSystem::OpenFile(extResourcePath, std::ios::binary | std::ios::in);
    string resHash = extResource.GetHash();

    string sourceType = extResource.FileType().erase(0,1);

    string supportedImageTypes[11] = {
        "jpg", "jpeg", "png", "bmp", "psd",
        "tga", "gif", "hdr", "pic", "ppm", "pgm"
    };

    if (ranges::find(supportedImageTypes, sourceType) != std::end(supportedImageTypes)) {
        
    }

    string supportedShaderTypes[9] = {
        "vert", "frag", "tesc", "tese", "geom", "comp", "glsl", "hlsl", "spv"
    };

    if (ranges::find(supportedShaderTypes, sourceType) != std::end(supportedShaderTypes)) {
        Shader::ShaderStage stage{};
        if (sourceType == "vert") stage = Shader::ShaderStage::StageVert;
        if (sourceType == "frag") stage = Shader::ShaderStage::StageFrag;
        if (sourceType == "tesc") stage = Shader::ShaderStage::StageTessControl;
        if (sourceType == "tese") stage = Shader::ShaderStage::StageTessEval;
        if (sourceType == "geom") stage = Shader::ShaderStage::StageGeom;
        if (sourceType == "comp") stage = Shader::ShaderStage::StageComp;
        auto shader = new Shader(extResource.ReadAllText(), Shader::ShaderLanguage::LanguageGLSL, stage);
        _updateCache(resHash, extResourcePath, shader);
        loadedResources[extResourcePath] = shader;

        return ImportResult::IMPORTED;
    }


    return ImportResult::NOT_RECOGNISED;
}


// Loads a resource from the filesystem
Resource* ResourceLoader::_load(string filePath) {
    if (filePath.ends_with(".rmeta")) {
        Log.Error("ResourceLoader", "Cannot import metadata file:" + filePath + " - import the actual resource instead");
    }

	if (loadedResources.contains(filePath)) {
		return loadedResources[filePath];
	}

    if (projectResources.contains(filePath) && !HasImportCacheChanged(filePath)) {
        Log.Debug("ResourceLoader", "Loading cached resource: " + filePath);
        Resource* r = ObjectLoader::LoadSerialisedResourceBinary(".gusengine/" + projectResources[filePath].hash);
        loadedResources[filePath] = r;
        return r;
    }
    
    if (filePath.ends_with(".res")) {
        Resource* r = ObjectLoader::LoadSerialisedResourceText(filePath);
        loadedResources[filePath] = r;
        return r;
    }
    else {
        ImportResult result = ImportResource(filePath);
        if (result == ImportResult::NOT_RECOGNISED) {
            Log.Error("ResourceLoader", "Unrecognised resource file type: " + filePath);
        }
        else if (result == ImportResult::IMPORT_FAIL) {
            Log.Error("ResourceLoader", "Failed to import resource: " + filePath);

        }
        else if (result == ImportResult::IMPORTED) {
            return loadedResources[filePath];
        }
    }

    Log.Error("ResourceLoader", "Resource does not exist: " + filePath);

    return nullptr;
}


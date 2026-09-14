#pragma once
#include "core/types/object.h"
#include "core/types/resource.h"
#include <vector>
#include <fstream>
#include <filesystem>
#include "resource_loader.h"
using namespace resources;
namespace EngineIO {
	class FileSystem;

	class File {
		friend FileSystem;
		private:
		std::fstream _file;
		string _path;
		string _type;
		string _name;

		File(string filePath, std::ios_base::openmode mode) {
			_path = filePath;
			_type = filesystem::path(filePath).extension().string();
			_name = filesystem::path(filePath).stem().string();
			_file = std::fstream(filePath, mode);
		}

		public:
		string FilePath() const {return _path; }
		string FileName() const {return _name; }
		string FileType() const {return _type; }

		string ReadAllText() const;

		string GetHash() const;

		vector<uint8_t> ReadAllBinary() const;

		fstream* GetFileStream() {return &_file; }

		~File() {
			_file.close();
		}
	};



	class FileSystem {
		public:
		static void Init() {};
		static bool FileExists(string filePath) {
			return filesystem::exists(filePath);
		};

		static vector<string> GetFilesInDir(string dirPath, bool recursive = false) {
			filesystem::path dp = filesystem::path(dirPath);
			vector<string> files;
			if (recursive) {
				for (const filesystem::directory_entry entry : filesystem::recursive_directory_iterator(dp)) {
					if (!entry.is_directory()) files.push_back(filesystem::relative(entry.path(), dp).string());
				}
			}
			else {
				for (const filesystem::directory_entry entry : filesystem::directory_iterator(dp)) {
					if (!entry.is_directory()) files.push_back(filesystem::relative(entry.path(), dp).string());
				}
			}
			return files;
		};

		static File OpenFile(string filePath, std::ios_base::openmode mode) {
			if (!FileExists(filePath)) {
				Log.Error("EngineIO", "Cannot open file: " + filePath + " - doesn't exist.");
			}
			return File(filePath, mode);
		};
		static File OpenOrCreateFile(string filePath, std::ios_base::openmode mode) {
			return File(filePath, mode);
		}
	};

	class ObjectSaver {
		public:
		static void SerialiseResourceBinary(Resource* res, std::string filepath);
		static void SerialiseResourceText(Resource res, std::string filepath);
	};

	class ObjectLoader {
		private:
		static Variant LoadBinaryVariant(File* input);
		public:
		static Resource* LoadSerialisedResourceBinary(std::string filepath);
		static Resource* LoadSerialisedResourceText(std::string filepath);
	};
}
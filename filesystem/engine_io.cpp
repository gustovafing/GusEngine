#include "engine_io.h"
#include "core/types/object.h"
#include <external/md5.h>
using namespace resources;
using namespace EngineIO;

string File::ReadAllText() const {
	fstream f = fstream(_path, std::ios::ate | std::ios::in | std::ios::out);
	const size_t fileSize = f.tellg();
	std::vector<char> buffer(fileSize + 1);
	f.seekg(0);
	f.read(buffer.data(), fileSize);
	f.close();

	return string(buffer.data());
}

string File::GetHash() const {
	const string data = ReadAllText();
	return md5::hash(data.data(), data.size());
}

vector<uint8_t> File::ReadAllBinary() const {
	auto f = fstream(_path, std::ios::ate | std::ios::binary | std::ios::in | std::ios::out);
	const size_t fileSize = (size_t)f.tellg();
	std::vector<char> buffer(fileSize);
	f.seekg(0);
	f.read(buffer.data(), fileSize);
	f.close();
	return { buffer.begin(), buffer.end() };
}

void ObjectSaver::SerialiseResourceBinary(Resource* res, std::string filepath)
{
	File outFile = FileSystem::OpenOrCreateFile(filepath, std::ios::binary | std::ios::out);
	fstream* outStream = outFile.GetFileStream();
	string className = res->_ClassName();
	string resourceName = res->Name();
	outStream->write(className.c_str(), className.size());
	outStream->put(0x00);
	outStream->write(resourceName.c_str(), resourceName.size());
	outStream->put(0x00);

	map<string, ObjectRTTIModel::ObjectPropertyDefinition> properties = res->_GetPropertyList();
	map<string, ObjectRTTIModel::ObjectPropertyDefinition>::iterator it = properties.begin();

	while (it != properties.end()) {
		outStream->write(it->first.c_str(), it->first.size());
		outStream->put(0x00);
		Variant value = res->_Call(it->second.getterName);
		char* val = Variant::BinarySerialise(value);
		outStream->write(val, Variant::BinarySerialisationLength(val));
		delete val;
		it++;
	}

}

void ObjectSaver::SerialiseResourceText(Resource res, std::string filepath)
{
	fstream* outFile = (FileSystem::OpenOrCreateFile(filepath, std::ios::out).GetFileStream());
	*outFile << "[" << res._ClassName() << "] " << res.Name()  << std::endl;

	map<string, ObjectRTTIModel::ObjectPropertyDefinition> properties = res._GetPropertyList();
	map<string, ObjectRTTIModel::ObjectPropertyDefinition>::iterator it = properties.begin();

	while (it != properties.end()) {
		Variant value = res._Call(it->second.getterName);
		*outFile << it->first << ": " << Variant::StringSerialise(value) << "" << std::endl;
		it++;
	}

}

Variant ObjectLoader::LoadBinaryVariant(File* input)
{
	fstream* inStream = input->GetFileStream();
	short* typeBin = new short;
	inStream->read((char*)typeBin, sizeof(short));
	Variant::StoredType type = static_cast<Variant::StoredType>(*typeBin);
	delete typeBin;

	char valB = 0;
	int32_t valInt = 0;
	uint32_t valUInt = 0;
	int64_t valLLong = 0;
	uint64_t valULLong = 0;
	float valFl = 0;
	double valDb = 0;
	int32_t strSize = 0;

	switch (type) {
		case Variant::Empty:
		case Variant::Void:
			return Variant(Variant::Void);
		case Variant::Bool:
			inStream->read(&valB, 1);
			return valB == 0xFF;
		case Variant::Int32:
			inStream->read((char*)&valInt, sizeof(int32_t));
			return valInt;
		case Variant::UInt32:
			inStream->read((char*)&valUInt, sizeof(uint32_t));
			return valUInt;
		case Variant::Int64:
			inStream->read((char*)&valLLong, sizeof(int64_t));
			return valLLong;
		case Variant::UInt64:
			inStream->read((char*)&valULLong, sizeof(uint64_t));
			return valULLong;
		case Variant::Float:
			inStream->read((char*)&valFl, sizeof(float));
			return valFl;
		case Variant::Double:
			inStream->read((char*)&valDb, sizeof(double));
			return valDb;
		case Variant::String:
			inStream->read((char*)&strSize, sizeof(int32_t));
			char* valStr = new char[strSize+1];
			inStream->read(valStr, strSize);
			valStr[strSize] = 0x00;
			return std::string(valStr);
	}
	return Variant();
}

Resource* ObjectLoader::LoadSerialisedResourceBinary(std::string filepath)
{
	File file = FileSystem::OpenFile(filepath, std::ios::in | std::ios::binary);
	fstream* inFile = file.GetFileStream();

	if (!inFile->is_open()) {
		Log.Error("EngineIO", "Cannot load file " + filepath);
	}

	std::string type;
	std::string name;
	
	char ch;
	while (inFile->read(&ch, 1)) {
		if (ch == '\0') {
			break;
		}
		type += ch; 
	}
	while (inFile->read(&ch, 1)) {
		if (ch == '\0') {
			break;
		}
		name += ch;
	}

	engine_type_registry::EngineClass engCls = engine_type_registry::type_registry::_registered_classes[type];
	Resource* res = dynamic_cast<Resource*>((*engCls._dynamic_constructor)());

	std::string currentProp;
	char* currentType = new char[VARIANT_ENUM_SIZE];
	while (!inFile->eof()) {
		while (inFile->read(&ch, 1)) {
			if (ch == '\0') {
				break;
			}
			currentProp += ch;
		}

		res->_Set(currentProp, LoadBinaryVariant(&file));
		currentProp = "";
	}
	delete[] currentType;
	res->_Init();
	return res;
}

Resource* ObjectLoader::LoadSerialisedResourceText(std::string filepath)
{
	File file = FileSystem::OpenFile(filepath, std::ios::in);
	return nullptr;
}
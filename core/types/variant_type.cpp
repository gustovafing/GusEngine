#include "variant_type.h"

#include <cstring>
#include <regex>

bool Variant::_same(const Variant& v1, const Variant& v2)
{
	if (v1.Type() != v2.Type()) return false;
	switch (v1.Type()) {
		case Empty:
		case Void:
			return true;
		case Bool:
			return v1.Value<bool>() == v2.Value<bool>();
		case Int32:
			return v1.Value<int32_t>() == v2.Value<int32_t>();
		case UInt32:
			return v1.Value<uint32_t>() == v2.Value<uint32_t>();
		case Int64:
			return v1.Value<int64_t>() == v2.Value<int64_t>();
		case UInt64:
			return v1.Value<uint64_t>() == v2.Value<uint64_t>();
		case Float:
			return v1.Value<float>() == v2.Value<float>();
		case Double:
			return v1.Value<double>() == v2.Value<double>();
		case String:
			return v1.Value<std::string>() == v2.Value<std::string>();
		case VariantArray:
			return v1.Value<std::vector<Variant>>() == v2.Value<std::vector<Variant>>();
		case UInt32Array:
			return false;
	}
	return false;
}

char* Variant::BinarySerialise(Variant v)
{
	char* buffer;
	short typeVal = v._currentType;
	switch (v.Type()) {
		case Empty:
		case Void:
			buffer = new char[VARIANT_ENUM_SIZE];
			memcpy(buffer, &typeVal, VARIANT_ENUM_SIZE);
			buffer[0] = static_cast<char>(v.Type());
			return buffer;
		case Bool:
			buffer = new char[VARIANT_ENUM_SIZE + 1];
			memcpy(buffer, &typeVal, VARIANT_ENUM_SIZE);
			buffer[VARIANT_ENUM_SIZE] = v.Value<bool>() ? 0xFF : 0x00;
			return buffer;
		case Int32:
			buffer = new char[VARIANT_ENUM_SIZE + sizeof(int32_t)];
			memcpy(buffer, &typeVal, VARIANT_ENUM_SIZE);
			memcpy(buffer + VARIANT_ENUM_SIZE, &v._primitiveData, sizeof(int32_t));
			return buffer;
		case UInt32:
			buffer = new char[VARIANT_ENUM_SIZE + sizeof(uint32_t)];
			memcpy(buffer, &typeVal, VARIANT_ENUM_SIZE);
			memcpy(buffer + VARIANT_ENUM_SIZE, &v._primitiveData, sizeof(uint32_t));
			return buffer;
		case Int64:
			buffer = new char[VARIANT_ENUM_SIZE + sizeof(int64_t)];
			memcpy(buffer, &typeVal, VARIANT_ENUM_SIZE);
			memcpy(buffer + VARIANT_ENUM_SIZE, &v._primitiveData, sizeof(int64_t));
			return buffer;
		case UInt64:
			buffer = new char[VARIANT_ENUM_SIZE + sizeof(uint64_t)];
			memcpy(buffer, &typeVal, VARIANT_ENUM_SIZE);
			memcpy(buffer + VARIANT_ENUM_SIZE, &v._primitiveData, sizeof(uint64_t));
			return buffer;
		case Float:
			buffer = new char[VARIANT_ENUM_SIZE + sizeof(float)];
			memcpy(buffer, &typeVal, VARIANT_ENUM_SIZE);
			memcpy(buffer + VARIANT_ENUM_SIZE, &v._primitiveData, sizeof(float));
			return buffer;
		case Double:
			buffer = new char[VARIANT_ENUM_SIZE + sizeof(double)];
			memcpy(buffer, &typeVal, VARIANT_ENUM_SIZE);
			memcpy(buffer + VARIANT_ENUM_SIZE, &v._primitiveData, sizeof(double));
			return buffer;
		case String: {
			string* stringPtr = static_cast<string *>(v._primitiveData._ptr);
			int32_t size = static_cast<int32_t>(stringPtr->size());
			buffer = new char[VARIANT_ENUM_SIZE + sizeof(int32_t) + size];
			memcpy(buffer, &typeVal, VARIANT_ENUM_SIZE);
			memcpy(buffer + VARIANT_ENUM_SIZE, &size, sizeof(int32_t));
			memcpy(buffer + VARIANT_ENUM_SIZE + sizeof(int32_t), stringPtr->data(), size);
			return buffer;
		}
		case VariantArray:
			Log.Warn("Variant", "Cannot serialise variant arr");
			break;
		case UInt32Array:
			Log.Warn("Variant", "Cannot serialise uint32 arr");
			break;
	}
	throw runtime_error("");
}

int32_t Variant::BinarySerialisationLength(const char* bin) {
	const auto typeBin = new short;
	memcpy(typeBin, bin, VARIANT_ENUM_SIZE);
	const auto type = static_cast<StoredType>(*typeBin);
	delete typeBin;
	int32_t len = 0;
	switch (type) {
		case Empty:
		case Void:
			len = 0;
			break;
		case Bool:
			len = sizeof(char);
			break;
		case Int32:
			len = sizeof(int32_t);
			break;
		case UInt32:
			len = sizeof(uint32_t);
			break;
		case Int64:
			len = sizeof(int64_t);
			break;
		case UInt64:
			len = sizeof(uint64_t);
			break;
		case Float:
			len = sizeof(float);
			break;
		case Double:
			len = sizeof(double);
			break;
		case String: {
			int32_t size = 0;
			memcpy(&size, bin + VARIANT_ENUM_SIZE, sizeof(int32_t));
			len = sizeof(int32_t) + sizeof(char) * size;
			break;
		}
		case VariantArray:
			break;
		case UInt32Array:
			break;
	}
	return len + VARIANT_ENUM_SIZE;
}


std::string Variant::StringSerialise(Variant v)
{
	switch (v.Type()) {
		case Empty:
		case Void:
			return "null";
		case Bool:
			return v.Value<bool>() ? "true" : "false";
		case Int32:
			return "Int32" + std::to_string(v.Value<int32_t>());
		case UInt32:
			return "UInt32" + std::to_string(v.Value<uint32_t>());
		case Int64:
			return "Llong" + std::to_string(v.Value<int64_t>());
		case UInt64:
			return "ULLong" + std::to_string(v.Value<uint64_t>());
		case Float:
			return "Float" + std::to_string(v.Value<float>());
		case Double:
			return "Double" + std::to_string(v.Value<double>());
		case String:
			return "\"" + v.Value<string>() + "\"";
		case VariantArray:
			return "";
		case UInt32Array:
			return "";
	}
	return "";
}

Variant Variant::FromString(const std::string* str)
{
	const std::regex pattern (R"(^([A-Za-z]+)(.+);$)");
	std::smatch match;

	if (!std::regex_search(*str, match, pattern)) {
		return Variant(Empty);
	}

	const string type = match[1];
	string content = match[2];

	if (str->starts_with(";")) {
		return Variant(Void);
	}
	if (type == "Bool") {
		return Variant(content == "1");
	}
	if (type == "Int32") {
		return Variant(std::stoi(content));
	}
	if (type == "UInt32") {
		return Variant(static_cast<uint32_t>(std::stoul(content)));
	}
	if (type == "LLong") {
		return Variant(std::stoll(content));
	}
	if (type == "ULLong") {
		return Variant(std::stoull(content));
	}
	if (type == "Float") {
		return Variant(std::stof(content));
	}
	if (type == "Double") {
		return Variant(std::stod(content));
	}
	if (type == "String") {
		content.pop_back();
		return Variant(content.substr(1));
	}

	return Variant(Empty);
}
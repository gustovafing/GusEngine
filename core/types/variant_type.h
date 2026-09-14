#pragma once
#include <string>
#include "core/globals.h"
#include <concepts>
#include <type_traits>
#include <vector>


template <typename T>
concept IsIntegerEnum = (std::is_enum_v<T>) && (std::is_same_v<std::underlying_type_t<T>, int32_t>);

template <typename T>
concept VariantScalar = std::is_arithmetic_v<std::decay_t<T>>; // covers bool, int32_t, uint32_t, int64_t, uint64_t, float, double

// Implementation of a variant data type, which can dynamically hold primitve data types, used to correctly cast data for use with RTTI.
struct Variant {
	enum StoredType: short {
		// NOTE: Empty and Void represent two different types - A variant that has not been assigned, vs an assigned variant holding nothing.
		Empty = 0,
		Void = (1u << 0),

		// Primitive types:
		Bool = (1u << 1),
		Int32 = (1u << 2),
		UInt32 = (1u << 3),
		Int64 = (1u << 4),
		UInt64 = (1u << 5),
		Float = (1u << 6),
		Double = (1u << 7),

		// Complex types:
		String = (1u << 8),
		VariantArray = (1u << 9),
		UInt32Array = (1u << 10),
		// WIP

	};

	static string VariantTypeToString(StoredType type) {
		switch (type) {
			case StoredType::Empty:
				return "Empty";
			case StoredType::Void:
				return "Void";
			case StoredType::Bool:
				return "Bool";
			case StoredType::Int32:
				return "Int32";
			case StoredType::UInt32:
				return "UInt32";
			case StoredType::Int64:
				return "Int64";
			case StoredType::UInt64:
				return "UInt64";
			case StoredType::Float:
				return "Float";
			case StoredType::Double:
				return "Double";
			case StoredType::String:
				return "String";
			case StoredType::VariantArray:
				return "VariantArray";
			case StoredType::UInt32Array:
				return "UInt32Array";
			default:
				return "";
		}
	}

	private:

	union variantData {
		bool _bool;
		int32_t _int;
		uint32_t _uint;
		int64_t _llong;
		uint64_t _ullong;
		float _float;
		double _double;

		void* _ptr;	
	} _primitiveData;

	StoredType _currentType;

	static bool _same(const Variant& v1, const Variant& v2);

	public:
	StoredType Type() const { return _currentType; }

	template <typename T>
	T Value() const {
		return static_cast<T>(*this);
	}

	// Returns true if the value seems to be false like or 0 like.
	bool IsValueFalseLike() const {
		if (IsEmptyOrVoid()) return true;
		switch (_currentType) {
			case StoredType::String:
				return static_cast<string *>(_primitiveData._ptr)->empty();
			case StoredType::VariantArray:
				return static_cast<std::vector<Variant> *>(_primitiveData._ptr)->empty();
			default:
				return this->operator bool() == false;
		}
	}


	bool IsEmptyOrVoid() const {
		return _currentType & (StoredType::Void | StoredType::Empty);
	}

	bool IsInt() const {
		return (_currentType & (StoredType::Int32 | StoredType::UInt32 | StoredType::Int64 | StoredType::UInt64));
	};

	bool IsNumeric() const {
		return (_currentType & (StoredType::Int32 | StoredType::UInt32 | StoredType::Int64 | StoredType::UInt64 | StoredType::Float | StoredType::Double));
	};

	bool IsSignedInt() const {
		return (_currentType & (StoredType::Int32 | StoredType::Int64));
	};

	static char* BinarySerialise(Variant v);
	static int32_t BinarySerialisationLength(char* bin);
	static std::string StringSerialise(Variant v);
	static Variant FromString(std::string* str);
	
	//// Operators and constructors

	Variant() { 
		_primitiveData._int = 0; 
		_currentType = StoredType::Empty; 
	};

	Variant(StoredType type) {
		_primitiveData._int = 0;
		_currentType = type;
	}

	Variant(const Variant& other) {
		_currentType = other._currentType;
		switch (_currentType)
		{
			case Variant::Bool:
			case Variant::Int32:
			case Variant::UInt32:
			case Variant::Int64:
			case Variant::UInt64:
			case Variant::Float:
			case Variant::Double:
				_primitiveData = other._primitiveData;
				break;
			case Variant::String:
				_primitiveData._ptr = new string(*(string*)other._primitiveData._ptr);
				break;
			case Variant::VariantArray:
				_primitiveData._ptr = new std::vector<Variant>(*(std::vector<Variant>*)other._primitiveData._ptr);
				break;
			case Variant::UInt32Array:
				_primitiveData._ptr = new std::vector<uint32_t>(*(std::vector<uint32_t>*)other._primitiveData._ptr);
				break;
			default:
				_primitiveData._int = 0;
				break;
		}
	};

	Variant& operator=(const Variant& other) {
		if (this == &other) return *this;
		_currentType = other._currentType;
		switch (_currentType)
		{
			case Variant::Bool:
			case Variant::Int32:
			case Variant::UInt32:
			case Variant::Int64:
			case Variant::UInt64:
			case Variant::Float:
			case Variant::Double:
				_primitiveData = other._primitiveData;
				break;
			case Variant::String:
				_primitiveData._ptr = new string(*(string*)other._primitiveData._ptr);
				break;
			case Variant::VariantArray:
				_primitiveData._ptr = new std::vector<Variant>(*(std::vector<Variant>*)other._primitiveData._ptr);
				break;
			case Variant::UInt32Array:
				_primitiveData._ptr = new std::vector<uint32_t>(*(std::vector<uint32_t>*)other._primitiveData._ptr);
				break;
			default:
				_primitiveData._int = 0;
				break;
		}
		return *this;
	}

	Variant(Variant&& other) noexcept {
		_primitiveData = other._primitiveData;
		_currentType = other._currentType;
		other._primitiveData._int = 0;
		other._currentType = StoredType::Empty;
	};


	template<typename A> requires (!std::same_as<std::decay_t<A>, Variant>)
	Variant(A&& data) {
		using T = std::decay_t<A>;
		if constexpr (IsIntegerEnum<T>) {
			_primitiveData._int = static_cast<int32_t>(data);
			_currentType = StoredType::Int32;
		}
		else if constexpr (std::is_same_v<T, bool>) {
			_currentType = StoredType::Bool;
			_primitiveData._bool = data;
		}
		else if constexpr (std::is_same_v<T, int32_t>) {
			_currentType = StoredType::Int32;
			_primitiveData._int = data;
		}
		else if constexpr (std::is_same_v<T, uint32_t>) {
			_currentType = StoredType::UInt32;
			_primitiveData._uint = data;
		}
		else if constexpr (std::is_same_v<T, int64_t>) {
			_currentType = StoredType::Int64;
			_primitiveData._llong = data;
		}
		else if constexpr (std::is_same_v<T, uint64_t>) {
			_currentType = StoredType::UInt64;
			_primitiveData._ullong = data;

		}
		else if constexpr (std::is_same_v<T, float>) {
			_currentType = StoredType::Float;
			_primitiveData._float = data;
		}
		else if constexpr (std::is_same_v<T, double>) {
			_currentType = StoredType::Double;
			_primitiveData._double = data;
		}
		else if constexpr (std::is_same_v<T, const char*>) {
			_currentType = StoredType::String;
			_primitiveData._ptr = new string(data);
		}
		else if constexpr (std::is_same_v<T, string>) {
			_currentType = StoredType::String;
			_primitiveData._ptr = new std::string(std::forward<T>(data));
		}
		else if constexpr (std::is_same_v<T, std::vector<Variant>>) {
			_currentType = StoredType::VariantArray;
			_primitiveData._ptr = new std::vector<Variant>(std::forward<T>(data));
		}
		else if constexpr (std::is_same_v<T, std::vector<uint32_t>>) {
			_currentType = StoredType::UInt32Array;
			_primitiveData._ptr = new std::vector<uint32_t>(std::forward<T>(data));
		}
		else {
			static_assert(false, "Invalid Variant constructor call");
		}
	}

	~Variant() {
		switch (_currentType)
		{
			case Variant::String:
			case Variant::VariantArray:
			case Variant::UInt32Array:
				delete _primitiveData._ptr;
				break;
			default:
				break;
		}
	};

	template<typename T> requires VariantScalar<T>
	explicit operator T() const {
		using U = std::decay_t<T>;
		if constexpr (std::is_same_v<U, bool>) {
			switch (_currentType) {
				case StoredType::Bool: return _primitiveData._bool;
				case StoredType::Int32: return _primitiveData._int != 0;
				case StoredType::UInt32: return _primitiveData._uint != 0;
				case StoredType::Int64: return _primitiveData._llong != 0;
				case StoredType::UInt64: return _primitiveData._ullong != 0;
				case StoredType::Float: return _primitiveData._float != 0.0f;
				case StoredType::Double: return _primitiveData._double != 0.0;
				default: return false;
			}
		}
		else if constexpr (std::is_arithmetic_v<U>) {
			switch (_currentType) {
				case StoredType::Bool: return static_cast<U>(_primitiveData._bool);
				case StoredType::Int32: return static_cast<U>(_primitiveData._int);
				case StoredType::UInt32: return static_cast<U>(_primitiveData._uint);
				case StoredType::Int64: return static_cast<U>(_primitiveData._llong);
				case StoredType::UInt64: return static_cast<U>(_primitiveData._ullong);
				case StoredType::Float: return static_cast<U>(_primitiveData._float);
				case StoredType::Double: return static_cast<U>(_primitiveData._double);
				default: return U{};
			}
		}
		else {
			return U{};
		}

	}

	operator std::string() const {
		switch (_currentType) {
			case StoredType::Empty:
			case StoredType::Void:
				return "";
			case StoredType::Bool:
				return _primitiveData._bool ? "true" : "false";
			case StoredType::Int32:
				return std::to_string(_primitiveData._int);
			case StoredType::UInt32:
				return std::to_string(_primitiveData._uint);
			case StoredType::Int64:
				return std::to_string(_primitiveData._llong);
			case StoredType::UInt64:
				return std::to_string(_primitiveData._ullong);
			case StoredType::Float:
				return std::to_string(_primitiveData._float);
			case StoredType::Double:
				return std::to_string(_primitiveData._double);
			case StoredType::String:
				return *static_cast<string *>(_primitiveData._ptr);
			default:
				return "";
		}
	}

	explicit operator std::vector<Variant>() const {
		switch (_currentType) {
			case StoredType::VariantArray:
				return *static_cast<std::vector<Variant> *>(_primitiveData._ptr);
			default:
				return {};
		}
	}

	explicit operator std::vector<uint32_t>() const {
		switch (_currentType) {
			case StoredType::UInt32Array:
				return *static_cast<std::vector<uint32_t> *>(_primitiveData._ptr);
			default:
				return {};
		}
	}

	template <typename E> requires IsIntegerEnum<E>
	explicit operator E() const noexcept {
		return static_cast<E>(_primitiveData._int);
	}

	friend bool operator==(const Variant& lhs, const Variant& rhs) {
		return _same(lhs, rhs);
	}
};
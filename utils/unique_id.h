#pragma once
#include <compare>
#include <stdint.h>

class IdGen;

// An integer id used to uniquely identify objects.
class Id {
	friend IdGen;
	private:
	uint32_t _id;
	uint32_t _genId;
	Id(uint32_t id, uint32_t genId): _id(id), _genId(genId) {}

	public:
	Id() : _id(-1), _genId(-1) {}
	std::partial_ordering operator<=>(const Id& other) const{
		if (_genId != other._genId) return std::partial_ordering::unordered;
		return _id <=> other._id;
	}
};

class IdGen {
	private:
	uint32_t _currentId;
	uint32_t _genId;
	static uint32_t _nextGenId;

	public:
	IdGen() {
		_genId = _nextGenId++;
		_currentId = 1;
	}

	Id Next() {
		return Id(_currentId++, _genId);
	}
};
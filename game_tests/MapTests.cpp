#include <iostream>
#include "../game_core/Map.hpp"
#include <cassert>

int main() {
	assert(classifyCell({ "****", "*..*", "****" }, 1, 1) == CellType::Space);
	assert(classifyCell({ "****", "*..*", "****" }, 0, 0) == CellType::Wall);
	assert(classifyCell({ "****", "*..*", "****" }, -1, 0) == CellType::OutOfBounds);
	assert(classifyCell({ "****", "*..*", "****" }, 3, 0) == CellType::OutOfBounds);
	assert(classifyCell({ "****", "*..*", "****" }, 0, -1) == CellType::OutOfBounds);
	assert(classifyCell({ "****", "*..*", "****" }, 0, 4) == CellType::OutOfBounds);
	assert(classifyCell({ "?" }, 0, 0) == CellType::Unknown);
	return 0;
}

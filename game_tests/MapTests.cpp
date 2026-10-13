#include <iostream>
#include "../game_core/Map.hpp"
#include <cassert>

int main() {
	assert(classifyCell({ "****", "*..*", "****" }, 1, 1) == "space");
	assert(classifyCell({ "****", "*..*", "****" }, 0, 0) == "wall");
	assert(classifyCell({ "****", "*..*", "****" }, -1, 0) == "out of bounds");
	assert(classifyCell({ "****", "*..*", "****" }, 3, 0) == "out of bounds");
	assert(classifyCell({ "****", "*..*", "****" }, 0, -1) == "out of bounds");
	assert(classifyCell({ "****", "*..*", "****" }, 0, 4) == "out of bounds");
	return 0;
}
#include "Map.hpp"

std::string classifyCell(const std::vector<std::string>& map, int x, int y) {
	if (x < 0 || x >= map.size()) {
		return "out of bounds";
	}
	else if (y < 0 || y >= map[x].size()) {
		return "out of bounds";
	}
	else if (map[x][y] == '*') {
		return "wall";
	}
	else if (map[x][y] == '.') {
		return "space";
	}
	else {
		return "unknown";
	}

}
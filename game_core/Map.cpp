#include "Map.hpp"

CellType classifyCell(const std::vector<std::string>& map, int x, int y) {
	if (x < 0 || x >= map.size()) {
		return CellType::OutOfBounds;
	}
	else if (y < 0 || y >= map[x].size()) {
		return CellType::OutOfBounds;
	}
	else if (map[x][y] == '*') {
		return CellType::Wall;
	}
	else if (map[x][y] == '.') {
		return CellType::Space;
	}
	else {
		return CellType::Unknown;
	}

}

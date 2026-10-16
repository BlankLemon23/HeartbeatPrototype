#include "Map.hpp"

CellType classifyCell(const std::vector<std::string>& map, int row, int column) {
	if (row < 0 || row >= map.size()) {
		return CellType::OutOfBounds;
	}
	else if (column < 0 || column >= map[row].size()) {
		return CellType::OutOfBounds;
	}
	else if (map[row][column] == '*') {
		return CellType::Wall;
	}
	else if (map[row][column] == '.') {
		return CellType::Space;
	}
	else {
		return CellType::Unknown;
	}

}

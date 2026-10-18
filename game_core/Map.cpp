#include "Map.hpp"

CellType Map::classifyCell(int row, int column) const {
	if (row < 0 || row >= map_.size()) {
		return CellType::OutOfBounds;
	}
	else if (column < 0 || column >= map_[row].size()) {
		return CellType::OutOfBounds;
	}
	else if (map_[row][column] == '*') {
		return CellType::Wall;
	}
	else if (map_[row][column] == '.') {
		return CellType::Space;
	}
	else {
		return CellType::Unknown;
	}

}
void Map::setMap(const std::vector<std::string>& map) {
	map_ = map;
	scannedCells_.clear();
	for (const auto& row : map_) {
		scannedCells_.emplace_back(row.size(), false);
		//为地图的每一行，往 scannedCells_ 里追加一个长度为 row.size()（该行格子数）的内层 vector<bool>，
		// 所有元素初始化为 false（表示都还没扫描）。
	}
}
const std::vector<std::string>& Map::getMap() const {
	return map_;
}

bool Map::isScanned(int row, int column) const {
	if (classifyCell(row, column) == CellType::OutOfBounds) {
		return false;
	}
	return scannedCells_[row][column];
}

bool Map::markScanned(int row, int column) {
	if (classifyCell(row, column) == CellType::OutOfBounds) {
		return false;
	}
	scannedCells_[row][column] = true;
	return true;
}

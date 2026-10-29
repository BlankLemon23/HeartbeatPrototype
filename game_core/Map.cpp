#include "Map.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

void Map::setMap(const std::vector<std::string>& rows) {
	map_ = rows;
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

int Map::rowCount() const {
	return static_cast<int>(map_.size());
}

int Map::maxColumnCount() const {
	int maxColumns = 0;
	for (const auto& row : map_) {
		maxColumns = std::max(maxColumns, static_cast<int>(row.size()));
	}//row.size() 返回的是 std::string::size_type,实际就是 std::size_t(无符号 64 位整数),
	//而 maxColumns 是 int(有符号 32 位)。 所以需要显性数据类型转换
	return maxColumns;
}

CellType Map::classifyCell(int row, int column) const {
	if (row < 0 || row >= rowCount()) {
		return CellType::OutOfBounds;
	}
	if (column < 0 || column >= static_cast<int>(map_[row].size())) {
		return CellType::OutOfBounds;
	}
	if (map_[row][column] == '*') {
		return CellType::Wall;
	}
	if (map_[row][column] == '.') {
		return CellType::Space;
	}
	return CellType::Unknown;
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

bool Map::hasLineOfSight(Position start, Position target, SightMode mode) const {
	if (classifyCell(target.row, target.column) == CellType::OutOfBounds 
		|| 
		classifyCell(start.row, start.column) == CellType::OutOfBounds) {
		return false;
	}

	if (start == target) {
		return true;
	}

	const double startCenterRow = start.row + 0.5;
	const double startCenterColumn = start.column + 0.5;
	const double targetCenterRow = target.row + 0.5;
	const double targetCenterColumn = target.column + 0.5;
	const double rowDistance = std::abs(targetCenterRow - startCenterRow);
	const double columnDistance = std::abs(targetCenterColumn - startCenterColumn);
	const double neverCross = std::numeric_limits<double>::infinity();
	// 这里比较的是射线到达下一条行边界和列边界的进度。
// 数学上两个进度相等时，射线恰好穿过格角；但 double 的计算结果可能有极小误差。
// 因此允许两者相差一点点，仍按穿过格角处理。
	const double cornerTolerance = 1e-12;

	int rowStep = 0;
	if (target.row > start.row) {
		rowStep = 1;
	}
	else if (target.row < start.row) {
		rowStep = -1;
	}
	int columnStep = 0;
	if (target.column > start.column) {
		columnStep = 1;
	}
	else if (target.column < start.column) {
		columnStep = -1;
	}//这样写避免了每次需要通过三元运算进行判断

	double nextRowProgress = neverCross;
	double rowProgressStep = neverCross;
	if (rowDistance > 0.0) {
		nextRowProgress = 0.5 / rowDistance;
		rowProgressStep = 1.0 / rowDistance;
	}
	double nextColumnProgress = neverCross;
	double columnProgressStep = neverCross;
	if (columnDistance > 0.0) {
		nextColumnProgress = 0.5 / columnDistance;
		columnProgressStep = 1.0 / columnDistance;
	}

	while (!(start == target)) {
		if (std::abs(nextRowProgress - nextColumnProgress) <= cornerTolerance) {
			// A wall touching the ray at a grid corner blocks cells behind it.
			if (blocksLineOfSight({start.row + rowStep, start.column}, mode) ||
				blocksLineOfSight({start.row, start.column + columnStep}, mode)) {
				return false;
			}
			start.row += rowStep;
			start.column += columnStep;
			nextRowProgress += rowProgressStep;
			nextColumnProgress += columnProgressStep;
		}
		else if (nextColumnProgress < nextRowProgress) {
			start.column += columnStep;
			nextColumnProgress += columnProgressStep;
		}
		else {
			start.row += rowStep;
			nextRowProgress += rowProgressStep;
		}


		if (start == target) {
			return true; //三个分支写完成一次跨界 再判断最后的位置是否是墙
		}
		if (blocksLineOfSight(start, mode)) {
			return false;
		}
	}
	return true;

}
bool Map::blocksLineOfSight(Position cell, SightMode mode) const {
	const CellType cellType = classifyCell(cell.row, cell.column);
	if (cellType == CellType::OutOfBounds) {
		return true;
	}
	if (cellType != CellType::Wall) {
		return false;
	}
	if (mode == SightMode::RealTerrain) {
		return true;
	}
	return isScanned(cell.row, cell.column);   // 只剩 KnownTerrain + Wall
}
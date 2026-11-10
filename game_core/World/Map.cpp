#include "Map.hpp"
#include "Actors/Enemy.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

Map::Map(MapConfig config) : map_(std::move(config.rows)) {
	scannedCells_.reserve(map_.size());

	for (const auto& row : map_) {
		scannedCells_.emplace_back(row.size(), false);
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
	}
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
	if (classifyCell(start.row, start.column) == CellType::OutOfBounds ||
		classifyCell(target.row, target.column) == CellType::OutOfBounds) {
		return false;
	}
	return traceLine(start, {target.row + 0.5, target.column + 0.5}, mode, {}).targetVisible;
}

Position Map::findLastFreeCell(Position start, Position target) const {
	return traceLine(start, {target.row + 0.5, target.column + 0.5},
		SightMode::RealTerrain, {})
		.lastFreeCell;
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
	return isScanned(cell.row, cell.column);
}

Map::LineTraceResult Map::traceLine(Position start, GridPoint target, SightMode mode,
	const std::vector<Enemy>& enemies) const {
	if (classifyCell(start.row, start.column) == CellType::OutOfBounds) {
		return {start, false};
	}

	const double startCenterRow = start.row + 0.5;
	const double startCenterColumn = start.column + 0.5;
	const double rowDistance = std::abs(target.row - startCenterRow);
	const double columnDistance = std::abs(target.column - startCenterColumn);
	const Position targetCell{
		static_cast<int>(std::floor(target.row)),
		static_cast<int>(std::floor(target.column))};
	const double neverCross = std::numeric_limits<double>::infinity();
	const double cornerTolerance = 1e-12;

	int rowStep = 0;
	if (target.row > startCenterRow) {
		rowStep = 1;
	}
	else if (target.row < startCenterRow) {
		rowStep = -1;
	}
	int columnStep = 0;
	if (target.column > startCenterColumn) {
		columnStep = 1;
	}
	else if (target.column < startCenterColumn) {
		columnStep = -1;
	}

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

	Position current = start;
	while (true) {
		// Inspect every enemy at this cell before advancing the ray once.
		for (const Enemy& enemy : enemies) {
			if (enemy.getPosition() == current) {
				return {current, current == targetCell, current};
			}
		}

		// Progress 1 is the exact endpoint, even when it is outside the map.
		const double nextBoundaryProgress = std::min(nextRowProgress, nextColumnProgress);
		if (nextBoundaryProgress > 1.0 + cornerTolerance) {
			return {current, true};
		}

		Position next = current;
		if (std::abs(nextRowProgress - nextColumnProgress) <= cornerTolerance) {
			// A wall touching the ray at a grid corner blocks cells behind it.
			if (blocksLineOfSight({current.row + rowStep, current.column}, mode) ||
				blocksLineOfSight({current.row, current.column + columnStep}, mode)) {
				return {current, false};
			}
			next.row += rowStep;
			next.column += columnStep;
			nextRowProgress += rowProgressStep;
			nextColumnProgress += columnProgressStep;
		}
		else if (nextColumnProgress < nextRowProgress) {
			next.column += columnStep;
			nextColumnProgress += columnProgressStep;
		}
		else {
			next.row += rowStep;
			nextRowProgress += rowProgressStep;
		}

		const CellType nextCell = classifyCell(next.row, next.column);
		if (blocksLineOfSight(next, mode)) {
			// A target wall is visible, while the landing cell stays before it.
			return {current, next == targetCell && nextCell != CellType::OutOfBounds};
		}
		current = next;
	}
}

Position Map::findFirstEnemyPositionFromPlayerToTarget(
	Position playerStart, GridPoint target, const std::vector<Enemy>& enemies) const {
	return traceLine(playerStart, target, SightMode::RealTerrain, enemies).firstEnemyPosition;
}

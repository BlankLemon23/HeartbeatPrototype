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

Map::LineTraceResult Map::traceLine(Position startCell, GridPoint targetPoint, SightMode mode,
	const std::vector<Enemy>& enemies) const {
	LineTraceResult result;
	result.lastFreeCell = startCell;
	result.targetVisible = false;

	if (classifyCell(startCell.row, startCell.column) == CellType::OutOfBounds) {
		return result;
	}

	const double startCenterRow = startCell.row + 0.5;
	const double startCenterColumn = startCell.column + 0.5;
	const double rowDistance = std::abs(targetPoint.row - startCenterRow);
	const double columnDistance = std::abs(targetPoint.column - startCenterColumn);
	const Position targetCell{
		static_cast<int>(std::floor(targetPoint.row)),
		static_cast<int>(std::floor(targetPoint.column))};

	int rowStep = 0;
	if (targetPoint.row > startCenterRow) {
		rowStep = 1;
	}
	else if (targetPoint.row < startCenterRow) {
		rowStep = -1;
	}
	int columnStep = 0;
	if (targetPoint.column > startCenterColumn) {
		columnStep = 1;
	}
	else if (targetPoint.column < startCenterColumn) {
		columnStep = -1;
	}

	const double neverCross = std::numeric_limits<double>::infinity();
	const double progressTolerance = 1e-12;

	double nextRowBoundaryProgress = neverCross;
	double rowProgressPerCell = neverCross;
	if (rowDistance > 0.0) {
		nextRowBoundaryProgress = 0.5 / rowDistance;
		rowProgressPerCell = 1.0 / rowDistance;
	}
	double nextColumnBoundaryProgress = neverCross;
	double columnProgressPerCell = neverCross;
	if (columnDistance > 0.0) {
		nextColumnBoundaryProgress = 0.5 / columnDistance;
		columnProgressPerCell = 1.0 / columnDistance;
	}

	Position currentCell = startCell;
	while (true) {
		result.lastFreeCell = currentCell;

		for (const Enemy& enemy : enemies) {
			if (enemy.getPosition() == currentCell) {
				result.firstEnemyPosition = currentCell;
				result.targetVisible = currentCell == targetCell;
				return result;
			}
		}

		if (currentCell == targetCell) {
			result.targetVisible = true;
			return result;
		}

		// Advance a candidate cell, then check it before accepting it as currentCell.
		Position nextCell = currentCell;
		if (std::abs(nextRowBoundaryProgress - nextColumnBoundaryProgress) <= progressTolerance) {
			const Position rowNeighbor{currentCell.row + rowStep, currentCell.column};
			const Position columnNeighbor{currentCell.row, currentCell.column + columnStep};

			// Either side of a touched corner can block the ray.
			if (blocksLineOfSight(rowNeighbor, mode) || blocksLineOfSight(columnNeighbor, mode)) {
				return result;
			}
			nextCell.row += rowStep;
			nextCell.column += columnStep;
			nextRowBoundaryProgress += rowProgressPerCell;
			nextColumnBoundaryProgress += columnProgressPerCell;
		}
		else if (nextColumnBoundaryProgress < nextRowBoundaryProgress) {
			nextCell.column += columnStep;
			nextColumnBoundaryProgress += columnProgressPerCell;
		}
		else {
			nextCell.row += rowStep;
			nextRowBoundaryProgress += rowProgressPerCell;
		}

		const CellType nextCellType = classifyCell(nextCell.row, nextCell.column);
		if (blocksLineOfSight(nextCell, mode)) {
			// The first wall is visible when it is the target, but the landing cell stays before it.
			result.targetVisible = nextCell == targetCell && nextCellType != CellType::OutOfBounds;
			return result;
		}
		currentCell = nextCell;
	}
}

Position Map::findFirstEnemyPositionFromPlayerToTarget(
	Position playerStart, GridPoint target, const std::vector<Enemy>& enemies) const {
	return traceLine(playerStart, target, SightMode::RealTerrain, enemies).firstEnemyPosition;
}

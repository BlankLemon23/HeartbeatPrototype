#include "ScanController.hpp"
#include "Player.hpp"
#include <cmath>
#include <limits>

ScanController::ScanController(bool isInBattle,double scanCooldownTime,double ScanRadius) : isInBattle_(isInBattle), scanCooldownTime_(scanCooldownTime), scanRadius_(ScanRadius) {
	
}
void ScanController::setScanCooldownTime(double scanCooldownTime) {
	scanCooldownTime_ = scanCooldownTime;
}

bool ScanController::canScanManually() const {
	if (isInBattle_) {
		return false;
	}
	else if (scanCooldownTime_ > 0) {
		return false;
	}
	
	return true;
}

void ScanController::updateScanTime(double elapsedSeconds) {
	if (scanCooldownTime_ > 0) {
		scanCooldownTime_ -= elapsedSeconds;
	}
};

void ScanController::scan(Position playerPosition, Map& map,double scanCooldownTime) {
	double scanRadiusSquared = scanRadius_ * scanRadius_;
	double rowOffset = 0;
	double columnOffset = 0;
	const auto& cells = map.getMap();
	for (int i = 0;i < cells.size();i++) {
		for (int j = 0;j < cells[i].size();j++) {
			rowOffset = playerPosition.row - i;
			columnOffset = playerPosition.column - j;
			double distanceSquared = rowOffset * rowOffset + columnOffset * columnOffset;

			if (distanceSquared <= scanRadiusSquared && ifPlayerCanSeeTheTarget(playerPosition, Position{ i, j }, map)) {
				map.markScanned(i, j);
			}
			
		}
	}
	setScanCooldownTime(scanCooldownTime);
	
}

bool ScanController::ifPlayerCanSeeTheTarget(Position playerPosition, Position targetPosition, const Map& map) {
	if (playerPosition == targetPosition) {
		return true;
	}

	const double playerCenterRow = playerPosition.row + 0.5;
	const double playerCenterColumn = playerPosition.column + 0.5;
	const double targetCenterRow = targetPosition.row + 0.5;
	const double targetCenterColumn = targetPosition.column + 0.5;
	const double rowDistance = std::abs(targetCenterRow - playerCenterRow);
	const double columnDistance = std::abs(targetCenterColumn - playerCenterColumn);
	const double neverCross = std::numeric_limits<double>::infinity();
	// 这里比较的是射线到达下一条行边界和列边界的进度。
// 数学上两个进度相等时，射线恰好穿过格角；但 double 的计算结果可能有极小误差。
// 因此允许两者相差一点点，仍按穿过格角处理。
	const double cornerTolerance = 1e-12;

	int rowStep = 0;
	if (targetPosition.row > playerPosition.row) {
		rowStep = 1;
	}
	else if (targetPosition.row < playerPosition.row) {
		rowStep = -1;
	}
	int columnStep = 0;
	if (targetPosition.column > playerPosition.column) {
		columnStep = 1;
	}
	else if (targetPosition.column < playerPosition.column) {
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

	while (!(playerPosition == targetPosition)) {
		if (std::abs(nextRowProgress - nextColumnProgress) <= cornerTolerance) {
			// A wall touching the ray at a grid corner blocks cells behind it.
			if (map.classifyCell(playerPosition.row + rowStep, playerPosition.column) == CellType::Wall ||
				map.classifyCell(playerPosition.row, playerPosition.column + columnStep) == CellType::Wall) {
				return false;
			}
			playerPosition.row += rowStep;
			playerPosition.column += columnStep;
			nextRowProgress += rowProgressStep;
			nextColumnProgress += columnProgressStep;
		}
		else if (nextColumnProgress < nextRowProgress) {
			playerPosition.column += columnStep;
			nextColumnProgress += columnProgressStep;
		}
		else {
			playerPosition.row += rowStep;
			nextRowProgress += rowProgressStep;
		}


		if (playerPosition == targetPosition) {
			return true; //三个分支写完成一次跨界 再判断最后的位置是否是墙
		}
		if (map.classifyCell(playerPosition.row, playerPosition.column) == CellType::Wall) {
			return false;
		}
	}
	return true;
}




#include "ScanController.hpp"
#include "Player.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

ScanController::ScanController(double initialScanCooldownSeconds,double ScanRadius) : scanCooldownRemainingSeconds_(initialScanCooldownSeconds), scanRadius_(ScanRadius) {
	
}
void ScanController::setScanCooldownRemainingSeconds(double remainingSeconds) {
	scanCooldownRemainingSeconds_ = remainingSeconds;
}



bool ScanController::canScanManually(const Player& player) const {
	if (player.isInBattle() || player.getHealth() == 0) {
		return false;
	}
	else if (scanCooldownRemainingSeconds_ > 0) {
		return false;
	}
	
	return true;
}

bool ScanController::tryScanManually(const Player& player, Map& map, double scanIntervalSeconds) {
	if (!canScanManually(player)) {
		return false;
	}

	scan(player, map, scanIntervalSeconds);
	return true;
}

// 每帧调用一次，驱动「战斗中的自动扫描」。
//
// 核心思路：用「本帧状态」和「上一帧状态」的差异，识别出玩家“刚进入战斗”的那一帧，
// 从而只在那一帧立即扫描一次；血量降入更低档时也立即扫描一次；其余战斗帧按冷却间隔扫描。
//
// 关键机制：wasInBattle_ 记录的是“上一帧结束时的 isInBattle_”。
//   - 本方法开头先读取 isInBattle（此时 wasInBattle_ 还是上一帧的旧值），
//     因此 `inBattle && !wasInBattle` 恰好表示“这一帧在战斗、但上一帧不在战斗”，
//     也就是刚进入战斗的那一帧。
//   - 在方法末尾才用 setWasInBattle 把本帧状态存下来，供下一帧开头读取。
//     由于“先读后写、且写在末尾”，这个更新天然滞后一帧，于是 was 和 is 始终
//     代表相邻两帧的状态。
//
// 注：正因最后才写，所以“本帧是何时被改成 inBattle 的”并不重要——只要本帧
// 调用本方法之前已经改好即可；写入结果要到下一帧才会被读到。
void ScanController::updateBattleScan(Player& player, Map& map, double highIntervalSeconds, double mediumIntervalSeconds, double lowIntervalSeconds) {
	const HpLevel currentLevel = player.getHpLevel();
	if (player.isInBattle() && currentLevel != HpLevel::Dead) {

		const double automaticIntervalSeconds = getAutoScanTimeAccordingToHpLevel(
			currentLevel, highIntervalSeconds, mediumIntervalSeconds, lowIntervalSeconds);

		const bool enteredBattle = !player.wasInBattle();

		const bool droppedToLowerLevel = previousHpLevel_.has_value() && currentLevel < *previousHpLevel_;
		//previousHpLevel_.has_value() 返回 bool，表示「这个 optional 里当前是否存着一个值」。
		//•	previousHpLevel_ 是 optional，不能直接比较，要先解引用取出里面的值。
		//•* previousHpLevel_ 就是「上一帧的血量等级」。
	    //•	currentLevel < *previousHpLevel_ 表示「当前血量等级比上一帧更低」。
		//枚举类从上到下的顺序是 Dead < Low < Medium < High，所以可以直接用 < 比较。也就是等

		if (enteredBattle || droppedToLowerLevel) {
			scan(player, map, automaticIntervalSeconds);
		}
		else {
			tryScanAutomatically(player, map, automaticIntervalSeconds);
		}
	}
	player.setWasInBattle(player.isInBattle());
	previousHpLevel_ = currentLevel;
}

bool ScanController::tryScanAutomatically(const Player& player, Map& map, double automaticIntervalSeconds) {
	if (!canScanAutomatically(player)) {
		return false;
	}
	scan(player, map, automaticIntervalSeconds);
	return true;
}

bool ScanController::canScanAutomatically(const Player& player) const
{
	if (!player.isInBattle() || player.getHealth() == 0) {
		return false;
	}
	else if (scanCooldownRemainingSeconds_ > 0) {
		return false;
	}

	return true;
}

double ScanController::getAutoScanTimeAccordingToHpLevel(HpLevel level, double highIntervalSeconds, double mediumIntervalSeconds, double lowIntervalSeconds) const
{
	switch (level)
	{
	case HpLevel::Dead:
		return 0.0;
	case HpLevel::Low:
		return lowIntervalSeconds;
	case HpLevel::Medium:
		return mediumIntervalSeconds;
	case HpLevel::High:
		return highIntervalSeconds;
	}
	throw std::logic_error("unknown HP level");
}

void ScanController::updateScanTime(double elapsedSeconds) {
	if (scanCooldownRemainingSeconds_ > 0) {
		scanCooldownRemainingSeconds_ -= elapsedSeconds;
	}
};

void ScanController::scan(const Player& player, Map& map,double scanIntervalSeconds) {
	const Position playerPosition = player.getPosition();
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
	setScanCooldownRemainingSeconds(scanIntervalSeconds);
	
}

bool ScanController::		 (Position playerPosition, Position targetPosition, const Map& map) {
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




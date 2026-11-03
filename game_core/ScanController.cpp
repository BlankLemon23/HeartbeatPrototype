#include "ScanController.hpp"
#include "Enemy.hpp"
#include "Player.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

ScanController::ScanController(double initialScanCooldownSeconds, double scanRadiusCells, double enemySnapshotDurationSeconds) : scanCooldownRemainingSeconds_(initialScanCooldownSeconds), scanRadiusCells_(scanRadiusCells), enemySnapshotDurationSeconds_(enemySnapshotDurationSeconds) {
	if (!std::isfinite(enemySnapshotDurationSeconds_) || enemySnapshotDurationSeconds_ <= 0.0) {
		throw std::invalid_argument("enemy snapshot duration must be positive and finite");
	}
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

bool ScanController::tryScanManually(const Player& player, Map& map, double scanIntervalSeconds, const std::vector<Enemy>& enemies) {
	if (!canScanManually(player)) {
		return false;
	}

	scan(player, map, scanIntervalSeconds, enemies);
	return true;
}

// 每帧调用一次，驱动「战斗中的自动扫描」。判断该帧是否需要扫描，用哪种扫描。
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
void ScanController::updateBattleAutoScan(Player& player, Map& map, const BattleScanConfig& config, const std::vector<Enemy>& enemies) {
	const HpLevel currentLevel = player.getHpLevel();
	if (player.isInBattle() && currentLevel != HpLevel::Dead) {

		const double automaticIntervalSeconds = getAutoScanTimeAccordingToHpLevel(
			currentLevel, config);

		const bool enteredBattle = !player.wasInBattle();

		const bool droppedToLowerLevel = previousHpLevel_.has_value() && currentLevel < *previousHpLevel_;
		//previousHpLevel_.has_value() 返回 bool，表示「这个 optional 里当前是否存着一个值」。
		//•	previousHpLevel_ 是 optional，不能直接比较，要先解引用取出里面的值。
		//•* previousHpLevel_ 就是「上一帧的血量等级」。
	    //•	currentLevel < *previousHpLevel_ 表示「当前血量等级比上一帧更低」。
		//枚举类从上到下的顺序是 Dead < Low < Medium < High，所以可以直接用 < 比较。也就是等

		if (enteredBattle || droppedToLowerLevel) {
			scan(player, map, automaticIntervalSeconds, enemies);
		}
		else {
			tryScanAutomatically(player, map, automaticIntervalSeconds, enemies);
		}
	}
	player.setWasInBattle(player.isInBattle());
	previousHpLevel_ = currentLevel;
}

bool ScanController::tryScanAutomatically(const Player& player, Map& map, double automaticIntervalSeconds, const std::vector<Enemy>& enemies) {
	if (!canScanAutomatically(player)) {
		return false;
	}
	scan(player, map, automaticIntervalSeconds, enemies);
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

double ScanController::getAutoScanTimeAccordingToHpLevel(HpLevel level, const BattleScanConfig& config) const
{
	switch (level)
	{
	case HpLevel::Dead:
		return 0.0;
	case HpLevel::Low:
		return config.lowHealthIntervalSeconds;
	case HpLevel::Medium:
		return config.mediumHealthIntervalSeconds;
	case HpLevel::High:
		return config.highHealthIntervalSeconds;
	}
	throw std::logic_error("unknown HP level");
}

void ScanController::updateScanTime(double elapsedSeconds) {
	if (scanCooldownRemainingSeconds_ > 0) {
		scanCooldownRemainingSeconds_ -= elapsedSeconds;
	}
};

void ScanController::updateSnapshotRemainingSeconds(double elapsedSeconds) {
	if (enemySnapshotRemainingSeconds_ > 0.0) {
		enemySnapshotRemainingSeconds_ = std::max(0.0, enemySnapshotRemainingSeconds_ - elapsedSeconds);
		if (enemySnapshotRemainingSeconds_ == 0.0) {
			enemySnapshotPositions_.clear();
		}
	}
};

const std::vector<Position>& ScanController::getEnemySnapshotPositions() const {
	return enemySnapshotPositions_;
}

void ScanController::scan(const Player& player, Map& map, double scanIntervalSeconds, const std::vector<Enemy>& enemies) {
	const Position playerPosition = player.getPosition();
	const double scanRadiusSquared = scanRadiusCells_ * scanRadiusCells_;
	

	//lamda表达式 &表示可以捕获外部参数 比如playePosition
	const auto isInsideScanAndVisible = [&](Position target) {
		const double distanceSquared = calculateSquaredDistanceToTarget(playerPosition, target);
		return distanceSquared <= scanRadiusSquared && map.hasLineOfSight(playerPosition, target);
	};

	const auto& cells = map.getMap();
	for (int i = 0;i < cells.size();i++) {
		for (int j = 0;j < cells[i].size();j++) {
			if (isInsideScanAndVisible(Position{ i, j })) {
				map.markScanned(i, j);
			}
		}
	}

	enemySnapshotPositions_.clear();
	for (const Enemy& enemy : enemies) {
		const Position enemyPosition = enemy.getPosition();
		if (isInsideScanAndVisible(enemyPosition)) {
			enemySnapshotPositions_.push_back(enemyPosition);
		}
	}
	enemySnapshotRemainingSeconds_ = enemySnapshotDurationSeconds_;
	setScanCooldownRemainingSeconds(scanIntervalSeconds);
	
}





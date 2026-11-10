#include "ScanController.hpp"
#include "Actors/Enemy.hpp"
#include "Actors/Player.hpp"
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

void ScanController::updateBattleAutoScan(Player& player, Map& map, const BattleScanConfig& config, const std::vector<Enemy>& enemies) {
	const HpLevel currentLevel = player.getHpLevel();
	if (player.isInBattle() && currentLevel != HpLevel::Dead) {
		const double automaticIntervalSeconds = getAutoScanTimeAccordingToHpLevel(
			currentLevel, config);

		const bool enteredBattle = !player.wasInBattle();

		const bool droppedToLowerLevel = previousHpLevel_.has_value() && currentLevel < *previousHpLevel_;

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

bool ScanController::canScanAutomatically(const Player& player) const {
	if (!player.isInBattle() || player.getHealth() == 0) {
		return false;
	}
	else if (scanCooldownRemainingSeconds_ > 0) {
		return false;
	}

	return true;
}

double ScanController::getAutoScanTimeAccordingToHpLevel(HpLevel level, const BattleScanConfig& config) const {
	switch (level) {
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

	const auto isInsideScanAndVisible = [&](Position target) {
		const double distanceSquared = calculateSquaredDistanceToTarget(playerPosition, target);
		return distanceSquared <= scanRadiusSquared && map.hasLineOfSight(playerPosition, target);
	};

	const auto& cells = map.getMap();
	for (int i = 0; i < cells.size(); i++) {
		for (int j = 0; j < cells[i].size(); j++) {
			if (isInsideScanAndVisible(Position{i, j})) {
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

#pragma once
#include "Map.hpp"
#include "Player.hpp"
#include "Position.hpp"
#include <optional>
#include <vector>

class Enemy;

// Positive intervals in seconds; supplied by the caller as one configuration.
struct BattleScanConfig {
    double highHealthIntervalSeconds = 3.0;
    double mediumHealthIntervalSeconds = 2.0;
    double lowHealthIntervalSeconds = 1.2;
};

class ScanController {
public:

	void updateScanTime(double elapsedSeconds);
	void updateSnapshotRemainingSeconds(double elapsedSeconds);
	const std::vector<Position>& getEnemySnapshotPositions() const;

	void updateBattleAutoScan(Player& player, Map& map, const BattleScanConfig& config, const std::vector<Enemy>& enemies);
	ScanController(double initialScanCooldownSeconds, double scanRadiusCells, double enemySnapshotDurationSeconds);
	bool canScanManually(const Player& player) const;
	bool tryScanManually(const Player& player, Map& map, double scanIntervalSeconds, const std::vector<Enemy>& enemies);

private:
	bool tryScanAutomatically(const Player& player, Map& map, double automaticIntervalSeconds, const std::vector<Enemy>& enemies);
	bool canScanAutomatically(const Player& player) const;
	double getAutoScanTimeAccordingToHpLevel(HpLevel level, const BattleScanConfig& config) const;
	void setScanCooldownRemainingSeconds(double remainingSeconds);
	void scan(const Player& player, Map& map, double scanIntervalSeconds, const std::vector<Enemy>& enemies);

	double scanCooldownRemainingSeconds_;
	double scanRadiusCells_;

	std::vector<Position> enemySnapshotPositions_;
	double enemySnapshotRemainingSeconds_ = 0.0;
	double enemySnapshotDurationSeconds_;
	std::optional<HpLevel> previousHpLevel_;// std::optional 用于表示可能没有值的情况，previousHpLevel_ 记录上一帧的血量等级 第一帧可能没有上一挡位
};

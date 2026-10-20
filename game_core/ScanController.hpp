#pragma once
#include "Map.hpp"
#include "Player.hpp"
#include "Position.hpp"
#include <optional>

// Positive intervals in seconds; supplied by the caller as one configuration.
struct BattleScanConfig {
    double highHealthIntervalSeconds;
    double mediumHealthIntervalSeconds;
    double lowHealthIntervalSeconds;
};

class ScanController {
public:
	void updateScanTime(double elapsedSeconds);
	void updateBattleAutoScan(Player& player, Map& map, const BattleScanConfig& config);
	ScanController(double initialScanCooldownSeconds, double scanRadiusCells);
	bool canScanManually(const Player& player) const;
	bool tryScanManually(const Player& player, Map& map, double scanIntervalSeconds);
	bool ifPlayerCanSeeTheTarget(Position playerPosition, Position targetPosition, const Map& map) const;
private:
	bool tryScanAutomatically(const Player& player, Map& map, double automaticIntervalSeconds);
	bool canScanAutomatically(const Player& player) const;
	double getAutoScanTimeAccordingToHpLevel(HpLevel level, const BattleScanConfig& config) const;
	void setScanCooldownRemainingSeconds(double remainingSeconds);
	void scan(const Player& player, Map& map, double scanIntervalSeconds);
	double scanCooldownRemainingSeconds_;
	double scanRadiusCells_;
	std::optional<HpLevel> previousHpLevel_;// std::optional 用于表示可能没有值的情况，previousHpLevel_ 记录上一帧的血量等级 第一帧可能没有上一挡位
};

#pragma once
#include "Map.hpp"
#include "Player.hpp"
#include <optional>
class ScanController{
public:
	void updateScanTime(double elapsedSeconds);
	void updateBattleScan(Player& player, Map& map, double highIntervalSeconds, double mediumIntervalSeconds, double lowIntervalSeconds);
	ScanController(double initialScanCooldownSeconds,double ScanRadius);
	bool canScanManually(const Player& player) const;
	bool tryScanManually(const Player& player, Map& map, double scanIntervalSeconds);
	bool ifPlayerCanSeeTheTarget(Position playerPosition, Position targetPosition, const Map& map);
private:
	bool tryScanAutomatically(const Player& player, Map& map, double automaticIntervalSeconds);
	bool canScanAutomatically(const Player& player) const;
	double getAutoScanTimeAccordingToHpLevel(HpLevel level, double highIntervalSeconds, double mediumIntervalSeconds, double lowIntervalSeconds) const;
	void setScanCooldownRemainingSeconds(double remainingSeconds);
	void scan(const Player& player, Map& map, double scanIntervalSeconds);
	double scanCooldownRemainingSeconds_;
	double scanRadius_;
	std::optional<HpLevel> previousHpLevel_;// std::optional 用于表示可能没有值的情况，previousHpLevel_ 记录上一帧的血量等级 第一帧可能没有上一挡位
};

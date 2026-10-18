#pragma once
#include "Map.hpp"
#include "Player.hpp"
class ScanController{
public:
	void updateScanTime(double elapsedSeconds);
	ScanController(bool isInBattle,double scanCooldownTime,double ScanRadius);  
	bool canScanManually() const;
	void setScanCooldownTime(double scanCooldownTime);
	void scan(Position playerPosition, Map& map,double scanCooldownTime);
	bool ifPlayerCanSeeTheTarget(Position playerPosition, Position targetPosition, const Map& map);
private:
	bool isInBattle_;
	double scanCooldownTime_;
	double scanRadius_;
};

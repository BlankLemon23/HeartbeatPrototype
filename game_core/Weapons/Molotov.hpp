#pragma once
#include "Position.hpp"
#include "Map.hpp"
#include <vector>
#include "Player.hpp"
#include "Enemy.hpp"

struct MolotovConfig {
	int throwRadiusCells = 8;
	int burnRadiusCells = 4;
	double burnDurationSeconds = 4;
	double damageIntervalSeconds = 1;
	int burnDamageEnemy = 30;
	int burnDamagePlayer = 10;
	int carryLimit = 5;
};




class Molotov {
public:
	Molotov(MolotovConfig config);
	bool tryThrow(Position playerPosition, Position clickedPosition, const Map& map);
	void updateBurningArea(double elapsedSeconds, Player& player, std::vector<Enemy>& enemies,const Map& map);
	std::vector<Position> getBurningCells() const;
	std::vector<Position> findValidThrowCells(Position playerPosition, const Map& map) const;
private:
	struct BurningArea {
		Position burningCenter;
		double remainingSeconds;
		double secondsUntilNextDamage;
		std::vector<Position> burningCells;
	};

	std::vector<Position> collectBurningCells(Position center,const Map& map) const;
	int calculateSquaredDistanceToTarget(Position startPosition,Position targetPosition) const;
	void createBurningArea(Position landingPosition, const Map& map);
	Position findLastFreeCell(const Map& map, Position start, Position target);
	void burnPlayer(Player& player,const BurningArea& area,const Map& map);
	void burnEnemy(std::vector<Enemy>& enemies,const BurningArea& area,const Map& map);
	bool ifInstanceInsideBurningArea(Position instancePosition,Position burningCenter);

	MolotovConfig config_;
	int remainingCount_;
	std::vector<BurningArea> burningAreas_;

};

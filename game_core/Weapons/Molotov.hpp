#pragma once
#include "World/Position.hpp"
#include "World/Map.hpp"
#include <vector>
#include "Actors/Player.hpp"
#include "Actors/Enemy.hpp"

struct MolotovConfig {
	int throwRadiusCells = 8;
	int burnRadiusCells = 4;
	double burnDurationSeconds = 4;
	double damageIntervalSeconds = 1;
	int burnDamageEnemy = 30;
	int burnDamagePlayer = 10;
	int carryLimit = 5;
	int initialCount = 2;
};




class Molotov {
public:
	explicit Molotov(MolotovConfig config);
	int getRemainingCount() const;
	bool tryThrow(Position playerPosition, Position clickedPosition, const Map& map);
	void updateBurningArea(double elapsedSeconds, Player& player, std::vector<Enemy>& enemies,const Map& map);
	std::vector<Position> getBurningCells() const;
	std::vector<Position> findValidThrowCells(Position playerPosition, const Map& map) const;
	void addMolotov(int amount);
	int getCarryLimit() const;
private:
	struct BurningArea {
		Position burningCenter;
		double remainingSeconds;
		double secondsUntilNextDamage;
		std::vector<Position> burningCells;
	};

	std::vector<Position> collectBurningCells(Position center,const Map& map) const;
	void createBurningArea(Position landingPosition, const Map& map);
	void burnPlayer(Player& player,const BurningArea& area,const Map& map);
	void burnEnemy(std::vector<Enemy>& enemies,const BurningArea& area,const Map& map);
	bool ifInstanceInsideBurningArea(Position instancePosition,Position burningCenter);

	MolotovConfig config_;
	int remainingCount_;
	std::vector<BurningArea> burningAreas_;

};

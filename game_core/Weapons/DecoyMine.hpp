#pragma once
#include <vector>
#include "Position.hpp"
#include "Map.hpp"
#include "Enemy.hpp"
struct DecoyMineConfig {
	int throwRadiusCells = 12;
	int soundRadiusCells = 8;
	int carryLimit = 5;
};

class DecoyMine {

public:
	explicit DecoyMine(DecoyMineConfig config);
	bool tryThrow(Position playerPosition, Position clickedPosition, const Map& map, std::vector<Enemy>& enemies);
	std::vector<Position> findValidThrowCells(Position playerPosition, const Map& map) const;
	int getRemainingCount() const;
private:
	DecoyMineConfig config_;
	int remainingCount_;
};

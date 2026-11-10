#include "DecoyMine.hpp"
#include "Throwing.hpp"
#include "Perception/Sound.hpp"
#include <algorithm>

DecoyMine::DecoyMine(DecoyMineConfig config)
	: config_(config),
	  remainingCount_(std::clamp(config.initialCount, 0, std::max(0, config.carryLimit))) {
}

bool DecoyMine::tryThrow(Position playerPosition, Position clickedPosition, const Map& map, std::vector<Enemy>& enemies) {
	if (remainingCount_ <= 0 ||
		!isValidThrowTarget(playerPosition, clickedPosition, config_.throwRadiusCells, map)) {
		return false;
	}

	const Position landingPosition = map.findLastFreeCell(playerPosition, clickedPosition);
	makeSound(landingPosition, config_.soundRadiusCells, enemies);
	remainingCount_ -= 1;
	return true;
}

std::vector<Position> DecoyMine::findValidThrowCells(Position playerPosition, const Map& map) const {
	return collectValidThrowCells(playerPosition, config_.throwRadiusCells, map);
}

int DecoyMine::getRemainingCount() const {
	return remainingCount_;
}

void DecoyMine::addDecoyMine(int amount) {
	remainingCount_ += amount;
}

int DecoyMine::getCarryLimit() const {
	return config_.carryLimit;
}

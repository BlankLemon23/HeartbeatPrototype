#include "Molotov.hpp"
#include "Molotov.hpp"
#include "Throwing.hpp"
#include <stdexcept>
#include <algorithm>
Molotov::Molotov(MolotovConfig config) {
	config_ = config;
	remainingCount_ = config_.carryLimit;
	if (config_.damageIntervalSeconds <= 0) {
		throw std::invalid_argument("damageIntervalSeconds must be positive");
	}
}

int Molotov::getRemainingCount() const
{
	return remainingCount_;
}

bool Molotov::tryThrow(Position playerPosition, Position clickedPosition, const Map& map) {
	if (remainingCount_ <= 0 ||
		!isValidThrowTarget(playerPosition, clickedPosition, config_.throwRadiusCells, map)) {
		return false;
	}

	const Position landingPosition = map.findLastFreeCell(playerPosition, clickedPosition);
	createBurningArea(landingPosition, map);
	remainingCount_ -= 1;
	return true;
}
void Molotov::updateBurningArea(double elapsedSeconds,Player& player, std::vector<Enemy>& enemies,const Map& map) {
	for (BurningArea& area : burningAreas_) {

		//activeSeconds 这一帧实际燃烧时间
		const double activeSeconds = std::min(elapsedSeconds, area.remainingSeconds);
		area.remainingSeconds -= activeSeconds;
		area.secondsUntilNextDamage -= activeSeconds;
		//使用while的原因 一帧可能跨越多个伤害周期

		const double timeToleranceSeconds = 1e-9;
		while (area.secondsUntilNextDamage <= timeToleranceSeconds) {
			burnEnemy(enemies, area, map);
			burnPlayer(player, area, map);
			area.secondsUntilNextDamage += config_.damageIntervalSeconds;
		}
	}
	const auto newEnd = std::remove_if(
		burningAreas_.begin(), burningAreas_.end(),
		[](const BurningArea& area) { return area.remainingSeconds <= 0; }
	);
	burningAreas_.erase(
		newEnd,
		burningAreas_.end()
	);
}




std::vector<Position> Molotov::getBurningCells() const
{
	std::vector<Position> cells;
	for (const BurningArea& area : burningAreas_) {
		cells.insert(cells.end(), area.burningCells.begin(), area.burningCells.end());
	}
	return cells;
}




void Molotov::createBurningArea(Position landingPosition, const Map& map) {
	burningAreas_.push_back(BurningArea{
		landingPosition,
		config_.burnDurationSeconds,
		config_.damageIntervalSeconds,
		collectBurningCells(landingPosition,map)
		});

}

std::vector<Position> Molotov::collectBurningCells(Position center, const Map& map) const
{
	std::vector<Position> cells;
	const int radius = config_.burnRadiusCells;
	for (int rowOffset = -radius; rowOffset <= radius; ++rowOffset) {
		for (int columnOffset = -radius; columnOffset <= radius; ++columnOffset) {
			if (rowOffset * rowOffset + columnOffset * columnOffset > radius * radius) {
				continue;
			}
			Position current = { center.row + rowOffset,center.column + columnOffset };
			if (map.hasLineOfSight(center,current)) {
				cells.push_back(current);
			}
		}
	}
	return cells;
}

void Molotov::burnPlayer(Player& player,const BurningArea& area, const Map& map)
{
	if (ifInstanceInsideBurningArea(player.getPosition(), area.burningCenter)
		&&
		map.hasLineOfSight(player.getPosition(),area.burningCenter)) {
		player.takeDamage(config_.burnDamagePlayer);
	}
}

void Molotov::burnEnemy(std::vector<Enemy>& enemies,const BurningArea& area,const Map& map)
{
	for (Enemy& enemy : enemies) {
		if (ifInstanceInsideBurningArea(enemy.getPosition(), area.burningCenter)
			&&
			map.hasLineOfSight(enemy.getPosition(),area.burningCenter)){
			enemy.takeDamage(config_.burnDamageEnemy);
		}
	}
}

bool Molotov::ifInstanceInsideBurningArea(Position instancePosition,Position burningCenter)
{
	int distance = calculateSquaredDistanceToTarget(instancePosition, burningCenter);
	if (distance <= config_.burnRadiusCells * config_.burnRadiusCells) {
		return true;
	}
	return false;
}


std::vector<Position> Molotov::findValidThrowCells(Position playerPosition, const Map& map) const
{
	return collectValidThrowCells(playerPosition, config_.throwRadiusCells, map);
}

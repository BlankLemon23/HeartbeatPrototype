#include "Molotov.hpp"
#include <cmath>
#include <limits>
#include <algorithm>
Molotov::Molotov(MolotovConfig config) {
	config_ = config;
	remainingCount_ = config_.carryLimit;
	if (config_.damageIntervalSeconds <= 0) {
		throw std::invalid_argument("damageIntervalSeconds must be positive");
	}
}

bool Molotov::tryThrow(Position playerPosition, Position clickedPosition, const Map& map) {
	int distanceSquar = calculateSquaredDistanceToTarget(playerPosition, clickedPosition);
	if (distanceSquar > config_.throwRadiusCells * config_.throwRadiusCells
		||
		remainingCount_ <= 0
		||
		map.classifyCell(clickedPosition.row,clickedPosition.column) == CellType::OutOfBounds
		||
		(map.classifyCell(clickedPosition.row, clickedPosition.column) == CellType::Wall)&&map.isScanned(clickedPosition.row, clickedPosition.column)
		||
		!map.hasLineOfSight(playerPosition, clickedPosition, SightMode::KnownTerrain)
		){
		return false;
	}

	createBurningArea(findLastFreeCell(map, playerPosition, clickedPosition),map);
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








int Molotov::calculateSquaredDistanceToTarget(Position startPosition,Position targetPosition) const
{
	int rowOffset = targetPosition.row - startPosition.row;
	int columnOffset = targetPosition.column - startPosition.column;
	return rowOffset * rowOffset + columnOffset * columnOffset;

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

Position Molotov::findLastFreeCell(const Map& map, Position start, Position target)
{
	if (start == target) {
		return start;
	}
	const double startCenterRow = start.row + 0.5;
	const double startCenterColumn = start.column + 0.5;
	const double targetCenterRow = target.row + 0.5;
	const double targetCenterColumn = target.column + 0.5;
	const double rowDistance = std::abs(targetCenterRow - startCenterRow);
	const double columnDistance = std::abs(targetCenterColumn - startCenterColumn);
	const double neverCross = std::numeric_limits<double>::infinity();
	// 这里比较的是射线到达下一条行边界和列边界的进度。
// 数学上两个进度相等时，射线恰好穿过格角；但 double 的计算结果可能有极小误差。
// 因此允许两者相差一点点，仍按穿过格角处理。
	const double cornerTolerance = 1e-12;

	int rowStep = 0;
	if (target.row > start.row) {
		rowStep = 1;
	}
	else if (target.row < start.row) {
		rowStep = -1;
	}
	int columnStep = 0;
	if (target.column > start.column) {
		columnStep = 1;
	}
	else if (target.column < start.column) {
		columnStep = -1;
	}//这样写避免了每次需要通过三元运算进行判断

	double nextRowProgress = neverCross;
	double rowProgressStep = neverCross;
	if (rowDistance > 0.0) {
		nextRowProgress = 0.5 / rowDistance;
		rowProgressStep = 1.0 / rowDistance;
	}
	double nextColumnProgress = neverCross;
	double columnProgressStep = neverCross;
	if (columnDistance > 0.0) {
		nextColumnProgress = 0.5 / columnDistance;
		columnProgressStep = 1.0 / columnDistance;
	}
	// current 始终是最后一个确认可落地的空地；next 只是下一步的候选格。
	Position current = start;
	while (!(current == target)) {
		Position next = current;
		if (std::abs(nextRowProgress - nextColumnProgress) <= cornerTolerance) {
			// A wall touching the ray at a grid corner blocks cells behind it.
			const CellType rowNeighbor = map.classifyCell(current.row + rowStep, current.column);
			const CellType columnNeighbor = map.classifyCell(current.row, current.column + columnStep);
			if (rowNeighbor == CellType::Wall || rowNeighbor == CellType::OutOfBounds
				|| columnNeighbor == CellType::Wall || columnNeighbor == CellType::OutOfBounds) {
				return current;
			}
			next.row += rowStep;
			next.column += columnStep;
			nextRowProgress += rowProgressStep;
			nextColumnProgress += columnProgressStep;
		}
		else if (nextColumnProgress < nextRowProgress) {
			next.column += columnStep;
			nextColumnProgress += columnProgressStep;
		}
		else {
			next.row += rowStep;
			nextRowProgress += rowProgressStep;
		}

		const CellType nextCell = map.classifyCell(next.row, next.column);
		if (nextCell == CellType::Wall || nextCell == CellType::OutOfBounds) {
			return current;
		}
		current = next;
	}
	return current;

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
	std::vector<Position> cells;
	const int radius = config_.throwRadiusCells;
	for (int rowOffset = -radius; rowOffset <= radius; ++rowOffset) {
		for (int columnOffset = -radius; columnOffset <= radius; ++columnOffset) {
			if (rowOffset * rowOffset + columnOffset * columnOffset > radius * radius) {
				continue;
			}
			const Position current{playerPosition.row + rowOffset, playerPosition.column + columnOffset};
			const CellType cellType = map.classifyCell(current.row, current.column);
			if (cellType == CellType::OutOfBounds ||
				(cellType == CellType::Wall && map.isScanned(current.row, current.column))) {
				continue;
			}
			if (map.hasLineOfSight(playerPosition, current, SightMode::KnownTerrain)) {
				cells.push_back(current);
			}
		}
	}
	return cells;
}

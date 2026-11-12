#pragma once

#include <string>
#include <vector>
#include "Position.hpp"

class Enemy;

enum class CellType {
	OutOfBounds,
	Wall,
	Space,
	Unknown
};

enum class SightMode {
	RealTerrain,
	KnownTerrain
};

struct MapConfig {
	std::vector<std::string> rows;
};

class Map {
public:
	explicit Map(MapConfig config);
	const std::vector<std::string>& getMap() const;
	int rowCount() const;
	int maxColumnCount() const;
	CellType classifyCell(int row, int column) const;

	bool isScanned(int row, int column) const;
	bool markScanned(int row, int column);

	bool hasLineOfSight(Position start, Position target,
		SightMode mode = SightMode::RealTerrain) const;

	// Start must be a free cell; real walls and map gaps stop the landing path.
	Position findLastFreeCell(Position start, Position target) const;
	Position findFirstEnemyPositionFromPlayerToTarget(Position playerStart, GridPoint target, const std::vector<Enemy>& enemies) const;

private:
	struct LineTraceResult {
		Position lastFreeCell;
		bool targetVisible;
		Position firstEnemyPosition{-1, -1};
	};

	bool blocksLineOfSight(Position cell, SightMode mode) const;
	LineTraceResult traceLine(Position startCell, GridPoint targetPoint, SightMode mode,
		const std::vector<Enemy>& enemies) const;

	// Terrain data and the discovery state have different lifetimes.
	std::vector<std::string> map_;
	std::vector<std::vector<bool>> scannedCells_;
};

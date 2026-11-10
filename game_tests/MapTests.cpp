#include "World/Map.hpp"
#include <cassert>
#include "Actors/Player.hpp"
#include "Perception/ScanController.hpp"
#include "Actors/Enemy.hpp"
#include "Weapons/Molotov.hpp"

int main() {
	const PlayerConfig playerConfig{100, 0.5, 1.0, 2.0, 20, 60};
	const std::vector<Enemy> noEnemies;
	Player healthPlayer({0, 0}, playerConfig);
	// The medium tier used to be unreachable, and zero health was classified as high.
	healthPlayer.setHealth(60);
	assert(healthPlayer.getHpLevel() == HpLevel::Medium);
	healthPlayer.setHealth(0);
	assert(healthPlayer.getHpLevel() == HpLevel::Dead);

	// Reject indices equal to the map dimensions before indexing a row or column.
	Map map(MapConfig{{"****", "*..*", "****"}});
	assert(map.classifyCell(1, 1) == CellType::Space);
	assert(map.classifyCell(0, 0) == CellType::Wall);
	assert(map.classifyCell(3, 0) == CellType::OutOfBounds);
	assert(map.classifyCell(0, 4) == CellType::OutOfBounds);

	// Switching modes keeps the current cooldown; the next move uses the new interval.
	{
		Map movementMap(MapConfig{{"....."}});
		Player movingPlayer({0, 1}, playerConfig);
		assert(movingPlayer.tryMove(Direction::RIGHT, movementMap));
		movingPlayer.toggleStealth();
		assert(!movingPlayer.tryMove(Direction::RIGHT, movementMap));
		movingPlayer.updateMoveCooldown(0.5);
		assert(movingPlayer.tryMove(Direction::RIGHT, movementMap));
		movingPlayer.toggleStealth();
		movingPlayer.updateMoveCooldown(0.5);
		assert(!movingPlayer.tryMove(Direction::RIGHT, movementMap));
		movingPlayer.updateMoveCooldown(0.5);
		assert(movingPlayer.tryMove(Direction::RIGHT, movementMap));
		assert((movingPlayer.getPosition() == Position{0, 4}));
	}

	// Failed moves must not delay a valid move in another direction.
	{
		Map obstacleMap(MapConfig{{".*.", "..."}});
		Player blockedPlayer({0, 0}, playerConfig);
		const bool hitWall = blockedPlayer.tryMove(Direction::RIGHT, obstacleMap);
		assert(!hitWall);
		assert((blockedPlayer.getPosition() == Position{0, 0}));
		const bool leftMap = blockedPlayer.tryMove(Direction::UP, obstacleMap);
		assert(!leftMap);
		assert((blockedPlayer.getPosition() == Position{0, 0}));
		const bool moveAfterBlockedAttempts = blockedPlayer.tryMove(Direction::DOWN, obstacleMap);
		assert(moveAfterBlockedAttempts);
		assert((blockedPlayer.getPosition() == Position{1, 0}));
	}

	Map scanMap(MapConfig{{".*."}});
	ScanController scanController(0.0, 3, 0.5);
	Player player({0, 0}, playerConfig);
	assert(scanController.canScanManually(player));
	const bool firstManualScan = scanController.tryScanManually(player, scanMap, 5.0, noEnemies);
	assert(firstManualScan);
	assert(scanMap.isScanned(0, 1) == true);
	assert(scanMap.isScanned(0, 2) == false);

	// Manual scanning remains blocked until the cooldown expires.
	assert(!scanController.canScanManually(player));
	Map blockedMap(MapConfig{{"."}});
	const bool blockedByCooldown = scanController.tryScanManually(player, blockedMap, 2.0, noEnemies);
	assert(!blockedByCooldown);
	assert(!blockedMap.isScanned(0, 0));
	scanController.updateScanTime(4.0);
	assert(!scanController.canScanManually(player));
	scanController.updateScanTime(1.0);
	assert(scanController.canScanManually(player));

	// Preview ignores an unknown wall; real throws still land before it.
	Map previewMap(MapConfig{{".*..."}});
	MolotovConfig previewConfig{};
	previewConfig.throwRadiusCells = 3;
	previewConfig.burnRadiusCells = 1;
	previewConfig.burnDurationSeconds = 4.0;
	previewConfig.damageIntervalSeconds = 1.0;
	previewConfig.burnDamageEnemy = 30;
	previewConfig.burnDamagePlayer = 10;
	previewConfig.carryLimit = 2;
	previewConfig.initialCount = 2;
	Molotov previewMolotov(previewConfig);
	assert((previewMolotov.findValidThrowCells({0, 0}, previewMap) ==
		std::vector<Position>{{0, 0}, {0, 1}, {0, 2}, {0, 3}}));
	assert(previewMolotov.tryThrow({0, 0}, {0, 3}, previewMap));
	assert((previewMolotov.getBurningCells() == std::vector<Position>{{0, 0}, {0, 1}}));
	previewMap.markScanned(0, 1);
	assert((previewMolotov.findValidThrowCells({0, 0}, previewMap) ==
		std::vector<Position>{{0, 0}}));
	assert(!previewMolotov.tryThrow({0, 0}, {0, 3}, previewMap));
	assert(previewMolotov.tryThrow({0, 0}, {0, 0}, previewMap)); // Rejected throws kept the last bottle.

	return 0;
}

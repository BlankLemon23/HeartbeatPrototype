#include <string>
#include <vector>
#include "../game_core/Map.hpp"
#include <cassert>
#include <stdexcept>
#include "../game_core/Player.hpp"
#include "../game_core/ScanController.hpp"
#include "../game_core/Enemy.hpp"
int main() {
	const std::vector<Enemy> noEnemies;
	Player healthPlayer({0, 0});
	assert(healthPlayer.getHpLevel() == HpLevel::High);
	healthPlayer.setHealth(61);
	assert(healthPlayer.getHpLevel() == HpLevel::High);
	healthPlayer.setHealth(60);
	assert(healthPlayer.getHpLevel() == HpLevel::Medium);
	healthPlayer.setHealth(21);
	assert(healthPlayer.getHpLevel() == HpLevel::Medium);
	healthPlayer.setHealth(20);
	assert(healthPlayer.getHpLevel() == HpLevel::Low);
	healthPlayer.setHealth(1);
	assert(healthPlayer.getHpLevel() == HpLevel::Low);
	healthPlayer.setHealth(0);
	assert(healthPlayer.getHpLevel() == HpLevel::Dead);
	PlayerConfig higherHealthConfig{};
	higherHealthConfig.maxHealth = 200;
	Player higherMaxHealthPlayer({0, 0}, higherHealthConfig);
	higherMaxHealthPlayer.setHealth(121);
	assert(higherMaxHealthPlayer.getHpLevel() == HpLevel::High);
	higherMaxHealthPlayer.setHealth(120);
	assert(higherMaxHealthPlayer.getHpLevel() == HpLevel::Medium);
	higherMaxHealthPlayer.setHealth(41);
	assert(higherMaxHealthPlayer.getHpLevel() == HpLevel::Medium);
	higherMaxHealthPlayer.setHealth(40);
	assert(higherMaxHealthPlayer.getHpLevel() == HpLevel::Low);
	higherMaxHealthPlayer.setHealth(250);
	assert(higherMaxHealthPlayer.getHealth() == 200);
	higherMaxHealthPlayer.setHealth(-5);
	assert(higherMaxHealthPlayer.getHealth() == 0);
	bool rejectedInvalidMaxHealth = false;
	try {
		PlayerConfig invalidConfig{};
		invalidConfig.maxHealth = 0;
		Player invalidPlayer({0, 0}, invalidConfig);
	}
	catch (const std::invalid_argument&) {
		rejectedInvalidMaxHealth = true;
	}
	assert(rejectedInvalidMaxHealth);
	bool rejectedInvalidMoveInterval = false;
	try {
		PlayerConfig invalidConfig{};
		invalidConfig.normalMoveIntervalSeconds = 0.0;
		Player invalidPlayer({0, 0}, invalidConfig);
	}
	catch (const std::invalid_argument&) {
		rejectedInvalidMoveInterval = true;
	}
	assert(rejectedInvalidMoveInterval);

	// Losing all pursuers counts down; renewed pursuit starts a full delay.
	PlayerConfig battleExitConfig{};
	battleExitConfig.outBattleIntervalSeconds = 1.5;
	Player battleExitPlayer({0, 0}, battleExitConfig);
	battleExitPlayer.setInBattle(true);
	battleExitPlayer.updateOutBattleRemainingSeconds(1.0);
	assert(battleExitPlayer.getOutBattleRemainingSeconds() == 0.5);
	battleExitPlayer.resetOutBattleRemainingSeconds();
	battleExitPlayer.updateOutBattleRemainingSeconds(1.5);
	assert(battleExitPlayer.getOutBattleRemainingSeconds() == 0.0);

	Map map;
	map.setMap({ "****", "*..*", "****" });
	assert(!map.isScanned(1, 1));
	const bool marked = map.markScanned(1, 1);
	assert(marked);
	assert(map.isScanned(1, 1));
	assert(!map.isScanned(1, 2));
	assert(map.classifyCell(1, 1) == CellType::Space);
	assert(map.classifyCell(0, 0) == CellType::Wall);
	assert(map.classifyCell(-1, 0) == CellType::OutOfBounds);
	assert(map.classifyCell(3, 0) == CellType::OutOfBounds);
	assert(map.classifyCell(0, -1) == CellType::OutOfBounds);
	assert(map.classifyCell(0, 4) == CellType::OutOfBounds);
	map.setMap({ "?" });
	assert(!map.isScanned(0, 0));
	assert(map.classifyCell(0, 0) == CellType::Unknown);

	// Movement requests share one cooldown, including direction changes.
	{
		Map movementMap;
		movementMap.setMap({ "....." });
		PlayerConfig movementConfig{};
		movementConfig.normalMoveIntervalSeconds = 0.75;
		Player movingPlayer({0, 1}, movementConfig);

		const bool firstMove = movingPlayer.tryMove(Direction::RIGHT, movementMap);
		assert(firstMove);
		assert((movingPlayer.getPosition() == Position{0, 2}));

		const bool repeatedMove = movingPlayer.tryMove(Direction::RIGHT, movementMap);
		assert(!repeatedMove);
		const bool changedDirection = movingPlayer.tryMove(Direction::LEFT, movementMap);
		assert(!changedDirection);
		assert((movingPlayer.getPosition() == Position{0, 2}));

		movingPlayer.updateMoveCooldown(0.5);
		const bool earlyMove = movingPlayer.tryMove(Direction::RIGHT, movementMap);
		assert(!earlyMove);
		assert((movingPlayer.getPosition() == Position{0, 2}));

		movingPlayer.updateMoveCooldown(0.25);
		const bool moveAtDeadline = movingPlayer.tryMove(Direction::RIGHT, movementMap);
		assert(moveAtDeadline);
		assert((movingPlayer.getPosition() == Position{0, 3}));

		// Idle time expires the cooldown without storing extra moves.
		movingPlayer.updateMoveCooldown(10.0);
		assert(movingPlayer.getMoveCooldownRemainingSeconds() == 0.0);
		const bool moveAfterIdle = movingPlayer.tryMove(Direction::LEFT, movementMap);
		assert(moveAfterIdle);
		assert((movingPlayer.getPosition() == Position{0, 2}));
		const bool extraMoveAfterIdle = movingPlayer.tryMove(Direction::LEFT, movementMap);
		assert(!extraMoveAfterIdle);
		assert((movingPlayer.getPosition() == Position{0, 2}));

	}

	// Switching modes keeps the current cooldown; the next move uses the new interval.
	{
		Map movementMap;
		movementMap.setMap({ "....." });
		PlayerConfig config{};
		config.normalMoveIntervalSeconds = 0.5;
		config.stealthMoveIntervalSeconds = 1.0;
		Player movingPlayer({0, 1}, config);
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
		Map obstacleMap;
		obstacleMap.setMap({ ".*." , "..." });
		Player blockedPlayer({0, 0});
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

	map.setMap({ ".*." });
	ScanController scanController(0.0, 3, 0.5);
	Player player({0, 0});
	assert(scanController.canScanManually(player));
	const bool firstManualScan = scanController.tryScanManually(player, map, 5.0, noEnemies);
	assert(firstManualScan);
	assert(map.isScanned(0, 1) == true);
	assert(map.isScanned(0, 2) == false);

	// Manual scanning remains blocked until the cooldown expires.
	assert(!scanController.canScanManually(player));
	Map blockedMap;
	blockedMap.setMap({ "." });
	const bool blockedByCooldown = scanController.tryScanManually(player, blockedMap, 2.0, noEnemies);
	assert(!blockedByCooldown);
	assert(!blockedMap.isScanned(0, 0));
	scanController.updateScanTime(4.0);
	assert(!scanController.canScanManually(player));
	scanController.updateScanTime(1.0);
	assert(scanController.canScanManually(player));
	assert(!map.isScanned(0, 2));

	// Each call supplies its own interval, rather than always using five seconds.
	const bool secondManualScan = scanController.tryScanManually(player, map, 2.0, noEnemies);
	assert(secondManualScan);
	assert(!scanController.canScanManually(player));
	scanController.updateScanTime(1.0);
	assert(!scanController.canScanManually(player));
	scanController.updateScanTime(1.0);
	assert(scanController.canScanManually(player));

	// The controller reads the player's current state on each call.
	player.setInBattle(true);
	assert(!scanController.canScanManually(player));
	const bool blockedByBattle = scanController.tryScanManually(player, blockedMap, 2.0, noEnemies);
	assert(!blockedByBattle);
	assert(!blockedMap.isScanned(0, 0));
	const Player otherPlayer({0, 0});
	assert(scanController.canScanManually(otherPlayer));
	player.setInBattle(false);
	assert(scanController.canScanManually(player));

	// Leaving battle preserves the remaining shared cooldown.
	const bool cooldownBeforeBattleScan = scanController.tryScanManually(player, map, 2.0, noEnemies);
	assert(cooldownBeforeBattleScan);
	player.setInBattle(true);
	scanController.updateScanTime(1.0);
	player.setInBattle(false);
	assert(!scanController.canScanManually(player));
	scanController.updateScanTime(1.0);
	assert(scanController.canScanManually(player));

	// A wall touching the ray at a corner blocks it in both directions.
	map.setMap({ ".*", ".." });
	assert(!map.hasLineOfSight({0, 0}, {1, 1}));
	assert(!map.hasLineOfSight({1, 1}, {0, 0}));

	// Entering battle scans immediately, even with a manual cooldown remaining.
	Map battleMap;
	battleMap.setMap({ "........." });
	Player battlePlayer({0, 0});
	ScanController battleScanner(0.0, 1.0, 0.5);
	const BattleScanConfig equalIntervals{2.0, 2.0, 2.0};
	const bool manualBeforeBattle = battleScanner.tryScanManually(battlePlayer, battleMap, 5.0, noEnemies);
	assert(manualBeforeBattle);
	battleScanner.updateScanTime(2.0);
	battlePlayer.setPosition({0, 4});
	battlePlayer.setInBattle(true);
	battleScanner.updateBattleAutoScan(battlePlayer, battleMap, equalIntervals, noEnemies);
	assert(battleMap.isScanned(0, 4));

	// Staying in battle waits for the automatic interval before the next scan.
	battlePlayer.setPosition({0, 7});
	battleScanner.updateScanTime(1.0);
	battleScanner.updateBattleAutoScan(battlePlayer, battleMap, equalIntervals, noEnemies);
	assert(!battleMap.isScanned(0, 7));
	battleScanner.updateScanTime(1.0);
	battleScanner.updateBattleAutoScan(battlePlayer, battleMap, equalIntervals, noEnemies);
	assert(battleMap.isScanned(0, 7));

	// Exiting preserves cooldown; entering again triggers a new immediate scan.
	battlePlayer.setInBattle(false);
	battleScanner.updateBattleAutoScan(battlePlayer, battleMap, equalIntervals, noEnemies);
	assert(!battleScanner.canScanManually(battlePlayer));
	battleScanner.updateScanTime(1.0);
	assert(!battleScanner.canScanManually(battlePlayer));
	battlePlayer.setPosition({0, 2});
	battlePlayer.setInBattle(true);
	battleScanner.updateBattleAutoScan(battlePlayer, battleMap, equalIntervals, noEnemies);
	assert(battleMap.isScanned(0, 2));

	// A lower health band scans immediately and starts its own interval.
	Map healthScanMap;
	healthScanMap.setMap({std::string(16, '.')});
	Player scanningPlayer({0, 0});
	ScanController healthScanner(0.0, 0.0, 0.5);
	const BattleScanConfig healthIntervals{3.0, 2.0, 1.2};
	scanningPlayer.setInBattle(true);
	healthScanner.updateBattleAutoScan(scanningPlayer, healthScanMap, healthIntervals, noEnemies);
	assert(healthScanMap.isScanned(0, 0));
	scanningPlayer.setPosition({0, 2});
	healthScanner.updateScanTime(2.0);
	healthScanner.updateBattleAutoScan(scanningPlayer, healthScanMap, healthIntervals, noEnemies);
	assert(!healthScanMap.isScanned(0, 2));
	healthScanner.updateScanTime(1.0);
	healthScanner.updateBattleAutoScan(scanningPlayer, healthScanMap, healthIntervals, noEnemies);
	assert(healthScanMap.isScanned(0, 2));
	scanningPlayer.setPosition({0, 4});
	scanningPlayer.setHealth(60);
	healthScanner.updateBattleAutoScan(scanningPlayer, healthScanMap, healthIntervals, noEnemies);
	assert(healthScanMap.isScanned(0, 4));
	scanningPlayer.setPosition({0, 6});
	healthScanner.updateScanTime(1.0);
	healthScanner.updateBattleAutoScan(scanningPlayer, healthScanMap, healthIntervals, noEnemies);
	assert(!healthScanMap.isScanned(0, 6));
	healthScanner.updateScanTime(1.0);
	healthScanner.updateBattleAutoScan(scanningPlayer, healthScanMap, healthIntervals, noEnemies);
	assert(healthScanMap.isScanned(0, 6));
	scanningPlayer.setPosition({0, 8});
	scanningPlayer.setHealth(20);
	healthScanner.updateBattleAutoScan(scanningPlayer, healthScanMap, healthIntervals, noEnemies);
	assert(healthScanMap.isScanned(0, 8));
	scanningPlayer.setPosition({0, 10});
	scanningPlayer.setHealth(19);
	healthScanner.updateBattleAutoScan(scanningPlayer, healthScanMap, healthIntervals, noEnemies);
	assert(!healthScanMap.isScanned(0, 10));
	healthScanner.updateScanTime(1.0);
	healthScanner.updateBattleAutoScan(scanningPlayer, healthScanMap, healthIntervals, noEnemies);
	assert(!healthScanMap.isScanned(0, 10));
	healthScanner.updateScanTime(0.2);
	healthScanner.updateBattleAutoScan(scanningPlayer, healthScanMap, healthIntervals, noEnemies);
	assert(healthScanMap.isScanned(0, 10));
	scanningPlayer.setPosition({0, 12});
	scanningPlayer.setHealth(0);
	healthScanner.updateScanTime(10.0);
	healthScanner.updateBattleAutoScan(scanningPlayer, healthScanMap, healthIntervals, noEnemies);
	assert(!healthScanMap.isScanned(0, 12));
	scanningPlayer.setInBattle(false);
	assert(!healthScanner.tryScanManually(scanningPlayer, healthScanMap, 5.0, noEnemies));

	map.setMap(std::vector<std::string>(11, std::string(11, '.')));
	ScanController radiusScanner(0.0, 5.0, 0.5);
	const Player radiusPlayer({5, 5});
	const bool radiusManualScan = radiusScanner.tryScanManually(radiusPlayer, map, 5.0, noEnemies);
	assert(radiusManualScan);
	assert(map.isScanned(8, 9)); // Distance five: include the circular boundary.
	assert(!map.isScanned(9, 9)); // Outside the circle.

	return 0;
}

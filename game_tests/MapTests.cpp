#include <string>
#include <vector>
#include "../game_core/Map.hpp"
#include <cassert>
#include <stdexcept>
#include "../game_core/Player.hpp"
#include "../game_core/ScanController.hpp"
int main() {
	assert((Position{ 1, 2 } == Position{ 1, 2 }));
	assert(!(Position{ 1, 2 } == Position{ 2, 1 }));
	assert(!(Position{ 1, 2 } == Position{ 1, 3 }));
	assert(!(Position{ 1, 2 } == Position{ 2, 2 }));

	Player healthPlayer({0, 0});
	assert(healthPlayer.getHealth() == 100);
	assert(healthPlayer.getMaxHealth() == 100);
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
	Player higherMaxHealthPlayer({0, 0}, 200);
	assert(higherMaxHealthPlayer.getHealth() == 200);
	higherMaxHealthPlayer.setHealth(150);
	assert(higherMaxHealthPlayer.getHealth() == 150);
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
		Player invalidPlayer({0, 0}, 0);
	}
	catch (const std::invalid_argument&) {
		rejectedInvalidMaxHealth = true;
	}
	assert(rejectedInvalidMaxHealth);

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

	map.setMap({ "*.**", "*..*", "****" });
	Player player({ 0, 1 });
	assert(player.tryMove(Direction::RIGHT, map) == false);
	assert(player.getPosition() == (Position{ 0, 1 }));
	assert(player.tryMove(Direction::UP, map) == false);
	assert(player.getPosition() == (Position{ 0, 1 }));
	assert(player.tryMove(Direction::DOWN, map) == true);
	assert(player.getPosition() == (Position{ 1, 1 }));

	map.setMap({ ".*." });
	ScanController scanController(0.0, 3);
	player.setPosition({ 0, 0 });
	assert(scanController.canScanManually(player));
	const bool firstManualScan = scanController.tryScanManually(player, map, 5.0);
	assert(firstManualScan);
	assert(map.isScanned(0, 1) == true);
	assert(map.isScanned(0, 2) == false);

	// Manual scanning remains blocked until the cooldown expires.
	assert(!scanController.canScanManually(player));
	Map blockedMap;
	blockedMap.setMap({ "." });
	const bool blockedByCooldown = scanController.tryScanManually(player, blockedMap, 2.0);
	assert(!blockedByCooldown);
	assert(!blockedMap.isScanned(0, 0));
	scanController.updateScanTime(4.0);
	assert(!scanController.canScanManually(player));
	scanController.updateScanTime(1.0);
	assert(scanController.canScanManually(player));
	assert(!map.isScanned(0, 2));

	// Each call supplies its own interval, rather than always using five seconds.
	const bool secondManualScan = scanController.tryScanManually(player, map, 2.0);
	assert(secondManualScan);
	assert(!scanController.canScanManually(player));
	scanController.updateScanTime(1.0);
	assert(!scanController.canScanManually(player));
	scanController.updateScanTime(1.0);
	assert(scanController.canScanManually(player));

	// The controller reads the player's current state on each call.
	assert(!player.isInBattle());
	player.setInBattle(true);
	assert(player.isInBattle());
	assert(!scanController.canScanManually(player));
	const bool blockedByBattle = scanController.tryScanManually(player, blockedMap, 2.0);
	assert(!blockedByBattle);
	assert(!blockedMap.isScanned(0, 0));
	const Player otherPlayer({0, 0});
	assert(scanController.canScanManually(otherPlayer));
	player.setInBattle(false);
	assert(scanController.canScanManually(player));

	// Leaving battle preserves the remaining shared cooldown.
	const bool cooldownBeforeBattleScan = scanController.tryScanManually(player, map, 2.0);
	assert(cooldownBeforeBattleScan);
	player.setInBattle(true);
	scanController.updateScanTime(1.0);
	player.setInBattle(false);
	assert(!scanController.canScanManually(player));
	scanController.updateScanTime(1.0);
	assert(scanController.canScanManually(player));

	// A wall touching the ray at a corner blocks it in both directions.
	map.setMap({ ".*", ".." });
	assert(!scanController.ifPlayerCanSeeTheTarget({0, 0}, {1, 1}, map));
	assert(!scanController.ifPlayerCanSeeTheTarget({1, 1}, {0, 0}, map));

	// Entering battle scans immediately, even with a manual cooldown remaining.
	Map battleMap;
	battleMap.setMap({ "........." });
	Player battlePlayer({0, 0});
	ScanController battleScanner(0.0, 1.0);
	const bool manualBeforeBattle = battleScanner.tryScanManually(battlePlayer, battleMap, 5.0);
	assert(manualBeforeBattle);
	battleScanner.updateScanTime(2.0);
	battlePlayer.setPosition({0, 4});
	battlePlayer.setInBattle(true);
	battleScanner.updateBattleScan(battlePlayer, battleMap, 2.0, 2.0, 2.0);
	assert(battlePlayer.wasInBattle());
	assert(battleMap.isScanned(0, 4));

	// Staying in battle waits for the automatic interval before the next scan.
	battlePlayer.setPosition({0, 7});
	battleScanner.updateScanTime(1.0);
	battleScanner.updateBattleScan(battlePlayer, battleMap, 2.0, 2.0, 2.0);
	assert(!battleMap.isScanned(0, 7));
	battleScanner.updateScanTime(1.0);
	battleScanner.updateBattleScan(battlePlayer, battleMap, 2.0, 2.0, 2.0);
	assert(battleMap.isScanned(0, 7));

	// Exiting preserves cooldown; entering again triggers a new immediate scan.
	battlePlayer.setInBattle(false);
	battleScanner.updateBattleScan(battlePlayer, battleMap, 2.0, 2.0, 2.0);
	assert(!battlePlayer.wasInBattle());
	assert(!battleScanner.canScanManually(battlePlayer));
	battleScanner.updateScanTime(1.0);
	assert(!battleScanner.canScanManually(battlePlayer));
	battlePlayer.setPosition({0, 2});
	battlePlayer.setInBattle(true);
	battleScanner.updateBattleScan(battlePlayer, battleMap, 2.0, 2.0, 2.0);
	assert(battleMap.isScanned(0, 2));

	// A lower health band scans immediately and starts its own interval.
	Map healthScanMap;
	healthScanMap.setMap({std::string(16, '.')});
	Player scanningPlayer({0, 0});
	ScanController healthScanner(0.0, 0.0);
	scanningPlayer.setInBattle(true);
	healthScanner.updateBattleScan(scanningPlayer, healthScanMap, 3.0, 2.0, 1.2);
	assert(healthScanMap.isScanned(0, 0));
	scanningPlayer.setPosition({0, 2});
	healthScanner.updateScanTime(2.0);
	healthScanner.updateBattleScan(scanningPlayer, healthScanMap, 3.0, 2.0, 1.2);
	assert(!healthScanMap.isScanned(0, 2));
	healthScanner.updateScanTime(1.0);
	healthScanner.updateBattleScan(scanningPlayer, healthScanMap, 3.0, 2.0, 1.2);
	assert(healthScanMap.isScanned(0, 2));
	scanningPlayer.setPosition({0, 4});
	scanningPlayer.setHealth(60);
	healthScanner.updateBattleScan(scanningPlayer, healthScanMap, 3.0, 2.0, 1.2);
	assert(healthScanMap.isScanned(0, 4));
	scanningPlayer.setPosition({0, 6});
	healthScanner.updateScanTime(1.0);
	healthScanner.updateBattleScan(scanningPlayer, healthScanMap, 3.0, 2.0, 1.2);
	assert(!healthScanMap.isScanned(0, 6));
	healthScanner.updateScanTime(1.0);
	healthScanner.updateBattleScan(scanningPlayer, healthScanMap, 3.0, 2.0, 1.2);
	assert(healthScanMap.isScanned(0, 6));
	scanningPlayer.setPosition({0, 8});
	scanningPlayer.setHealth(20);
	healthScanner.updateBattleScan(scanningPlayer, healthScanMap, 3.0, 2.0, 1.2);
	assert(healthScanMap.isScanned(0, 8));
	scanningPlayer.setPosition({0, 10});
	scanningPlayer.setHealth(19);
	healthScanner.updateBattleScan(scanningPlayer, healthScanMap, 3.0, 2.0, 1.2);
	assert(!healthScanMap.isScanned(0, 10));
	healthScanner.updateScanTime(1.0);
	healthScanner.updateBattleScan(scanningPlayer, healthScanMap, 3.0, 2.0, 1.2);
	assert(!healthScanMap.isScanned(0, 10));
	healthScanner.updateScanTime(0.2);
	healthScanner.updateBattleScan(scanningPlayer, healthScanMap, 3.0, 2.0, 1.2);
	assert(healthScanMap.isScanned(0, 10));
	scanningPlayer.setPosition({0, 12});
	scanningPlayer.setHealth(0);
	healthScanner.updateScanTime(10.0);
	healthScanner.updateBattleScan(scanningPlayer, healthScanMap, 3.0, 2.0, 1.2);
	assert(!healthScanMap.isScanned(0, 12));
	scanningPlayer.setInBattle(false);
	assert(!healthScanner.tryScanManually(scanningPlayer, healthScanMap, 5.0));

	map.setMap(std::vector<std::string>(11, std::string(11, '.')));
	ScanController radiusScanner(0.0, 5.0);
	const Player radiusPlayer({5, 5});
	const bool radiusManualScan = radiusScanner.tryScanManually(radiusPlayer, map, 5.0);
	assert(radiusManualScan);
	assert(map.isScanned(8, 9)); // Distance five: include the circular boundary.
	assert(!map.isScanned(9, 9)); // Outside the circle.

	return 0;
}

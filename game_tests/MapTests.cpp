#include <string>
#include <vector>
#include "../game_core/Map.hpp"
#include <cassert>
#include "../game_core/Player.hpp"
#include "../game_core/ScanController.hpp"
int main() {
	assert((Position{ 1, 2 } == Position{ 1, 2 }));
	assert(!(Position{ 1, 2 } == Position{ 2, 1 }));
	assert(!(Position{ 1, 2 } == Position{ 1, 3 }));
	assert(!(Position{ 1, 2 } == Position{ 2, 2 }));

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
	ScanController scanController(false, 0.0, 3);
	player.setPosition({ 0, 0 });
	assert(scanController.canScanManually());
	scanController.scan(player.getPosition(), map, 5.0);
	assert(map.isScanned(0, 1) == true);
	assert(map.isScanned(0, 2) == false);

	// Manual scanning remains blocked until the cooldown expires.
	assert(!scanController.canScanManually());
	scanController.updateScanTime(4.0);
	assert(!scanController.canScanManually());
	scanController.updateScanTime(1.0);
	assert(scanController.canScanManually());
	assert(!map.isScanned(0, 2));

	// Each call supplies its own interval, rather than always using five seconds.
	scanController.scan(player.getPosition(), map, 2.0);
	assert(!scanController.canScanManually());
	scanController.updateScanTime(1.0);
	assert(!scanController.canScanManually());
	scanController.updateScanTime(1.0);
	assert(scanController.canScanManually());

	ScanController battleScanner(true, 0.0, 5.0);
	assert(!battleScanner.canScanManually());

	// A wall touching the ray at a corner blocks it in both directions.
	map.setMap({ ".*", ".." });
	assert(!scanController.ifPlayerCanSeeTheTarget({0, 0}, {1, 1}, map));
	assert(!scanController.ifPlayerCanSeeTheTarget({1, 1}, {0, 0}, map));

	map.setMap(std::vector<std::string>(11, std::string(11, '.')));
	ScanController radiusScanner(false, 0.0, 5.0);
	radiusScanner.scan({5, 5}, map, 5.0);
	assert(map.isScanned(8, 9)); // Distance five: include the circular boundary.
	assert(!map.isScanned(9, 9)); // Outside the circle.

	return 0;
}

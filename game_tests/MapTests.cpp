#include <string>
#include <vector>
#include "../game_core/Map.hpp"
#include <cassert>
#include "../game_core/Player.hpp"

int main() {
	assert((Position{ 1, 2 } == Position{ 1, 2 }));
	assert(!(Position{ 1, 2 } == Position{ 2, 1 }));
	assert(!(Position{ 1, 2 } == Position{ 1, 3 }));
	assert(!(Position{ 1, 2 } == Position{ 2, 2 }));

	const std::vector<std::string> movementMap0 = { "****", "*..*", "****" };
	assert(classifyCell(movementMap0, 1, 1) == CellType::Space);
	assert(classifyCell(movementMap0, 0, 0) == CellType::Wall);
	assert(classifyCell(movementMap0, -1, 0) == CellType::OutOfBounds);
	assert(classifyCell(movementMap0, 3, 0) == CellType::OutOfBounds);
	assert(classifyCell(movementMap0, 0, -1) == CellType::OutOfBounds);
	assert(classifyCell(movementMap0, 0, 4) == CellType::OutOfBounds);
	assert(classifyCell({ "?" }, 0, 0) == CellType::Unknown);

	const std::vector<std::string> movementMap1 = { "*.**", "*..*", "****" };
	Player player({ 0, 1 });
	assert(player.tryMove(Direction::RIGHT, movementMap1) == false);
	assert(player.getPosition() == (Position{ 0, 1 }));
	assert(player.tryMove(Direction::UP, movementMap1) == false);
	assert(player.getPosition() == (Position{ 0, 1 }));
	assert(player.tryMove(Direction::DOWN, movementMap1) == true);
	assert(player.getPosition() == (Position{ 1, 1 }));

	return 0;
}

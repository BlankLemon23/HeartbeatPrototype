#include "Throwing.hpp"

bool isValidThrowTarget(Position playerPosition, Position targetPosition,
                        int throwRadiusCells, const Map& map) {
    if (calculateSquaredDistanceToTarget(playerPosition, targetPosition) >
        throwRadiusCells * throwRadiusCells) {
        return false;
    }

    const CellType cellType = map.classifyCell(targetPosition.row, targetPosition.column);
    if (cellType == CellType::OutOfBounds ||
        (cellType == CellType::Wall && map.isScanned(targetPosition.row, targetPosition.column))) {
        return false;
    }
    return map.hasLineOfSight(playerPosition, targetPosition, SightMode::KnownTerrain);
}

std::vector<Position> collectValidThrowCells(Position playerPosition,
                                            int throwRadiusCells, const Map& map) {
    std::vector<Position> cells;
    for (int rowOffset = -throwRadiusCells; rowOffset <= throwRadiusCells; ++rowOffset) {
        for (int columnOffset = -throwRadiusCells; columnOffset <= throwRadiusCells; ++columnOffset) {
            const Position target{playerPosition.row + rowOffset, playerPosition.column + columnOffset};
            if (isValidThrowTarget(playerPosition, target, throwRadiusCells, map)) {
                cells.push_back(target);
            }
        }
    }
    return cells;
}

#pragma once

#include "World/Map.hpp"
#include "World/Position.hpp"
#include <vector>

// Target selection uses known terrain; actual landing uses the real map.
bool isValidThrowTarget(Position playerPosition, Position targetPosition,
                        int throwRadiusCells, const Map& map);

std::vector<Position> collectValidThrowCells(Position playerPosition,
                                            int throwRadiusCells, const Map& map);

#pragma once
#include <string>
#include <vector>
enum class CellType {
    OutOfBounds,
    Wall,
    Space,
    Unknown
};

CellType classifyCell(const std::vector<std::string>& map, int x, int y);

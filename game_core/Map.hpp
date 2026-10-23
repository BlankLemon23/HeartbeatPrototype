#pragma once

#include <string>
#include <vector>
#include "Position.hpp"
enum class CellType {
    OutOfBounds,
    Wall,
    Space,
    Unknown
};

class Map {
public:
    void setMap(const std::vector<std::string>& rows);
    const std::vector<std::string>& getMap() const;
    int rowCount() const;
    int maxColumnCount() const;
    CellType classifyCell(int row, int column) const;

    bool isScanned(int row, int column) const;
    bool markScanned(int row, int column);

    bool hasLineOfSight(Position start, Position target) const;

private:
    // Terrain data and the discovery state have different lifetimes.
    std::vector<std::string> map_;
    std::vector<std::vector<bool>> scannedCells_;
};

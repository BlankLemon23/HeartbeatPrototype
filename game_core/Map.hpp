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

enum class SightMode {
    RealTerrain,
    KnownTerrain
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

    bool hasLineOfSight(Position start, Position target,
                        SightMode mode = SightMode::RealTerrain) const;
    //这样写 调用时参数只需要传入一个start和一个target mode可以选择性的传

private:
    bool blocksLineOfSight(Position cell, SightMode mode) const;

    // Terrain data and the discovery state have different lifetimes.
    std::vector<std::string> map_;
    std::vector<std::vector<bool>> scannedCells_;
};

#pragma once
#include <string>
#include <vector>

enum class CellType {
    OutOfBounds,
    Wall,
    Space,
    Unknown
};


class Map {
public:
    CellType classifyCell(int row, int column) const;
    void setMap(const std::vector<std::string>& map);
    const std::vector<std::string>& getMap() const;
    bool isScanned(int row, int column) const;
    bool markScanned(int row, int column);

private:
    std::vector<std::string> map_;
	std::vector<std::vector<bool>> scannedCells_;

};

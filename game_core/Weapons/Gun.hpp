#pragma once

#include "Position.hpp"
#include <vector>

class Enemy;
class Map;

struct GunConfig {
    int damagePerShot = 25;
    int rangeCells = 10;
    double fireIntervalSeconds = 0.5;
    int soundRadiusCells = 12;

    int magazineCapacity = 6;
    int maxTotalAmmo = 24; // Loaded and reserve rounds together.
    int initialMagazineAmmo = 6;
    int initialReserveAmmo = 12;
    double reloadDurationSeconds = 2.0;
};

class Gun {
public:
    //explicit 它用来阻止编译器自动把 GunConfig 转换成 Gun。这两者含义不同
    explicit Gun(GunConfig config);

    void update(double elapsedSeconds);

    // Returns the range-circle endpoint in cell units; ignores walls.
    // With no aim direction, returns the player's cell center.
    GridPoint calculateEdgePosition(Position playerPosition, GridPoint aimPosition) const;

    bool tryFire(Position playerPosition, GridPoint aimPosition,
                 const Map& map, std::vector<Enemy>& enemies);

    bool tryReload();

    int addAmmo(int amount);

    int getAmmoInMagazine() const;
    int getReserveAmmo() const;
    bool isReloading() const;

private:
    GunConfig config_;
    int ammoInMagazine_ = 0;
    int reserveAmmo_ = 0;
    double fireCooldownRemainingSeconds_ = 0.0;
    double reloadRemainingSeconds_ = 0.0;
};

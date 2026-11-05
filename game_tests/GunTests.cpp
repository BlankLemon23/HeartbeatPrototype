#include "Weapons/Gun.hpp"
#include "Actors/Enemy.hpp"
#include "World/Map.hpp"
#include "Actors/Player.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

int main() {
    GunConfig config{};
    const Gun geometry(config);
    const auto nearlyEqual = [](double actual, double expected) {
        return std::abs(actual - expected) < 1e-9;
    };

    // Endpoints retain direction and land on the range circle in cell units.
    const GridPoint diagonal = geometry.calculateEdgePosition({1, 1}, {4.5, 5.5});
    assert(nearlyEqual(diagonal.row, 7.5) && nearlyEqual(diagonal.column, 9.5));
    const GridPoint reverse = geometry.calculateEdgePosition({1, 1}, {-1.5, -2.5});
    assert(nearlyEqual(reverse.row, -4.5) && nearlyEqual(reverse.column, -6.5));
    const GridPoint horizontal = geometry.calculateEdgePosition({1, 1}, {1.5, 2.0});
    assert(nearlyEqual(horizontal.row, 1.5) && nearlyEqual(horizontal.column, 11.5));
    assert(nearlyEqual(std::hypot(diagonal.row - 1.5, diagonal.column - 1.5),
                       config.rangeCells));

    Map corridor;
    corridor.setMap({"....................."});
    EnemyConfig enemyConfig{};
    std::vector<Enemy> enemies;
    enemies.emplace_back(Position{0, 7}, enemyConfig);
    enemies.emplace_back(Position{0, 3}, enemyConfig);
    Gun gun(config);

    // No direction consumes neither ammunition nor the firing cooldown.
    const GridPoint noDirection = gun.calculateEdgePosition({0, 0}, {0.5, 0.5});
    assert(nearlyEqual(noDirection.row, 0.5) && nearlyEqual(noDirection.column, 0.5));
    assert(!gun.tryFire({0, 0}, {0.5, 0.5}, corridor, enemies));
    assert(gun.getAmmoInMagazine() == config.initialMagazineAmmo);

    // A nearby cursor still fires to full range and hits only the nearest cell.
    assert(gun.tryFire({0, 0}, {0.5, 1.5}, corridor, enemies));
    assert(enemies[0].getHealth() == 100 && enemies[1].getHealth() == 75);
    assert(!gun.tryFire({0, 0}, {0.5, 1.5}, corridor, enemies));
    gun.update(config.fireIntervalSeconds);
    std::reverse(enemies.begin(), enemies.end());
    assert(gun.tryFire({0, 0}, {0.5, 1.5}, corridor, enemies));
    assert(enemies[0].getHealth() == 50 && enemies[1].getHealth() == 100);

    // Repeated reload requests do not restart the transfer at two seconds.
    assert(gun.tryReload());
    assert(!gun.tryReload());
    gun.update(1.0);
    assert(gun.isReloading() && gun.getAmmoInMagazine() == 4);
    assert(!gun.tryFire({0, 0}, {0.5, 1.5}, corridor, enemies));
    gun.update(1.0);
    assert(!gun.isReloading() && gun.getAmmoInMagazine() == 6 &&
           gun.getReserveAmmo() == 10);

    // Traversal stops at the range endpoint before reaching farther enemies.
    std::vector<Enemy> atRange;
    atRange.emplace_back(Position{0, 10}, enemyConfig);
    atRange.emplace_back(Position{0, 11}, enemyConfig);
    Gun rangeGun(config);
    assert(rangeGun.tryFire({0, 0}, {0.5, 20.5}, corridor, atRange));
    assert(atRange[0].getHealth() == 75 && atRange[1].getHealth() == 100);
    atRange.erase(atRange.begin());
    rangeGun.update(config.fireIntervalSeconds);
    assert(rangeGun.tryFire({0, 0}, {0.5, 1.5}, corridor, atRange));
    assert(atRange[0].getHealth() == 100);

    Map tiny;
    tiny.setMap({"."});
    Gun boundaryGun(config);
    std::vector<Enemy> noEnemies;
    assert(boundaryGun.tryFire({0, 0}, {0.5, 1.5}, tiny, noEnemies));

    Map wall;
    wall.setMap({".*..."});
    Gun wallGun(config);
    std::vector<Enemy> behindWall;
    behindWall.emplace_back(Position{0, 3}, enemyConfig);
    assert(wallGun.tryFire({0, 0}, {0.5, 1.5}, wall, behindWall));
    assert(behindWall[0].getHealth() == 100);

    Map corner;
    corner.setMap({".*", ".."});
    Gun cornerGun(config);
    std::vector<Enemy> behindCorner;
    behindCorner.emplace_back(Position{1, 1}, enemyConfig);
    assert(cornerGun.tryFire({0, 0}, {1.5, 1.5}, corner, behindCorner));
    assert(behindCorner[0].getHealth() == 100);

    // Two mouse points in the same grid cell must retain different directions.
    Map open;
    open.setMap({"........", "........", "........", "........", "........"});
    std::vector<Enemy> aboveAndBelow;
    aboveAndBelow.emplace_back(Position{1, 3}, enemyConfig);
    aboveAndBelow.emplace_back(Position{3, 3}, enemyConfig);
    Gun mouseGun(config);
    assert(mouseGun.tryFire({2, 0}, {2.05, 1.5}, open, aboveAndBelow));
    assert(aboveAndBelow[0].getHealth() == 75 && aboveAndBelow[1].getHealth() == 100);
    mouseGun.update(config.fireIntervalSeconds);
    assert(mouseGun.tryFire({2, 0}, {2.95, 1.5}, open, aboveAndBelow));
    assert(aboveAndBelow[0].getHealth() == 75 && aboveAndBelow[1].getHealth() == 75);

    // Firing emits sound even when the listener cannot see the player.
    EnemyConfig listenerConfig = enemyConfig;
    listenerConfig.normalDetectionRadiusCells = 0;
    listenerConfig.stealthDetectionRadiusCells = 0;
    listenerConfig.wanderingRadiusCells = 0;
    std::vector<Enemy> listeners;
    listeners.emplace_back(Position{0, 5}, listenerConfig);
    Player player({0, 0}, PlayerConfig{});
    Gun loudGun(config);
    assert(loudGun.tryFire({0, 0}, {0.5, 1.5}, corridor, listeners));
    listeners[0].updateEnemy(player, 0.0, corridor, false);
    assert((listeners[0].getPosition() == Position{0, 4}));

    // Pickup results report the accepted amount and preserve valid ammunition.
    Gun pickup(config);
    assert(pickup.addAmmo(5) == 5);
    assert(pickup.addAmmo(10) == 1);
    assert(pickup.addAmmo(0) == 0 && pickup.addAmmo(-5) == 0);
    assert(pickup.getAmmoInMagazine() + pickup.getReserveAmmo() == config.maxTotalAmmo);
}

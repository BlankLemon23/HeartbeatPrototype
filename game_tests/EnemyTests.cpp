#include "Enemy.hpp"
#include <cassert>
#include <cmath>

int main() {
    EnemyConfig config{};
    config.startingHealth = 100;
    config.moveIntervalSeconds = 0.5;
    config.wanderIntervalSeconds = 0.5;
    config.wanderingRadiusCells = 1;
    config.waitAfterLossSeconds = 2.0;
    config.normalDetectionRadiusCells = 8;
    config.stealthDetectionRadiusCells = 3;
    config.stopChaseRadiusCells = 15;

    // A wall hides the player, leaving one legal neighbor for wandering.
    Map corridor;
    corridor.setMap({".", ".", "*", "."});
    const Position hiddenPlayer{3, 0};
    Enemy enemy({0, 0}, config);
    enemy.updateEnemy(0.0, corridor, hiddenPlayer, false);
    assert((enemy.getPosition() == Position{1, 0}));
    enemy.updateEnemy(0.0, corridor, hiddenPlayer, false);
    enemy.updateEnemy(0.25, corridor, hiddenPlayer, false);
    assert((enemy.getPosition() == Position{1, 0}));
    enemy.updateEnemy(0.25, corridor, hiddenPlayer, false);
    assert((enemy.getPosition() == Position{0, 0}));

    // Stealth reduces detection before the chase starts.
    Map detectionMap;
    detectionMap.setMap({"....."});
    EnemyConfig detectionConfig = config;
    detectionConfig.wanderingRadiusCells = 0;
    Enemy watcher({0, 0}, detectionConfig);
    watcher.updateEnemy(0.0, detectionMap, {0, 4}, true);
    assert(!watcher.isChasing());
    watcher.updateEnemy(0.0, detectionMap, {0, 4}, false);
    assert(watcher.isChasing());
    watcher.updateEnemy(0.0, detectionMap, {0, 4}, true);
    assert(watcher.isChasing());

    // No legal neighbor: a blocked enemy cannot move into a wall.
    Map blocked;
    blocked.setMap({".*", "**", ".."});
    Enemy trapped({0, 0}, config);
    trapped.updateEnemy(1.0, blocked, {2, 0}, false);
    assert((trapped.getPosition() == Position{0, 0}));

    // Random choices must remain adjacent, on open cells and inside the radius.
    Map openMap;
    openMap.setMap({".....", ".....", ".....", ".....", "....."});
    EnemyConfig wanderConfig = config;
    wanderConfig.normalDetectionRadiusCells = 1;
    Enemy walker({2, 2}, wanderConfig);
    for (int step = 0; step < 20; ++step) {
        const Position before = walker.getPosition();
        walker.updateEnemy(0.5, openMap, {0, 0}, false);
        const Position after = walker.getPosition();
        assert(std::abs(after.row - before.row) + std::abs(after.column - before.column) == 1);
        const int rowOffset = after.row - 2;
        const int columnOffset = after.column - 2;
        assert(rowOffset * rowOffset + columnOffset * columnOffset <= 1);
        assert(openMap.classifyCell(after.row, after.column) == CellType::Space);
    }

    wanderConfig.wanderingRadiusCells = 0;
    Enemy stationary({2, 2}, wanderConfig);
    stationary.updateEnemy(1.0, openMap, {0, 0}, false);
    assert((stationary.getPosition() == Position{2, 2}));

    // After detection, chase around a wall and move only the first path step.
    Map detour;
    detour.setMap({".*.", "..."});
    Enemy pursuer({0, 0}, config);
    pursuer.updateEnemy(0.0, detour, {1, 0}, false);
    assert(pursuer.isChasing());
    pursuer.updateEnemy(0.5, detour, {0, 2}, false);
    assert((pursuer.getPosition() == Position{1, 0}));
    pursuer.updateEnemy(0.0, detour, {0, 2}, false);
    assert((pursuer.getPosition() == Position{1, 0}));

    // Chase range is measured from the enemy. After losing the player, wait two seconds.
    EnemyConfig trackingConfig = config;
    trackingConfig.normalDetectionRadiusCells = 1;
    trackingConfig.stopChaseRadiusCells = 2;
    Enemy tracker({2, 2}, trackingConfig);
    tracker.updateEnemy(0.0, openMap, {2, 3}, false);
    assert(tracker.isChasing());
    tracker.updateEnemy(0.0, openMap, {3, 2}, false);
    assert(tracker.isChasing());
    tracker.updateEnemy(0.0, openMap, {0, 0}, false);
    assert(!tracker.isChasing());
    const Position lossPosition = tracker.getPosition();
    tracker.updateEnemy(1.0, openMap, {0, 0}, false);
    assert(tracker.getPosition() == lossPosition);
    tracker.updateEnemy(1.0, openMap, {0, 0}, false);
    assert(tracker.getPosition() == lossPosition);
    tracker.updateEnemy(0.5, openMap, {0, 0}, false);
    assert(!(tracker.getPosition() == lossPosition));
}

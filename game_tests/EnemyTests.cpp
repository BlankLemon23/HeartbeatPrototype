#include "Enemy.hpp"
#include "Player.hpp"
#include "ScanController.hpp"
#include <cassert>
#include <cmath>
#include <vector>

// Existing behavior tests provide a player at the requested position.
void updateEnemyAt(Enemy& enemy, double elapsedSeconds, const Map& map,
                   Position playerPosition, bool playerIsStealth) {
    Player player(playerPosition);
    enemy.updateEnemy(player, elapsedSeconds, map, playerIsStealth);
}

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
    config.hearSoundRadiusCells = 8;
    config.attackIntervalSeconds = 2.0;
    config.attackDamage = 8;

    // Positive damage lowers health; zero and negative values do not heal.
    Player injuredPlayer({1, 1});
    injuredPlayer.takeDamage(0);
    injuredPlayer.takeDamage(-5);
    assert(injuredPlayer.getHealth() == 100);
    Enemy injuredEnemy({1, 2}, config);
    injuredEnemy.takeDamage(-5);
    assert(injuredEnemy.getHealth() == 100);

    // The first contact hits immediately; the same enemy then waits two seconds.
    Map contactMap;
    contactMap.setMap({"...", "...", "..."});
    injuredEnemy.updateEnemy(injuredPlayer, 0.0, contactMap, false);
    assert(injuredPlayer.getHealth() == 92);
    injuredEnemy.updateEnemy(injuredPlayer, 1.0, contactMap, false);
    assert(injuredPlayer.getHealth() == 92);
    injuredEnemy.updateEnemy(injuredPlayer, 1.0, contactMap, false);
    assert(injuredPlayer.getHealth() == 84); // Same-cell contact still hits.
    injuredEnemy.takeDamage(200);
    injuredPlayer.takeDamage(200);
    assert(injuredEnemy.getHealth() == 0);
    assert(injuredPlayer.getHealth() == 0);

    // A wall hides the player, leaving one legal neighbor for wandering.
    Map corridor;
    corridor.setMap({".", ".", "*", "."});
    const Position hiddenPlayer{3, 0};
    Enemy enemy({0, 0}, config);
    updateEnemyAt(enemy, 0.0, corridor, hiddenPlayer, false);
    assert((enemy.getPosition() == Position{1, 0}));
    updateEnemyAt(enemy, 0.0, corridor, hiddenPlayer, false);
    updateEnemyAt(enemy, 0.25, corridor, hiddenPlayer, false);
    assert((enemy.getPosition() == Position{1, 0}));
    updateEnemyAt(enemy, 0.25, corridor, hiddenPlayer, false);
    assert((enemy.getPosition() == Position{0, 0}));

    // Stealth reduces detection before the chase starts.
    Map detectionMap;
    detectionMap.setMap({"....."});
    EnemyConfig detectionConfig = config;
    detectionConfig.wanderingRadiusCells = 0;
    Enemy watcher({0, 0}, detectionConfig);
    updateEnemyAt(watcher, 0.0, detectionMap, {0, 4}, true);
    assert(!watcher.isChasing());
    updateEnemyAt(watcher, 0.0, detectionMap, {0, 4}, false);
    assert(watcher.isChasing());
    updateEnemyAt(watcher, 0.0, detectionMap, {0, 4}, true);
    assert(watcher.isChasing());

    // No legal neighbor: a blocked enemy cannot move into a wall.
    Map blocked;
    blocked.setMap({".*", "**", ".."});
    Enemy trapped({0, 0}, config);
    updateEnemyAt(trapped, 1.0, blocked, {2, 0}, false);
    assert((trapped.getPosition() == Position{0, 0}));

    // Random choices must remain adjacent, on open cells and inside the radius.
    Map openMap;
    openMap.setMap({".....", ".....", ".....", ".....", "....."});
    EnemyConfig wanderConfig = config;
    wanderConfig.normalDetectionRadiusCells = 1;
    Enemy walker({2, 2}, wanderConfig);
    for (int step = 0; step < 20; ++step) {
        const Position before = walker.getPosition();
        updateEnemyAt(walker, 0.5, openMap, {0, 0}, false);
        const Position after = walker.getPosition();
        assert(std::abs(after.row - before.row) + std::abs(after.column - before.column) == 1);
        const int rowOffset = after.row - 2;
        const int columnOffset = after.column - 2;
        assert(rowOffset * rowOffset + columnOffset * columnOffset <= 1);
        assert(openMap.classifyCell(after.row, after.column) == CellType::Space);
    }

    wanderConfig.wanderingRadiusCells = 0;
    Enemy stationary({2, 2}, wanderConfig);
    updateEnemyAt(stationary, 1.0, openMap, {0, 0}, false);
    assert((stationary.getPosition() == Position{2, 2}));

    // After detection, chase around a wall and move only the first path step.
    Map detour;
    detour.setMap({".*.", "..."});
    Enemy pursuer({0, 0}, config);
    updateEnemyAt(pursuer, 0.0, detour, {1, 0}, false);
    assert(pursuer.isChasing());
    updateEnemyAt(pursuer, 0.5, detour, {0, 2}, false);
    assert((pursuer.getPosition() == Position{1, 0}));
    updateEnemyAt(pursuer, 0.0, detour, {0, 2}, false);
    assert((pursuer.getPosition() == Position{1, 0}));

    // Chase range is measured from the enemy. After losing the player, wait two seconds.
    EnemyConfig trackingConfig = config;
    trackingConfig.normalDetectionRadiusCells = 1;
    trackingConfig.stopChaseRadiusCells = 2;
    Enemy tracker({2, 2}, trackingConfig);
    updateEnemyAt(tracker, 0.0, openMap, {2, 3}, false);
    assert(tracker.isChasing());
    updateEnemyAt(tracker, 0.0, openMap, {3, 2}, false);
    assert(tracker.isChasing());
    updateEnemyAt(tracker, 0.0, openMap, {0, 0}, false);
    assert(!tracker.isChasing());
    const Position lossPosition = tracker.getPosition();
    updateEnemyAt(tracker, 1.0, openMap, {0, 0}, false);
    assert(tracker.getPosition() == lossPosition);
    updateEnemyAt(tracker, 1.0, openMap, {0, 0}, false);
    assert(tracker.getPosition() == lossPosition);
    updateEnemyAt(tracker, 0.5, openMap, {0, 0}, false);
    assert(!(tracker.getPosition() == lossPosition));

    // A scan captures enemy positions at that moment; walls and radius limit the snapshot.
    Map snapshotMap;
    snapshotMap.setMap({".*...", "....."});
    const Player observer({0, 0});
    std::vector<Enemy> snapshotCandidates;
    snapshotCandidates.emplace_back(Position{1, 0}, config); // Visible.
    snapshotCandidates.emplace_back(Position{0, 2}, config); // Behind a wall.
    snapshotCandidates.emplace_back(Position{0, 4}, config); // Outside radius three.
    ScanController snapshotScanner(0.0, 3.0, 0.5);
    assert(snapshotScanner.tryScanManually(observer, snapshotMap, 0.0, snapshotCandidates));
    assert(snapshotScanner.getEnemySnapshotPositions().size() == 1);
    assert((snapshotScanner.getEnemySnapshotPositions().front() == Position{1, 0}));
    snapshotScanner.updateSnapshotRemainingSeconds(0.25);
    assert(snapshotScanner.getEnemySnapshotPositions().size() == 1);
    snapshotScanner.updateSnapshotRemainingSeconds(0.25);
    assert(snapshotScanner.getEnemySnapshotPositions().empty());

    // The next scan replaces the old snapshot, even when no enemies are seen.
    assert(snapshotScanner.tryScanManually(observer, snapshotMap, 0.0, snapshotCandidates));
    const std::vector<Enemy> noEnemies;
    assert(snapshotScanner.tryScanManually(observer, snapshotMap, 0.0, noEnemies));
    assert(snapshotScanner.getEnemySnapshotPositions().empty());

    // Sound crosses a wall: only listeners in range investigate the fixed source.
    Map soundMap;
    soundMap.setMap({".....", ".***.", "....."});
    Player soundOwner({2, 2});
    EnemyConfig soundConfig = config;
    soundConfig.wanderingRadiusCells = 0;
    soundConfig.hearSoundRadiusCells = 2;
    std::vector<Enemy> listeners;
    listeners.emplace_back(Position{0, 2}, soundConfig);
    listeners.emplace_back(Position{0, 0}, soundConfig);
    soundOwner.makeSound(soundOwner.getPosition(), listeners);
    for (Enemy& listener : listeners) {
        updateEnemyAt(listener, 0.5, soundMap, soundOwner.getPosition(), false);
    }
    assert(!(listeners[0].getPosition() == Position{0, 2}));
    assert((listeners[1].getPosition() == Position{0, 0}));
    assert(!listeners[0].isChasing());

    // An unreachable sound source ends investigation and resumes local wandering.
    Map separatedMap;
    separatedMap.setMap({"...", "***", "..."});
    EnemyConfig separatedConfig = config;
    separatedConfig.wanderingRadiusCells = 1;
    Enemy separatedListener({0, 1}, separatedConfig);
    separatedListener.hearSound({2, 1});
    updateEnemyAt(separatedListener, 0.0, separatedMap, {2, 1}, false);
    assert((separatedListener.getPosition() == Position{0, 1}));
    updateEnemyAt(separatedListener, separatedConfig.wanderIntervalSeconds,
                                  separatedMap, {2, 1}, false);
    assert(!(separatedListener.getPosition() == Position{0, 1}));
}

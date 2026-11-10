#include "Actors/Enemy.hpp"
#include "Actors/Player.hpp"
#include "Perception/Sound.hpp"
#include "World/Map.hpp"
#include <cassert>
#include <vector>

static constexpr PlayerConfig playerConfig{100, 0.5, 1.0, 2.0, 20, 60};

// Existing behavior tests provide a player at the requested position.
void updateEnemyAt(Enemy& enemy, double elapsedSeconds, const Map& map,
	Position playerPosition, bool playerIsStealth) {
	Player player(playerPosition, playerConfig);
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
	config.attackIntervalSeconds = 2.0;
	config.attackDamage = 8;

	// Positive damage lowers health; zero and negative values do not heal.
	Player injuredPlayer({1, 1}, playerConfig);
	injuredPlayer.takeDamage(0);
	injuredPlayer.takeDamage(-5);
	assert(injuredPlayer.getHealth() == 100);
	Enemy injuredEnemy({1, 2}, config);
	injuredEnemy.takeDamage(-5);
	assert(injuredEnemy.getHealth() == 100);

	// The first contact hits immediately; the same enemy then waits two seconds.
	Map contactMap(MapConfig{{"...", "...", "..."}});
	injuredEnemy.updateEnemy(injuredPlayer, 0.0, contactMap, false);
	assert(injuredPlayer.getHealth() == 92);
	injuredEnemy.updateEnemy(injuredPlayer, 1.0, contactMap, false);
	assert(injuredPlayer.getHealth() == 92);
	injuredEnemy.updateEnemy(injuredPlayer, 1.0, contactMap, false);
	assert(injuredPlayer.getHealth() == 84); // Same-cell contact still hits.

	// A wall hides the player, leaving one legal neighbor for wandering.
	Map corridor(MapConfig{{".", ".", "*", "."}});
	const Position hiddenPlayer{3, 0};
	Enemy enemy({0, 0}, config);
	updateEnemyAt(enemy, 0.0, corridor, hiddenPlayer, false);
	assert((enemy.getPosition() == Position{1, 0}));

	// After detection, chase around a wall and move only the first path step.
	Map detour(MapConfig{{".*.", "..."}});
	Enemy pursuer({0, 0}, config);
	updateEnemyAt(pursuer, 0.0, detour, {1, 0}, false);
	assert(pursuer.isChasing());
	updateEnemyAt(pursuer, 0.5, detour, {0, 2}, false);
	assert((pursuer.getPosition() == Position{1, 0}));
	updateEnemyAt(pursuer, 0.0, detour, {0, 2}, false);
	assert((pursuer.getPosition() == Position{1, 0}));

	// Sound crosses a wall: only listeners in range investigate the fixed source.
	Map soundMap(MapConfig{{".....", ".***.", "....."}});
	Player soundOwner({2, 2}, playerConfig);
	EnemyConfig soundConfig = config;
	soundConfig.wanderingRadiusCells = 0;
	std::vector<Enemy> listeners;
	listeners.emplace_back(Position{0, 2}, soundConfig);
	listeners.emplace_back(Position{0, 0}, soundConfig);
	makeSound(soundOwner.getPosition(), 2, listeners);
	for (Enemy& listener : listeners) {
		updateEnemyAt(listener, 0.5, soundMap, soundOwner.getPosition(), false);
	}
	assert(!(listeners[0].getPosition() == Position{0, 2}));
	assert((listeners[1].getPosition() == Position{0, 0}));
	assert(!listeners[0].isChasing());

	// An unreachable sound source ends investigation and resumes local wandering.
	Map separatedMap(MapConfig{{"...", "***", "..."}});
	EnemyConfig separatedConfig = config;
	separatedConfig.wanderingRadiusCells = 1;
	Enemy separatedListener({0, 1}, separatedConfig);
	separatedListener.hearSound({2, 1}, 8);
	updateEnemyAt(separatedListener, 0.0, separatedMap, {2, 1}, false);
	assert((separatedListener.getPosition() == Position{0, 1}));
	updateEnemyAt(separatedListener, separatedConfig.wanderIntervalSeconds,
		separatedMap, {2, 1}, false);
	assert(!(separatedListener.getPosition() == Position{0, 1}));
}

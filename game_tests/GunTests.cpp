#include "Weapons/Gun.hpp"
#include "Actors/Enemy.hpp"
#include "World/Map.hpp"
#include "Actors/Player.hpp"
#include <algorithm>
#include <cassert>
#include <vector>

int main() {
	GunConfig config{};
	config.damagePerShot = 25;
	config.rangeCells = 10;
	config.fireIntervalSeconds = 0.5;
	config.soundRadiusCells = 12;
	config.magazineCapacity = 6;
	config.maxTotalAmmo = 24;
	config.initialMagazineAmmo = 6;
	config.initialReserveAmmo = 12;
	config.reloadDurationSeconds = 2.0;

	Map corridor(MapConfig{{"....................."}});
	EnemyConfig enemyConfig{};
	enemyConfig.startingHealth = 100;
	enemyConfig.moveIntervalSeconds = 2.0;
	enemyConfig.wanderIntervalSeconds = 0.5;
	enemyConfig.attackIntervalSeconds = 2.0;
	enemyConfig.wanderingRadiusCells = 3;
	enemyConfig.waitAfterLossSeconds = 2.0;
	enemyConfig.normalDetectionRadiusCells = 8;
	enemyConfig.stealthDetectionRadiusCells = 3;
	enemyConfig.stopChaseRadiusCells = 15;
	enemyConfig.attackDamage = 8;
	std::vector<Enemy> enemies;
	enemies.emplace_back(Position{0, 7}, enemyConfig);
	enemies.emplace_back(Position{0, 3}, enemyConfig);
	Gun gun(config);

	// A nearby cursor still fires to full range and hits only the nearest cell.
	assert(gun.tryFire({0, 0}, {0.5, 1.5}, corridor, enemies));
	assert(enemies[0].getHealth() == 100 && enemies[1].getHealth() == 75);
	gun.update(config.fireIntervalSeconds);
	std::reverse(enemies.begin(), enemies.end());
	assert(gun.tryFire({0, 0}, {0.5, 1.5}, corridor, enemies));
	assert(enemies[0].getHealth() == 50 && enemies[1].getHealth() == 100);

	// Firing emits sound even when the listener cannot see the player.
	EnemyConfig listenerConfig = enemyConfig;
	listenerConfig.normalDetectionRadiusCells = 0;
	listenerConfig.stealthDetectionRadiusCells = 0;
	listenerConfig.wanderingRadiusCells = 0;
	std::vector<Enemy> listeners;
	listeners.emplace_back(Position{0, 5}, listenerConfig);
	const PlayerConfig playerConfig{100, 0.5, 1.0, 2.0, 20, 60};
	Player player({0, 0}, playerConfig);
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

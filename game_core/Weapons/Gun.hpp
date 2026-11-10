#pragma once

#include "World/Position.hpp"
#include <vector>

class Enemy;
class Map;

struct GunConfig {
	int damagePerShot;
	int rangeCells;
	double fireIntervalSeconds;
	int soundRadiusCells;

	int magazineCapacity;
	int maxTotalAmmo; // Loaded and reserve rounds together.
	int initialMagazineAmmo;
	int initialReserveAmmo;
	double reloadDurationSeconds;
};

class Gun {
public:
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
	int ammoInMagazine_;
	int reserveAmmo_;
	double fireCooldownRemainingSeconds_ = 0.0;
	double reloadRemainingSeconds_ = 0.0;
};

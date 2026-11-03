#include "Gun.hpp"
#include <algorithm>
#include <cmath>
#include "Map.hpp"
#include "Position.hpp"
#include "Enemy.hpp"
#include "Sound.hpp"

Gun::Gun(GunConfig config)
{
	config_ = config;
	ammoInMagazine_ = config_.initialMagazineAmmo;
	reserveAmmo_ = config_.initialReserveAmmo;

}

void Gun::update(double elapsedSeconds) {
	fireCooldownRemainingSeconds_ = std::max(0.0, fireCooldownRemainingSeconds_ - elapsedSeconds);

	if (reloadRemainingSeconds_ <= 0.0) {
		return;                       
	}
	reloadRemainingSeconds_ -= elapsedSeconds;
	if (reloadRemainingSeconds_ > 0.0) {
		return;                       
	}
	reloadRemainingSeconds_ = 0.0;

	const int missingRounds = config_.magazineCapacity - ammoInMagazine_;//弹夹还差几发
	const int loadedRounds = std::min(missingRounds, reserveAmmo_);//这次能装进去几发
	ammoInMagazine_ += loadedRounds;
	reserveAmmo_ -= loadedRounds;
}

GridPoint Gun::calculateEdgePosition(Position playerPosition, GridPoint aimPosition) const {
    const GridPoint playerCenter{
        playerPosition.row + 0.5,
        playerPosition.column + 0.5
    };
    const double rowOffset = aimPosition.row - playerCenter.row;
    const double columnOffset = aimPosition.column - playerCenter.column;
    const double aimDistance = std::hypot(rowOffset, columnOffset);//也就是计算两个offset的平方和

    if (aimDistance == 0.0) {
        return playerCenter;
    }

    const double scale = config_.rangeCells / aimDistance;
    return {
        playerCenter.row + rowOffset * scale,
        playerCenter.column + columnOffset * scale
    };
}

bool Gun::tryFire(Position playerPosition, GridPoint aimPosition,
    const Map& map, std::vector<Enemy>& enemies) {
    if (fireCooldownRemainingSeconds_ > 0.0 || reloadRemainingSeconds_ > 0.0) {
        return false;
    }
    if (ammoInMagazine_ <= 0) {
        return false;
    }

    const GridPoint edgePosition = calculateEdgePosition(playerPosition, aimPosition);
    if (edgePosition.row == playerPosition.row + 0.5 &&
        edgePosition.column == playerPosition.column + 0.5) {
        return false;
    }

    ammoInMagazine_ -= 1;
    fireCooldownRemainingSeconds_ = config_.fireIntervalSeconds;
    makeSound(playerPosition, config_.soundRadiusCells, enemies);

    const Position firstEnemyPosition =
        map.findFirstEnemyPositionFromPlayerToTarget(playerPosition, edgePosition, enemies);
    const Position invalidPosition{-1, -1};
    if (!(firstEnemyPosition == invalidPosition)) {
        for (Enemy& enemy : enemies) {
            if (enemy.getPosition() == firstEnemyPosition) {
                enemy.takeDamage(config_.damagePerShot);
                break;
            }
        }
    }

    return true;
}
bool Gun::tryReload()
{
	if (reloadRemainingSeconds_ <= 0.0 
		&& 
		ammoInMagazine_ < config_.magazineCapacity 
		&&
		reserveAmmo_ > 0) {
		reloadRemainingSeconds_ = config_.reloadDurationSeconds;
		return true;
	}
	return false;
}

int Gun::addAmmo(int amount)
{
    if (amount <= 0) {
        return 0;
    }
	if ((reserveAmmo_ + ammoInMagazine_ + amount) < config_.maxTotalAmmo) {
		reserveAmmo_ += amount;
		return amount;
	}
	int acceptedAmmo = (config_.maxTotalAmmo - ammoInMagazine_ - reserveAmmo_);
	reserveAmmo_ = (config_.maxTotalAmmo - ammoInMagazine_);
	return acceptedAmmo;
}

int Gun::getAmmoInMagazine() const
{
	return ammoInMagazine_;
}

int Gun::getReserveAmmo() const
{
	return reserveAmmo_;
}

bool Gun::isReloading() const
{
	if (reloadRemainingSeconds_ > 0.0) {
		return true;
	}
	else {
		return false;
	}
}

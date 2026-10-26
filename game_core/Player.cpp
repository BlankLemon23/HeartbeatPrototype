#include "Player.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>


Player::Player(Position startPosition, PlayerConfig config)
	: config_(config), position_(startPosition), health_(config.maxHealth),
	  outBattleRemainingSeconds_(config.outBattleIntervalSeconds) {
	if (config.maxHealth <= 0) {
		throw std::invalid_argument("maxHealth must be positive");
	}
	if (!std::isfinite(config.normalMoveIntervalSeconds) || config.normalMoveIntervalSeconds <= 0.0) {
		throw std::invalid_argument("normalMoveIntervalSeconds must be finite and positive");
	}
	if (!std::isfinite(config.stealthMoveIntervalSeconds) || config.stealthMoveIntervalSeconds <= 0.0) {
		throw std::invalid_argument("stealthMoveIntervalSeconds must be finite and positive");
	}
	if (!std::isfinite(config.outBattleIntervalSeconds) || config.outBattleIntervalSeconds <= 0.0) {
		throw std::invalid_argument("outBattleIntervalSeconds must be finite and positive");
	}
}

bool Player::tryMove(Direction direction, const Map& map) {
	if (moveCooldownRemainingSeconds_ > 0.0) {
		return false;
	}

	Position newPosition = position_;
	switch (direction) {
	case Direction::UP:
		newPosition.row--;
		break;
	case Direction::DOWN:
		newPosition.row++;
		break;
	case Direction::LEFT:
		newPosition.column--;
		break;
	case Direction::RIGHT:
		newPosition.column++;
		break;
	default:
		return false;
	}
	if (map.classifyCell(newPosition.row, newPosition.column) == CellType::Space) {
		moveCooldownRemainingSeconds_ = stealthEnabled_
			? config_.stealthMoveIntervalSeconds
			: config_.normalMoveIntervalSeconds; //撞墙不触发冷却，只有成功移动才触发冷却
		position_ = newPosition;
		return true;
	}
	return false;
}

void Player::updateMoveCooldown(double elapsedSeconds) {
	if (moveCooldownRemainingSeconds_ > 0) {
		moveCooldownRemainingSeconds_ = std::max(0.0, moveCooldownRemainingSeconds_ - elapsedSeconds);
	}
}

double Player::getMoveCooldownRemainingSeconds() const {
	return moveCooldownRemainingSeconds_;
}

Position Player::getPosition() const {
	return position_;
}

void Player::setPosition(Position newPosition) {
	position_ = newPosition;
}

int Player::getHealth() const {
	return health_;
}

int Player::getMaxHealth() const {
	return config_.maxHealth;
}

void Player::setHealth(int health) {
	health_ = std::clamp(health, 0, config_.maxHealth); //把 health 的值约束在 [0, config_.maxHealth] 之间，结果赋给 health_。
}

HpLevel Player::getHpLevel() const {
	if (health_ == 0) {
		return HpLevel::Dead;
	}
	if (health_ <= config_.maxHealth / 5) {
		return HpLevel::Low;
	}
	if (health_ <= 3 * config_.maxHealth / 5) {
		return HpLevel::Medium;
	}
	return HpLevel::High;
}

bool Player::isInBattle() const {
	return isInBattle_;
}

void Player::setInBattle(bool isInBattle) {
	isInBattle_ = isInBattle;
}

bool Player::wasInBattle() const {
	return wasInBattle_;
}

void Player::setWasInBattle(bool wasInBattle) {
	wasInBattle_ = wasInBattle;
}

void Player::toggleStealth() {
	stealthEnabled_ = !stealthEnabled_;
}

bool Player::isStealthEnabled() const {
	return stealthEnabled_;
}

double Player::getOutBattleRemainingSeconds() const {
	return outBattleRemainingSeconds_;
}

void Player::updateOutBattleRemainingSeconds(double elapsedSeconds) {
	outBattleRemainingSeconds_ = std::max(0.0, outBattleRemainingSeconds_ - elapsedSeconds);
}

void Player::resetOutBattleRemainingSeconds() {
	outBattleRemainingSeconds_ = config_.outBattleIntervalSeconds;
}

void Player::makeSound(Position soundSource, std::vector<Enemy>& enemies) {
	for (Enemy& enemy : enemies) {
		enemy.hearSound(soundSource);
	}
}

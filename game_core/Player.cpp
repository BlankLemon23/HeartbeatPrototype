#include "Player.hpp"
#include "Map.hpp"
#include <algorithm>
#include <stdexcept>


bool Player::tryMove(Direction direction, const Map& map, double moveIntervalSeconds) {
	if (moveCooldownRemainingSeconds_ > 0) {
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
		moveCooldownRemainingSeconds_ = moveIntervalSeconds; //撞墙不触发冷却，只有成功移动才触发冷却
		position_ = newPosition;
		return true;
	}
	return false;
}

Player::Player(Position startPosition, int maxHealth)
	: position_(startPosition), maxHealth_(maxHealth), health_(maxHealth) {
	if (maxHealth <= 0) {
		throw std::invalid_argument("maxHealth must be positive");
	}
}

HpLevel Player::getHpLevel() const
{
	if (health_ == 0)
	{
		return HpLevel::Dead;
	}
	if (health_ <= maxHealth_ / 5)
	{
		return HpLevel::Low;
	}
	if (maxHealth_ / 5 < health_ && health_ <= 3 * maxHealth_ / 5)
	{
		return HpLevel::Medium;
	}
	return HpLevel::High;
}

Position Player::getPosition() const {
	return position_;
}

void Player::setPosition(Position newPosition)
{
	position_ = newPosition;
}

bool Player::isInBattle() const {
	return isInBattle_;
}

void Player::setInBattle(bool isInBattle) {
	isInBattle_ = isInBattle;
}

bool Player::wasInBattle() const
{
	return wasInBattle_;
}

void Player::setWasInBattle(bool wasInBattle)
{
	wasInBattle_ = wasInBattle;
}

int Player::getHealth() const
{
	return health_;
}

int Player::getMaxHealth() const
{
	return maxHealth_;
}

void Player::setHealth(int health)
{
	health_ = std::clamp(health, 0, maxHealth_);//把 health 的值约束在 [0, maxHealth_] 之间，结果赋给 health_。
}


double Player::getMoveCooldownRemainingSeconds() const
{
	return moveCooldownRemainingSeconds_;
}

void Player::updateMoveCooldown(double elapsedSeconds)
{
	if (moveCooldownRemainingSeconds_ > 0) {
		moveCooldownRemainingSeconds_ = std::max(0.0, moveCooldownRemainingSeconds_ - elapsedSeconds);
	}
}

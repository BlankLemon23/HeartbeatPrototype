#pragma once

#include "World/Map.hpp"
#include "World/Position.hpp"

// Keep levels ordered so a smaller value means a lower health band.
enum class HpLevel {
	Dead,
	Low,
	Medium,
	High
};

enum class Direction {
	UP,
	DOWN,
	LEFT,
	RIGHT
};

struct PlayerConfig {
	int maxHealth;
	double normalMoveIntervalSeconds;
	double stealthMoveIntervalSeconds;
	double outBattleIntervalSeconds;
	int lowHealthThresholdPercent;
	int mediumHealthThresholdPercent;
};

class Player {
public:
	explicit Player(Position startPosition, PlayerConfig config);

	bool tryMove(Direction direction, const Map& map);
	void updateMoveCooldown(double elapsedSeconds);
	double getMoveCooldownRemainingSeconds() const;

	Position getPosition() const;
	void setPosition(Position newPosition);

	int getHealth() const;
	int getMaxHealth() const;
	void setHealth(int health);
	HpLevel getHpLevel() const;

	bool isInBattle() const;
	void setInBattle(bool isInBattle);
	bool wasInBattle() const;
	void setWasInBattle(bool wasInBattle);

	void toggleStealth();
	bool isStealthEnabled() const;

	double getOutBattleRemainingSeconds() const;
	void updateOutBattleRemainingSeconds(double elapsedSeconds);
	void resetOutBattleRemainingSeconds();

	void takeDamage(int damage);

private:
	// Fixed settings are separate from the state that changes each frame.
	PlayerConfig config_;

	Position position_;
	int health_;
	double moveCooldownRemainingSeconds_ = 0.0;
	double outBattleRemainingSeconds_;
	bool isInBattle_ = false;
	bool wasInBattle_ = false;
	bool stealthEnabled_ = false;
};

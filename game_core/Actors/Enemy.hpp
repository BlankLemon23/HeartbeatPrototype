#pragma once
#include "World/Map.hpp"
#include "World/Position.hpp"
#include <random>

class Player;

struct EnemyConfig {
	int startingHealth;
	double moveIntervalSeconds;
	double wanderIntervalSeconds;
	double attackIntervalSeconds;
	int wanderingRadiusCells;
	double waitAfterLossSeconds;
	int normalDetectionRadiusCells;
	int stealthDetectionRadiusCells;
	int stopChaseRadiusCells;
	int attackDamage;
};

enum class EnemyState {
	Wandering,
	Chasing,
	WaitingAfterLoss,
	InvestigatingSound
};

class Enemy {
public:
	explicit Enemy(Position startPosition, EnemyConfig config);
	void updateEnemy(Player& player, double elapsedSeconds, const Map& map, bool playerIsStealth);
	Position getPosition() const;
	int getHealth() const;
	bool isChasing() const;
	void hearSound(Position soundSource, int soundRadiusCells);
	void takeDamage(int damage);

private:
	void tryAttack(Player& player, double elapsedSeconds);
	bool canHearSound(Position soundSource, int soundRadiusCells) const;
	bool canSeePlayer(Position playerPosition, bool playerIsStealth, const Map& map) const;
	bool tryMoveTo(Position nextPosition, const Map& map);
	bool tryMoveToward(Position targetPosition, const Map& map);
	void wander(const Map& map);
	void stopChasing();
	// Seed once per enemy; retain the generator state between moves.
	std::mt19937 randomEngine_{std::random_device{}()};
	EnemyConfig config_;
	Position position_;
	Position wanderCenter_;
	Position soundSourcePosition_;
	int health_;
	double moveCooldownRemainingSeconds_ = 0.0;
	double waitRemainingSeconds_ = 0.0;
	double wanderRemainingSeconds_ = 0.0;
	double attackCooldownRemainingSeconds_ = 0.0;
	EnemyState state_ = EnemyState::Wandering;
};

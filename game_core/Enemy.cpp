#include "Enemy.hpp"
#include "Player.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>
#include <queue>

//public

Enemy::Enemy(Position startPosition, const EnemyConfig& config)
    : config_(config),
    position_(startPosition),
    wanderCenter_(startPosition),
    health_(config.startingHealth) {
} //这个构造函数很重要 学习写法 



void Enemy::updateEnemy(Player& player, double elapsedSeconds, const Map& map, bool playerIsStealth)
{
	
	if (health_ <= 0) {
		return;
	}
	const Position playerPosition = player.getPosition();
	tryAttack(player, elapsedSeconds);
	bool ifEnemyCanSeeThePlayer = canSeePlayer(playerPosition, playerIsStealth, map);
	const int distanceSquared = calculateSquaredDistanceToTarget(playerPosition);


	switch (state_) {
	case EnemyState::Wandering:
		if (ifEnemyCanSeeThePlayer) {
			state_ = EnemyState::Chasing;
			break;
		}
		else {
			wanderRemainingSeconds_ -= elapsedSeconds;
			wander(map);
		}
		break;

	case EnemyState::Chasing:
		if (distanceSquared >
			config_.stopChaseRadiusCells * config_.stopChaseRadiusCells) {
			stopChasing();
			break;
		}
		else {
			moveCooldownRemainingSeconds_ -= elapsedSeconds;
			if (moveCooldownRemainingSeconds_ <= 0.0) {
				tryMoveToward(playerPosition, map);
			}
		}
		break;
	case EnemyState::WaitingAfterLoss:
		if (ifEnemyCanSeeThePlayer) {
			state_ = EnemyState::Chasing;
			waitRemainingSeconds_ = config_.waitAfterLossSeconds;
			break;
		}
		waitRemainingSeconds_ -= elapsedSeconds;
		if (waitRemainingSeconds_ <= 0) {
			state_ = EnemyState::Wandering;
		}
		break;
	case EnemyState::InvestigatingSound:
		if (ifEnemyCanSeeThePlayer) {
			state_ = EnemyState::Chasing;
			break;
		}

		if (position_ == soundSourcePosition_) {
			stopChasing();
			break;
		}
		moveCooldownRemainingSeconds_ -= elapsedSeconds;
		if (moveCooldownRemainingSeconds_ <= 0.0 &&
			!tryMoveToward(soundSourcePosition_, map)) {
			// The source cannot be reached: wander around the current position.
			wanderCenter_ = position_;
			wanderRemainingSeconds_ = config_.wanderIntervalSeconds;
			state_ = EnemyState::Wandering;
		}
		break;

	default:
		break;
	}


}

Position Enemy::getPosition() const
{
    return position_;
}

int Enemy::getHealth() const
{
	return health_;
}


//private
bool Enemy::isChasing() const
{
	if (state_ == EnemyState::Chasing) {
		return true;
	}
	else {
		return false;
	}
}

//tell if the enemy can see the player while not chasing(player is stealth or not)
bool Enemy::canSeePlayer(Position playerPosition, bool playerIsStealth, const Map& map) const
{
	const double enemyCenterRow = position_.row + 0.5;
	const double enemyCenterColumn = position_.column + 0.5;
	const double playerCenterRow = playerPosition.row + 0.5;
	const double playerCenterColumn = playerPosition.column + 0.5;

	const double rowDistance = std::abs(playerCenterRow - enemyCenterRow);
	const double columnDistance = std::abs(playerCenterColumn - enemyCenterColumn);

	
	int detectionRadius = 0;

	if (playerIsStealth) {
		detectionRadius = config_.stealthDetectionRadiusCells;
	}
	else {
		detectionRadius = config_.normalDetectionRadiusCells;
	}
	bool ifInDetectionRadius = (rowDistance * rowDistance + columnDistance * columnDistance) <= (detectionRadius * detectionRadius);
	
	if (ifInDetectionRadius && map.hasLineOfSight(position_, playerPosition) ){
		return true;
	}
	return false;
	
}
//for chase,investigate and and wander,wenn cooldown = 0 ,move to next position and set cooldown to interval
bool Enemy::tryMoveTo(Position nextPosition, const Map& map)
{
	if (state_ == EnemyState::Chasing || state_ == EnemyState::InvestigatingSound) {
		if (health_ <= 0 || moveCooldownRemainingSeconds_ > 0.0) {
			return false;
		}
	}
	else {
		if (health_ <= 0 ||wanderRemainingSeconds_ > 0.0) {
			return false;
		}
	}
    

    const double rowStep = static_cast<double>(nextPosition.row) - position_.row;
    const double columnStep = static_cast<double>(nextPosition.column) - position_.column;

    if (std::abs(rowStep) + std::abs(columnStep) != 1.0 ||
        map.classifyCell(nextPosition.row, nextPosition.column) != CellType::Space) {
        return false;
    }

    position_ = nextPosition;

	if (state_ == EnemyState::Chasing || state_ == EnemyState::InvestigatingSound) {
		moveCooldownRemainingSeconds_ = config_.moveIntervalSeconds;
	}
	else {
		wanderRemainingSeconds_ = config_.wanderIntervalSeconds;
	}

    return true;
}
//实现朝目标寻路 重点学习语法和previous frontier visited的数据结构如何定义和初始化以及更新
bool Enemy::tryMoveToward(Position targetPosition, const Map& map)
{
	if (position_ == targetPosition) {
		return true;
	}
	//frontier 已经发现、但还没被探索(展开)的节点集合
	std::queue<Position> frontier;
	
	std::vector<std::vector<bool>> visited;
	for (const std::string& row : map.getMap()) {
		visited.emplace_back(row.size(), false);
	}

	std::vector<std::vector<Position>> previous;
	for (const auto& row : map.getMap()) {
		previous.emplace_back(row.size(), Position{ -1, -1 });
	}

	const std::array<Position, 4> offsets{ { { -1, 0 },{ 1, 0 },{ 0, -1 },{ 0, 1 } } };

	visited[position_.row][position_.column] = true;
	frontier.push(position_);

	Position current = position_;
	while (!frontier.empty()) {
		//如果是Position current = frontier.front(); 就会重新定义一个current
		 current = frontier.front(); 
		if (current == targetPosition) {
			break; 
		}
		frontier.pop();

			for (Position offset : offsets) {
				Position nextPosition = { current.row + offset.row,current.column + offset.column };
				if (map.classifyCell(nextPosition.row, nextPosition.column) == CellType::Space
					&&
					visited[nextPosition.row][nextPosition.column] == false) {
					frontier.push(nextPosition);
					visited[nextPosition.row][nextPosition.column] = true;
					previous[nextPosition.row][nextPosition.column] = current;
				}
				
			}
	
	}

	if (frontier.empty()) {
		return false;
	}
	
	Position previousPosition = previous[current.row][current.column];
		while (!(previousPosition == position_)) {
			current = previousPosition;
			previousPosition = previous[current.row][current.column];
		}

		return tryMoveTo(current, map);
}

void Enemy::wander(const Map& map)
{
    if (state_ != EnemyState::Wandering || health_ <= 0 || wanderRemainingSeconds_ > 0.0) {
        return;
    }

    const std::array<Position, 4> offsets{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};

    std::vector<Position> validPositions;
    validPositions.reserve(offsets.size());
	//reverse 提前申请好能容纳 n 个元素的内存，但不改变元素数量 size仍是0 这样可以提高性能 

    const double radius = config_.wanderingRadiusCells;
    const double radiusSquared = radius * radius;

    for (const Position& offset : offsets) {
        const Position candidate{
            position_.row + offset.row,
            position_.column + offset.column
        };
        if (map.classifyCell(candidate.row, candidate.column) != CellType::Space) {
            continue;
        }
        const double rowOffset = static_cast<double>(candidate.row) - wanderCenter_.row;
        const double columnOffset = static_cast<double>(candidate.column) - wanderCenter_.column;
		//static_cast<double> int到double的数据转换


        if (rowOffset * rowOffset + columnOffset * columnOffset <= radiusSquared) {
            validPositions.push_back(candidate);
        }
    }

    if (validPositions.empty()) {
        return;
    }
    // Choose only from legal neighbors so a blocked direction cannot waste a move.
    std::uniform_int_distribution<std::size_t> chooseIndex(0, validPositions.size() - 1);
    tryMoveTo(validPositions[chooseIndex(randomEngine_)], map);
	
	//•	std::uniform_int_distribution<std::size_t> 
	// 是一个"均匀整数分布"类型，<std::size_t> 表示它产生 size_t 无符号整数 常用于下标
	//•	chooseIndex(randomEngine_) —— 把引擎 randomEngine_ 喂给分布，生成一个在范围内的随机整数。
	//引擎是随机性的来源 负责产生原始随机比特流
}

void Enemy::stopChasing()
{

state_ = EnemyState::WaitingAfterLoss;
wanderCenter_ = position_;
wanderRemainingSeconds_ = config_.wanderIntervalSeconds;
waitRemainingSeconds_ = config_.waitAfterLossSeconds;	
}

int Enemy::calculateSquaredDistanceToTarget(Position targetPosition) const
{
	int rowOffset = targetPosition.row - position_.row;
	int columnOffset = targetPosition.column - position_.column;
	return rowOffset * rowOffset + columnOffset * columnOffset;
		    
}

bool Enemy::canHearSound(Position soundSource) const {
	const int distanceSquaredToSound = calculateSquaredDistanceToTarget(soundSource);
	const int hearingRadius = config_.hearSoundRadiusCells;
	return distanceSquaredToSound <= hearingRadius * hearingRadius;
}

void Enemy::hearSound(Position soundSource) {
	if (health_ <= 0 || state_ == EnemyState::Chasing || !canHearSound(soundSource)) {
		return;
	}
	soundSourcePosition_ = soundSource;
	state_ = EnemyState::InvestigatingSound;
}

void Enemy::takeDamage(int damage) {
	if (damage <= 0) {
		return;
	}
	health_ = std::max(0,health_ - damage);
};

void Enemy::tryAttack(Player& player,double elapsedSeconds) {
	attackCooldownRemainingSeconds_ = std::max(0.0,attackCooldownRemainingSeconds_ - elapsedSeconds);
	const std::array<Position, 5> offsets = { { { -1, 0 },{ 1, 0 },{ 0, -1 },{ 0, 1 },{ 0,0 } } };
	for (const Position offset : offsets) {
		Position checkPosition = { position_.row + offset.row,position_.column + offset.column };
		if (checkPosition == player.getPosition() && attackCooldownRemainingSeconds_ <= 0) {

				attackCooldownRemainingSeconds_ = config_.attackIntervalSeconds;
				player.takeDamage(config_.attackDamage);

		}
	}

};

#pragma once
#include "Map.hpp"
#include "Position.hpp"
#include <random>


struct EnemyConfig {
    int startingHealth;
    double moveIntervalSeconds;
    double wanderIntervalSeconds;
    int wanderingRadiusCells;
    double waitAfterLossSeconds;
    int normalDetectionRadiusCells;
    int stealthDetectionRadiusCells;
    int stopChaseRadiusCells;
    int hearSoundRadiusCells;
};

enum class EnemyState {
    Wandering,
    Chasing,
    WaitingAfterLoss,
    InvestigatingSound
};

class Enemy {
    //公有的方法外部可以调用 私有方法一般不会被外部调用 才定义为私有
public:
    Enemy(Position startPosition, const EnemyConfig& config);
    void updateEnemy(double elapsedSeconds, const Map& map, Position playerPosition, bool playerIsStealth);
    Position getPosition() const;
    int getHealth() const;
    bool isChasing() const;
    void hearSound(Position soundSource);

private:
    bool canHearSound(Position soundSource) const;
    bool canSeePlayer(Position playerPosition, bool playerIsStealth, const Map& map) const;
    bool tryMoveTo(Position nextPosition, const Map& map);
    bool tryMoveToward(Position targetPosition, const Map& map);
    void wander(const Map& map);
    void stopChasing();
    int calculateSquaredDistanceToTarget(Position targetPosition) const;
    // Seed once per enemy; retain the generator state between moves.
    std::mt19937 randomEngine_{std::random_device{}()};
    EnemyConfig config_;
    //：配置回答“应该怎样行动”，状态回答“现在是什么情况”。
    //下述状态是动态的
    // 
    //配置规定规则，状态记录当前情况
    //：配置（Configuration）：敌人应该按什么规则行动。
        // 状态（State）：这个敌人现在处于什么情况。
    Position position_;
    Position wanderCenter_;
    Position soundSourcePosition_;
    int health_;
    double moveCooldownRemainingSeconds_ = 0.0;
    double waitRemainingSeconds_ = 0.0;
    double wanderRemainingSeconds_ = 0.0;
    EnemyState state_ = EnemyState::Wandering;
};

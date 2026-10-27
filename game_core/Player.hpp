#pragma once

#include "Map.hpp"
#include "Position.hpp"
#include <vector>

class Enemy;

// Keep levels ordered so a smaller value means a lower health band.
enum class HpLevel {
    Dead,
    Low,
    Medium,
    High
};
//等级从上到下的顺序是 Dead < Low < Medium < High，
// 所以可以直接用 < 比较。也就是等于说，Dead 的值最小，High 的值最大。

enum class Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

struct PlayerConfig {
    int maxHealth = 100;
    double normalMoveIntervalSeconds = 0.5;
    double stealthMoveIntervalSeconds = 1.0;
    double outBattleIntervalSeconds = 2.0;
};

class Player {
public:
    explicit Player(Position startPosition, PlayerConfig config = {});

    bool tryMove(Direction direction, const Map& map);
    void updateMoveCooldown(double elapsedSeconds);
    double getMoveCooldownRemainingSeconds() const;

    Position getPosition() const; //const在末尾表示不会修改成员变量
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

    void makeSound(Position soundSource, std::vector<Enemy>& enemies);

    void takeDamage(int damage);
private:
    // Fixed settings are separate from the state that changes each frame.
    PlayerConfig config_;

    Position position_; //成员变量用_结尾
    int health_;
    double moveCooldownRemainingSeconds_ = 0.0;
    double outBattleRemainingSeconds_;
    bool isInBattle_ = false;
    bool wasInBattle_ = false;
    bool stealthEnabled_ = false;
};

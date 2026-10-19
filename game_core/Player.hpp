#pragma once
#include <string>
#include <vector>
#include "Map.hpp"

struct Position
{
	int row;
	int column;
};

bool operator == (const Position& lhs, const Position& rhs);

// Keep levels ordered so a smaller value means a lower health band.
enum class HpLevel
{
	Dead,
	Low,
	Medium,
	High
};
//等级从上到下的顺序是 Dead < Low < Medium < High，
// 所以可以直接用 < 比较。也就是等于说，Dead 的值最小，High 的值最大。
enum class Direction
{
	UP,
	DOWN,
	LEFT,
	RIGHT
};

class Player {
public:
    explicit Player(Position startPosition, int maxHealth = 100);
    bool tryMove(
        Direction direction,
        const Map& map
    );

	HpLevel getHpLevel() const;
	Position getPosition() const;//const在末尾表示不会修改成员变量
	void setPosition(Position newPosition);
	bool isInBattle() const;
	void setInBattle(bool isInBattle);
	bool wasInBattle() const;
	void setWasInBattle(bool wasInBattle);
	int getHealth() const;
	int getMaxHealth() const;
	void setHealth(int health);
private:
	Position position_;//成员变量用_结尾
	bool isInBattle_ = false;
	bool wasInBattle_ = false;
	int maxHealth_;
	int health_;
};

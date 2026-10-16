#pragma once
#include <string>
#include <vector>


struct Position
{
	int row;
	int column;
};

bool operator == (const Position& lhs, const Position& rhs);

enum class Direction
{
	UP,
	DOWN,
	LEFT,
	RIGHT
};


class Player {
public:
    explicit Player(Position startPosition);//单参数用explicit

    bool tryMove(
        Direction direction,
        const std::vector<std::string>& map
    );

	Position getPosition() const;//const在末尾表示不会修改成员变量

private:
	Position position_;//成员变量用_结尾
};
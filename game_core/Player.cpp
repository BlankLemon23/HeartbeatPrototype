#include "Player.hpp"
#include "Map.hpp"


bool operator == (const Position& lhs, const Position& rhs) {
	return lhs.row == rhs.row && lhs.column == rhs.column;
}//lhs和rhs是常用的命名方式，表示左侧和右侧操作数（left-hand side / right-hand side），不是左值和右值分类

bool Player::tryMove(Direction direction, const Map& map) {
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
		position_ = newPosition;
		return true;
	}
	return false;
}

Player::Player(Position startPosition) : position_(startPosition) {
}//初始化列表 不用让 position_ = startPosition

Position Player::getPosition() const {
	return position_;
}

void Player::setPosition(Position newPosition)
{
	position_ = newPosition;
}

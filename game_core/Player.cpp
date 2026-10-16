#include "Map.hpp"
#include "Player.hpp"

bool operator == (const Position& lhs, const Position& rhs) {
	return lhs.row == rhs.row && lhs.column == rhs.column;
}//lhs和rhs是常用的命名方式，表示左值和右值 left hand side,right hand side

bool Player::tryMove(Direction direction, const std::vector<std::string>& map) {
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
	if (classifyCell(map, newPosition.row, newPosition.column) == CellType::Space) {
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
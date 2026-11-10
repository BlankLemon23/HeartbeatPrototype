#include "Position.hpp"

bool operator==(const Position& lhs, const Position& rhs) {
	return lhs.row == rhs.row && lhs.column == rhs.column;
}

int calculateSquaredDistanceToTarget(Position startPosition, Position targetPosition) {
	int rowOffset = targetPosition.row - startPosition.row;
	int columnOffset = targetPosition.column - startPosition.column;
	return rowOffset * rowOffset + columnOffset * columnOffset;
}
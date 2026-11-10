#pragma once

struct Position {
	int row;
	int column;
};

// Continuous coordinates in cells, in row-column order.
struct GridPoint {
	double row;
	double column;
};

bool operator==(const Position& lhs, const Position& rhs);

int calculateSquaredDistanceToTarget(Position startPosition, Position targetPosition);
